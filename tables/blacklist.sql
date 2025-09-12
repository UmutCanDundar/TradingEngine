CREATE TABLE IF NOT EXIST BLACKLIST (
   symbol String,
   instrument_id UInt64
) ENGINE = MergeTree
  SETTINGS index_granularity = 8192;