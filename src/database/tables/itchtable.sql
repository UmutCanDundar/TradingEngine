CREATE TABLE IF NOT EXISTS ITCH_Table (
    timestamp DateTime64(9),
    venue LowCardinality(String),
    protocol LowCardinality(String),
    msg_type LowCardinality(String),
    stock LowCardinality(String),
    order_ref UInt64,
    price UInt32,
    quantity UInt32,
    executed_quantity UInt32,
    execution_price UInt32,
    cancelled_quantity UInt32,
    side LowCardinality(String),
    mpid LowCardinality(String),
    match_id LowCardinality(String),
    flags UInt32 DEFAULT 0
) ENGINE = MergeTree
PARTITION BY toYYYYMMDD(timestamp)
ORDER BY (stock, timestamp)
SETTINGS index_granularity = 8192;
