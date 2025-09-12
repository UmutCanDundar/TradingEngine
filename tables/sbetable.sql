CREATE TABLE IF NOT EXISTS SBE_Table (
    timestamp DateTime64(9),
    venue LowCardinality(String),
    protocol LowCardinality(String),
    template_id UInt16,
    instrument_id UInt64,
    order_id UInt64,
    trade_id UInt64,
    price UInt32,
    quantity UInt32,
    new_quantity UInt32,
    lot_size UInt32,
    side UInt8,
    aggressor_side UInt8,
    market_state UInt8,
    currency_code LowCardinality(String),
    new_seq_no UInt64,
    flags UInt32 DEFAULT 0
) ENGINE = MergeTree
PARTITION BY toYYYYMMDD(timestamp)
ORDER BY (instrument_id, timestamp)
SETTINGS index_granularity = 8192;
