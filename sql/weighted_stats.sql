CREATE function _numeric_weighted_mean_intermediate (
  internal, 
  numeric,
  numeric)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE function _numeric_weighted_mean_final (
  internal)
RETURNS numeric
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

create aggregate weighted_mean (numeric, numeric)(
  sfunc = _numeric_weighted_mean_intermediate,
  finalfunc = _numeric_weighted_mean_final,
  stype = internal
);

CREATE function _numeric_weighted_stddev_samp_intermediate (
  internal, 
  numeric,
  numeric)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE function _numeric_weighted_stddev_samp_final (
  internal)
RETURNS numeric
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

create aggregate weighted_stddev_samp (numeric, numeric)(
  sfunc = _numeric_weighted_stddev_samp_intermediate,
  finalfunc = _numeric_weighted_stddev_samp_final,
  stype = internal
);

CREATE function _float8_weighted_mean_intermediate (
  float8[], 
  float8,
  float8)
RETURNS float8[]
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE function _float8_weighted_mean_final (float8[])
RETURNS float8
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

create aggregate weighted_mean (float8, float8)(
  sfunc = _float8_weighted_mean_intermediate,
  finalfunc = _float8_weighted_mean_final,
  stype = float8[]
);

CREATE function _float8_weighted_stddev_samp_intermediate (
  float8[], 
  float8,
  float8)
RETURNS float8[]
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE function _float8_weighted_stddev_samp_final (float8[])
RETURNS float8
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

create aggregate weighted_stddev_samp (float8, float8)(
  sfunc = _float8_weighted_stddev_samp_intermediate,
  finalfunc = _float8_weighted_stddev_samp_final,
  stype = float8[]
);

