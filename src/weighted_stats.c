
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
	Datum		s_2; /* sum(w_k * x_k ^ 2) */
	Datum		s_1; /* sum(w_k * x_k ^ 1) */
	Datum		W;  /* sum(w_k) */
	Datum		A;  /* A_0 = 0.  A_k = A_k-1 + w_k * (x_k - A_k-1) / W_k, k > 0 */
	Datum		Q;  /* Q_0 = 0.  Q_k = Q_k-1 + w_k * (x_k - A_k-1) / (x_k - A_k), k > 0 */
	Datum		n_prime;  /* number of elements with non-zero weights */
}	WeightedStddevSampInternalState;


static Datum
make_numeric(int i)
{
	return DirectFunctionCall1(int4_numeric, Int32GetDatum(i));
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

	if (DirectFunctionCall2(numeric_eq,
							zero, state->running_weight))
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
	Datum		one = make_numeric(1);

	state = PG_ARGISNULL(0)
			? NULL
			: (WeightedStddevSampInternalState *) PG_GETARG_POINTER(0);

	if ((state == NULL) || /* No row has ever been processed. */
		(DirectFunctionCall2(numeric_le, state->n_prime, one))) /* Too few non-zero weights */
	{
		PG_RETURN_NULL();
	}
	else
	{
		result = DirectFunctionCall1(
					numeric_sqrt,
					DirectFunctionCall2(
						numeric_div,
						DirectFunctionCall2(numeric_mul, state->n_prime, state->Q),
						DirectFunctionCall2(
							numeric_mul,
							DirectFunctionCall2(numeric_sub, state->n_prime, one),
							state->W
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
				s_2, new_s_2,
				s_1, new_s_1,
				W, new_W,
				A, new_A,
				Q, new_Q,
				n_prime, new_n_prime;
	Datum		zero = make_numeric(0);
	Datum		one = make_numeric(1);
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
		state->W = make_numeric(0);
		state->A = make_numeric(0);
		state->Q = make_numeric(0);
		state->n_prime = make_numeric(0);
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
	if (DirectFunctionCall2(numeric_eq, weight, zero))
		PG_RETURN_POINTER(state);

	/*
	 * The new running totals must be allocated in the long-lived context.  We
	 * rely on the numeric_* functions to clean up after themselves (which they
	 * currently do, but only if the input is already detoasted); we could play
	 * safe and copy only the final results into aggcontext, but this turns out
	 * to have a measurable performance hit.
	 */

	s_2 = state->s_2;
	s_1 = state->s_1;
	W = state->W;
	A = state->A;
	Q = state->Q;
	n_prime = state->n_prime;

	new_s_2 = DirectFunctionCall2(
							 numeric_add,
							 s_2,
							 DirectFunctionCall2(
								 numeric_mul,
								 weight,
								 DirectFunctionCall2(
									 numeric_mul,
									 value,
									 value
								 )
							 )
						 );
	new_s_1 = DirectFunctionCall2(
					 numeric_add,
					 s_1,
					 DirectFunctionCall2(
						 numeric_mul,
						 weight,
						 value
					 )
				 );
	new_W = DirectFunctionCall2(numeric_add, W, weight);
	new_A = DirectFunctionCall2(
				   numeric_add,
				   A,
				   DirectFunctionCall2(
					   numeric_mul,
					   weight,
					   DirectFunctionCall2(
						   numeric_div,
						   DirectFunctionCall2(numeric_sub, value, A),
						   new_W
					   )
				   )
			   );
	new_Q = DirectFunctionCall2(
				   numeric_add,
				   Q,
				   DirectFunctionCall2(
					   numeric_mul,
					   weight,
					   DirectFunctionCall2(
						   numeric_mul,
						   DirectFunctionCall2(numeric_sub, weight, A),
						   DirectFunctionCall2(numeric_sub, weight, new_A)
					   )
				   )
			   );
	new_n_prime = DirectFunctionCall2(numeric_add, n_prime, one);

	oldcontext = MemoryContextSwitchTo(aggcontext);

	pfree(DatumGetPointer(state->s_2));
	state->s_2 = datumCopy(new_s_2, false, -1);
	pfree(DatumGetPointer(state->s_1));
	state->s_1 = datumCopy(new_s_1, false, -1);
	pfree(DatumGetPointer(state->W));
	state->W = datumCopy(new_W, false, -1);
	pfree(DatumGetPointer(state->A));
	state->A = datumCopy(new_A, false, -1);
	pfree(DatumGetPointer(state->Q));
	state->Q = datumCopy(new_Q, false, -1);
	pfree(DatumGetPointer(state->n_prime));
	state->n_prime = datumCopy(new_n_prime, false, -1);

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_POINTER(state);
}
