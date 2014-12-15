#include "postgres.h"
#include "fmgr.h"
#include "utils/memutils.h"
#include "utils/numeric.h"
#include "utils/builtins.h"
#include "utils/datum.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(_weighted_mean_intermediate);
PG_FUNCTION_INFO_V1(_weighted_mean_final);
PG_FUNCTION_INFO_V1(_weighted_stddev_samp_intermediate);
PG_FUNCTION_INFO_V1(_weighted_stddev_samp_final);


typedef struct WeightedMeanInternalState
{
	Datum		running_sum;
	Datum		running_weight;
}	WeightedMeanInternalState;

/* http://en.wikipedia.org/wiki/Standard_deviation#Rapid_calculation_methods */
typedef struct WeightedStddevSampInternalState
{
	int64		n_prime;	/* number of elements with non-zero weights */
	Datum		zero;		/* always 0 */
	Datum		s_2;		/* sum(w_k * x_k ^ 2) */
	Datum		s_1;		/* sum(w_k * x_k ^ 1) */
	Datum		s_0;		/* sum(w_k) */
}	WeightedStddevSampInternalState;


static Datum
make_numeric(int64 i)
{
	return DirectFunctionCall1(int8_numeric, Int64GetDatumFast(i));
}

/*
 * We must not modify or free the state in here, and nor should we allocate
 * the result in a context other than the one we're called in.
 */

Datum
_weighted_mean_final(PG_FUNCTION_ARGS)
{
	WeightedMeanInternalState *state;
	Datum		total;
	Datum		zero = make_numeric(0);

	state = PG_ARGISNULL(0)
			? NULL
			: (WeightedMeanInternalState *) PG_GETARG_POINTER(0);

	/* No row has ever been processed. */
	if (state == NULL)
	{
		return zero;
	}

	if (DatumGetBool(DirectFunctionCall2(numeric_eq,
										 zero, state->running_weight)))
	{
		total = zero;
	}
	else
	{
		total = DirectFunctionCall2(numeric_div,
									state->running_sum, state->running_weight);
	}

	PG_RETURN_NUMERIC(total);
}

Datum
_weighted_mean_intermediate(PG_FUNCTION_ARGS)
{
	WeightedMeanInternalState *state;
	Datum		value,
				weight,
				temp_total,
				old_sum,
				old_weight;
	MemoryContext aggcontext,
				oldcontext;

	if (!AggCheckCallContext(fcinfo, &aggcontext))
	{
		/* cannot be called directly because of internal-type argument */
		elog(ERROR, "_weighted_mean_intermediate called in non-aggregate context");
	}

	if (PG_ARGISNULL(0))
	{
		oldcontext = MemoryContextSwitchTo(aggcontext);
		state = (WeightedMeanInternalState *) palloc(sizeof(WeightedMeanInternalState));
		state->running_sum = make_numeric(0);
		state->running_weight = make_numeric(0);
		MemoryContextSwitchTo(oldcontext);
	}
	else
	{
		state = (WeightedMeanInternalState *) PG_GETARG_POINTER(0);
	}

	/*
	 * We're non-strict, so we MUST check args for nullity ourselves before
	 * using them.  To preserve the behaviour of null inputs, we skip updating
	 * on them.
	 */

	if (PG_ARGISNULL(1) || PG_ARGISNULL(2))
		PG_RETURN_POINTER(state);

	/*
	 * We fetch and process the input in the shortlived calling context to
	 * avoid leaking memory in aggcontext per cycle. We force the input to be
	 * detoasted here, too, in the shortlived context. (PG_GETARG_DATUM does
	 * not detoast, but PG_GETARG_NUMERIC does.)
	 */

	value = NumericGetDatum(PG_GETARG_NUMERIC(1));
	weight = NumericGetDatum(PG_GETARG_NUMERIC(2));
	temp_total = DirectFunctionCall2(numeric_mul, value, weight);

	/*
	 * The new running totals must be allocated in the long-lived context.  We
	 * rely on the numeric_* functions to clean up after themselves (which they
	 * currently do, but only if the input is already detoasted); we could play
	 * safe and copy only the final results into aggcontext, but this turns out
	 * to have a measurable performance hit.
	 */

	oldcontext = MemoryContextSwitchTo(aggcontext);

	old_sum = state->running_sum;
	old_weight = state->running_weight;

	state->running_sum = DirectFunctionCall2(numeric_add,
											 state->running_sum, temp_total);

	state->running_weight = DirectFunctionCall2(numeric_add,
												state->running_weight, weight);

	pfree(DatumGetPointer(old_sum));
	pfree(DatumGetPointer(old_weight));

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_POINTER(state);
}

Datum
_weighted_stddev_samp_final(PG_FUNCTION_ARGS)
{
	WeightedStddevSampInternalState *state;
	Datum		result;

	state = PG_ARGISNULL(0)
			? NULL
			: (WeightedStddevSampInternalState *) PG_GETARG_POINTER(0);

	if ((state == NULL) || /* No row has ever been processed. */
		(state->n_prime < 2)) /* Too few non-zero weights */
	{
		PG_RETURN_NULL();
	}
	else
	{
		Datum	n_prime = make_numeric(state->n_prime);

		/* sqrt((n/(n-1)) * ((s0*s2 - s1*s1)/(s0*s0)) */

		result
			= DirectFunctionCall1(
				numeric_sqrt,
				DirectFunctionCall2(
					numeric_mul,
					DirectFunctionCall2(
						numeric_div,
						n_prime,
						DirectFunctionCall2(
							numeric_sub,
							n_prime,
							/*
							 * This rather convoluted way to compute the value
							 * 1 gives us a result which should have at least
							 * as big a decimal scale as s_2 does, which should
							 * guarantee that our result is as precise as the
							 * input...
							 */
							DirectFunctionCall2(
								numeric_add,
								DirectFunctionCall2(
									numeric_sub,
									state->s_2,
									state->s_2
									),
								make_numeric(1)
								)
							)
						),
					DirectFunctionCall2(
						numeric_div,
						DirectFunctionCall2(
							numeric_sub,
							DirectFunctionCall2(
								numeric_mul,
								state->s_0,
								state->s_2
								),
							DirectFunctionCall2(
								numeric_mul,
								state->s_1,
								state->s_1
								)
							),
						DirectFunctionCall2(
							numeric_mul,
							state->s_0,
							state->s_0
							)
						)
					)
				);
	}

	PG_RETURN_NUMERIC(result);
}

Datum
_weighted_stddev_samp_intermediate(PG_FUNCTION_ARGS)
{
	WeightedStddevSampInternalState *state;
	Datum		value,
				weight,
				old_s_0,
				old_s_1,
				old_s_2,
				w_v,
				w_v2;
	MemoryContext aggcontext,
				  oldcontext;

	if (!AggCheckCallContext(fcinfo, &aggcontext))
	{
		/* cannot be called directly because of internal-type argument */
		elog(ERROR, "_weighted_stddev_samp_intermediate called in non-aggregate context");
	}

	if (PG_ARGISNULL(0))
	{
		oldcontext = MemoryContextSwitchTo(aggcontext);
		state = (WeightedStddevSampInternalState *) palloc(sizeof(WeightedStddevSampInternalState));
		state->s_2 = make_numeric(0);
		state->s_1 = make_numeric(0);
		state->s_0 = make_numeric(0);
		state->zero = make_numeric(0);
		state->n_prime = 0;
		MemoryContextSwitchTo(oldcontext);
	}
	else
	{
		state = (WeightedStddevSampInternalState *) PG_GETARG_POINTER(0);
	}

	/*
	 * We're non-strict, so we MUST check args for nullity ourselves before
	 * using them.  To preserve the behaviour of null inputs, we skip updating
	 * on them.
	 */

	if (PG_ARGISNULL(1) || PG_ARGISNULL(2))
		PG_RETURN_POINTER(state);

	/*
	 * We fetch and process the input in the shortlived calling context to
	 * avoid leaking memory in aggcontext per cycle. We force the input to be
	 * detoasted here, too, in the shortlived context. (PG_GETARG_DATUM does
	 * not detoast, but PG_GETARG_NUMERIC does.)
	 */

	value = NumericGetDatum(PG_GETARG_NUMERIC(1));
	weight = NumericGetDatum(PG_GETARG_NUMERIC(2));

	/*
	 * We also skip updating when the weight is zero.
	 */
	if (DatumGetBool(DirectFunctionCall2(numeric_eq, weight, state->zero)))
		PG_RETURN_POINTER(state);

	/*
	 * Compute intermediate values w*v and w*(v^2) in the short-lived context
	 */

	w_v = DirectFunctionCall2(numeric_mul, weight, value);
	w_v2 = DirectFunctionCall2(numeric_mul, w_v, value);

	/*
	 * The new running totals must be allocated in the long-lived context.  We
	 * rely on the numeric_* functions to clean up after themselves (which they
	 * currently do, but only if the input is already detoasted); we could play
	 * safe and copy only the final results into aggcontext, but this turns out
	 * to have a measurable performance hit.
	 */

	oldcontext = MemoryContextSwitchTo(aggcontext);

	old_s_2 = state->s_2;
	old_s_1 = state->s_1;
	old_s_0 = state->s_0;

	state->s_0 = DirectFunctionCall2(numeric_add, old_s_0, weight);
	state->s_1 = DirectFunctionCall2(numeric_add, old_s_1, w_v);
	state->s_2 = DirectFunctionCall2(numeric_add, old_s_2, w_v2);

	state->n_prime += 1;

	pfree(DatumGetPointer(old_s_2));
	pfree(DatumGetPointer(old_s_1));
	pfree(DatumGetPointer(old_s_0));

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_POINTER(state);
}
