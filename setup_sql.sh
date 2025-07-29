
clickhouse-client --multiquery < tables/fixtable.sql
clickhouse-client --multiquery < tables/itchtable.sql
clickhouse-client --multiquery < tables/sbetable.sql
clickhouse-client --multiquery < tables/ordertable.sql
clickhouse-client --multiquery < tables/blacklist.sql

touch /home/umut/Masaüstü/MarketData/build/setup_sql.done
