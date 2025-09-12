CREATE TABLE IF NOT EXISTS FIX_Table (
    timestamp DateTime64(9),
    venue LowCardinality(String),
    protocol LowCardinality(String),
    msg_type LowCardinality(String),
    symbol LowCardinality(String),
    order_id String,
    cl_ord_id String,
    exec_id String,
    price Int64,
    quantity UInt32,
    leaves_qty UInt32,
    filled_qty UInt32,
    side LowCardinality(String),
    ord_status LowCardinality(String),
    exec_type LowCardinality(String),
    ord_type LowCardinality(String),
    time_in_force LowCardinality(String),
    fix_version LowCardinality(String),
    flags UInt32 DEFAULT 0
) ENGINE = MergeTree
PARTITION BY toYYYYMMDD(timestamp)
ORDER BY (symbol, timestamp)
SETTINGS index_granularity = 8192;
