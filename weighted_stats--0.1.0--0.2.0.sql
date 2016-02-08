\echo Use "ALTER EXTENSION weighted_stats UPDATE TO '0.2.0'" to load this file. \quit

/* Drop the aggregates, then the functions that define them. */
ALTER EXTENSION weighted_stats DROP AGGREGATE weighted_mean(numeric, numeric);
ALTER EXTENSION weighted_stats DROP AGGREGATE weighted_stddev_samp(numeric, numeric);

ALTER EXTENSION weighted_stats DROP FUNCTION _weighted_mean_intermediate(internal, numeric, numeric);
ALTER EXTENSION weighted_stats DROP FUNCTION _weighted_mean_final(internal, numeric, numeric);

ALTER EXTENSION weighted_stats DROP FUNCTION _weighted_stddev_samp_intermediate(internal, numeric, numeric);
ALTER EXTENSION weighted_stats DROP FUNCTION _weighted_stddev_samp_final(internal, numeric, numeric);

DROP AGGREGATE weighted_mean(numeric, numeric);
DROP AGGREGATE weighted_stddev_samp(numeric, numeric);

DROP FUNCTION _weighted_mean_intermediate(internal, numeric, numeric);
DROP FUNCTION _weighted_mean_final(internal, numeric, numeric);

DROP FUNCTION _weighted_stddev_samp_intermediate(internal, numeric, numeric);
DROP FUNCTION _weighted_stddev_samp_final(internal, numeric, numeric);

/* XXX need to get the rest of the function in here once it's done. */
