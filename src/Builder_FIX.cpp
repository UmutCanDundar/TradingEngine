#include "Builder_FIX.h"

std::array<uint32_t, VENUE_COUNT> Builder_FIX::seq_nums{0};

char* Builder_FIX::buildHeader(char* buf, size_t body_len, Venue venue) noexcept 
    {
        uint8_t venue_index = static_cast<uint8_t>(venue); 
        const char* my_id = venueIDs[venue_index].my_id;
        const char* venue_id = venueIDs[venue_index].venue_id;
        const size_t my_id_len = venueIDs[venue_index].my_id_len;
        const size_t venue_id_len = venueIDs[venue_index].venue_id_len;

        buf = addTag(8,  "FIX.4.4", 7, buf);
        buf = addTag(9, static_cast<uint32_t>(body_len), buf);
        buf = addTag(49, my_id, my_id_len, buf);   // senin kimliğin
        buf = addTag(56, venue_id, venue_id_len, buf);  // karşı taraf (örneğin borsa)

        static uint32_t seqnum = 1;
        buf = addTag(34, seqnum++, buf);

        char ts_buf[24];
        const auto now = std::chrono::system_clock::now();
        const std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        gmtime_r(&t, &tm); // thread-safe UTC

        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        const int written = std::snprintf(
            ts_buf, sizeof(ts_buf),
            "%04d%02d%02d-%02d:%02d:%02d.%03d",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec,
            static_cast<int>(ms.count())
        );

        buf = addTag(52, ts_buf, written, buf);

            return buf;
    }

