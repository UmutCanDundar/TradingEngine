#include "Logger.h"

void Logger::Init(const std::string &filename)
{
   // Sadece ilk seferde kurulum yapılır (idempotent)
   if (instance_ != nullptr)
      return;

   spdlog::init_thread_pool(8192, 1); // Queue: 8192, 1 background thread

   auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, false);

   instance_ = std::make_unique<spdlog::async_logger>(
       "async_logger",                               // logger ismi
       file_sink,                                    // tek sink
       spdlog::thread_pool(),                        // thread pool (global)
       spdlog::async_overflow_policy::overrun_oldest // queue dolarsa eskiyi at
   );

   instance_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%# %!] %v");
   instance_->set_level(spdlog::level::info);
   instance_->flush_on(spdlog::level::err);      // Hata ve üstü loglar hemen dosyaya yazılır
   spdlog::flush_every(std::chrono::seconds(1)); // Diğerleri ise her 1 saniyede bir flush edilir
}
