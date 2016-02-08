CREATE FUNCTION _numeric_weighted_mean_intermediate (
    internal,
    numeric,
    numeric)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE FUNCTION _numeric_weighted_mean_final (internal)
RETURNS numeric
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE AGGREGATE weighted_mean (numeric, numeric) (
    sfunc = _numeric_weighted_mean_intermediate,
    finalfunc = _numeric_weighted_mean_final,
    stype = internal
);

CREATE FUNCTION _numeric_weighted_stddev_samp_intermediate (
    internal,
    numeric,
    numeric)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE FUNCTION _numeric_weighted_stddev_samp_final (internal)
RETURNS numeric
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE AGGREGATE weighted_stddev_samp (numeric, numeric) (
    sfunc = _numeric_weighted_stddev_samp_intermediate,
    finalfunc = _numeric_weighted_stddev_samp_final,
    stype = internal
);

CREATE FUNCTION _float8_weighted_mean_intermediate (
    internal,
    float8,
    float8)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE FUNCTION _float8_weighted_mean_final (internal)
RETURNS float8
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE AGGREGATE weighted_mean (float8, float8) (
    sfunc = _float8_weighted_mean_intermediate,
    finalfunc = _float8_weighted_mean_final,
    stype = internal
);

CREATE FUNCTION _float8_weighted_stddev_samp_intermediate (
    internal,
    float8,
    float8)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE FUNCTION _float8_weighted_stddev_samp_final (internal)
RETURNS float8
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE AGGREGATE weighted_stddev_samp (float8, float8) (
    sfunc = _float8_weighted_stddev_samp_intermediate,
    finalfunc = _float8_weighted_stddev_samp_final,
    stype = internal
);

CREATE FUNCTION _float4_weighted_mean_intermediate (
    internal,
    float4,
    float4)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE FUNCTION _float4_weighted_mean_final (internal)
RETURNS float4
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE AGGREGATE weighted_mean (float4, float4) (
    sfunc = _float4_weighted_mean_intermediate,
    finalfunc = _float4_weighted_mean_final,
    stype = internal
);

CREATE FUNCTION _float4_weighted_stddev_samp_intermediate (
    internal,
    float4,
    float4)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE FUNCTION _float4_weighted_stddev_samp_final (internal)
RETURNS float4
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE;

CREATE AGGREGATE weighted_stddev_samp (float4, float4) (
    sfunc = _float4_weighted_stddev_samp_intermediate,
    finalfunc = _float4_weighted_stddev_samp_final,
    stype = internal
);

