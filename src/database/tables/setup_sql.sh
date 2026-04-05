DIR="$(dirname "$0")"
clickhouse-client --multiquery < "${DIR}/fixtable.sql"
clickhouse-client --multiquery < "${DIR}/itchtable.sql"
#clickhouse-client --multiquery < "${DIR}/ouchtable.sql"
clickhouse-client --multiquery < "${DIR}/ordertable.sql"


touch "$1"
