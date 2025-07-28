CREATE TABLE IF NOT EXISTS OrdersTable (
    timestamp DateTime64(9),
    last_update_time DateTime64(9),
    venue LowCardinality(String),
    protocol LowCardinality(String),
    msg_type LowCardinality(String),
    symbol LowCardinality(String),
    instrument_id UInt64,
    order_id UInt64,
    client_order_id UInt64,
    price Int64,
    quantity UInt32,
    filled_quantity UInt32,
    cancelled_quantity UInt32,
    side Enum8('Buy' = 0, 'Sell' = 1),
    status LowCardinality(String),
    time_in_force UInt8,
    order_type UInt8,
    priority_level UInt8,
    flags UInt32 DEFAULT 0
) ENGINE = MergeTree
PARTITION BY toYYYYMMDD(timestamp)
ORDER BY (symbol, timestamp)
SETTINGS index_granularity = 8192;
