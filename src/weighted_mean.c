
#include "postgres.h"
#include "fmgr.h"
#include "utils/memutils.h"
#include "utils/numeric.h"
#include "utils/builtins.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(_weighted_mean_intermediate);
PG_FUNCTION_INFO_V1(_weighted_mean_final);


typedef struct WeightedMeanInternalState
{
	Datum		running_sum;
	Datum		running_amount;
}	WeightedMeanInternalState;


static Datum
make_zero(void)
{
	return DirectFunctionCall1(int4_numeric, Int32GetDatum(0));
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
	Datum		zero = make_zero();

	state = PG_ARGISNULL(0)
			? NULL
			: (WeightedMeanInternalState *) PG_GETARG_POINTER(0);

	/* No row has ever been processed. */
	if (state == NULL)
	{
		return zero;
	}

	if (DirectFunctionCall2(numeric_eq,
							zero, state->running_amount))
	{
		total = zero;
	}
	else
	{
		total = DirectFunctionCall2(numeric_div,
									state->running_sum, state->running_amount);
	}

	PG_RETURN_NUMERIC(total);
}

Datum
_weighted_mean_intermediate(PG_FUNCTION_ARGS)
{
	WeightedMeanInternalState *state;
	Datum		value,
				amount,
				temp_total,
				old_sum,
				old_amount;
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
		state->running_sum = make_zero();
		state->running_amount = make_zero();
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
	amount = NumericGetDatum(PG_GETARG_NUMERIC(2));
	temp_total = DirectFunctionCall2(numeric_mul, value, amount);

	/*
	 * The new running totals must be allocated in the long-lived context.  We
	 * rely on the numeric_* functions to clean up after themselves (which they
	 * currently do, but only if the input is already detoasted); we could play
	 * safe and copy only the final results into aggcontext, but this turns out
	 * to have a measurable performance hit.
	 */

	oldcontext = MemoryContextSwitchTo(aggcontext);

	old_sum = state->running_sum;
	old_amount = state->running_amount;

	state->running_sum = DirectFunctionCall2(numeric_add,
											 state->running_sum, temp_total);

	state->running_amount = DirectFunctionCall2(numeric_add,
												state->running_amount, amount);

	pfree(DatumGetPointer(old_sum));
	pfree(DatumGetPointer(old_amount));

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_POINTER(state);
}
