CREATE function _weighted_mean_intermediate (
  internal, 
  numeric,
  numeric)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE function _weighted_mean_final (
  internal)
RETURNS numeric
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

create aggregate weighted_mean (numeric, numeric)(
  sfunc = _weighted_mean_intermediate,
  finalfunc = _weighted_mean_final,
  stype = internal
);

CREATE function _weighted_stddev_samp_intermediate (
  internal, 
  numeric,
  numeric)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE function _weighted_stddev_samp_final (
  internal)
RETURNS numeric
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

create aggregate weighted_stddev_samp (numeric, numeric)(
  sfunc = _weighted_stddev_samp_intermediate,
  finalfunc = _weighted_stddev_samp_final,
  stype = internal
);
