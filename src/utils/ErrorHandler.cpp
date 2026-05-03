#include "ErrorHandler.h"


#include <cstring> // strerror
#include <cerrno>  
#include <thread>
#include <chrono>

using namespace std::chrono;

void ErrorHandler::resetRetry_errno() noexcept
{
   errno_RETRY_DELAY_MS = START_RETRY_DELAY_MS;
   errno_retry_count = 0;
}

bool ErrorHandler::BackoffRetry_errno(uint8_t max_retry) noexcept
{
   if (errno_retry_count < max_retry)
   {
      std::this_thread::sleep_for(milliseconds(errno_RETRY_DELAY_MS));
      errno_RETRY_DELAY_MS = (errno_RETRY_DELAY_MS * 2 >= MAX_DELAY_MS)
                           ? MAX_DELAY_MS
                           : errno_RETRY_DELAY_MS * 2;
      errno_retry_count++;
      return true;
   }
   
   resetRetry_errno();
   return false;
}

bool ErrorHandler::handleError(int err, uint8_t max_retry) noexcept
{
   char errmsg [LOG_BUF_SIZE];
   auto _ = strerror_r(err, errmsg, LOG_BUF_SIZE);
   (void)_;

   switch (errno_strategies[err])
   {
   case ErrorStrategy::Retry:
      logError("Retry: {}, Error: {}", errno_retry_count, errmsg);
      return BackoffRetry_errno(max_retry);
   case ErrorStrategy::Abort:
      logError("Process Aborted after this error: {}", errmsg);
      std::abort();
      break;
   default:
      break;
   }

   return false;
}

void ErrorHandler::resetRetry_error() noexcept
{
   error_RETRY_DELAY_MS = START_RETRY_DELAY_MS;
   error_retry_count = 0;
}

bool ErrorHandler::BackoffRetry_error(uint8_t max_retry) noexcept
{
   if (error_retry_count < max_retry)
   {
      std::this_thread::sleep_for(milliseconds(error_RETRY_DELAY_MS));
      error_RETRY_DELAY_MS = (error_RETRY_DELAY_MS * 2 >= MAX_DELAY_MS)
                           ? MAX_DELAY_MS
                           : error_RETRY_DELAY_MS * 2;
      error_retry_count++;
      return true;
   }
   
   resetRetry_error();
   return false;
}

bool ErrorHandler::handleError(ErrorName err, uint8_t max_retry) noexcept
{
   switch (error_strategies[static_cast<uint8_t>(err)])
   {
   case ErrorStrategy::Retry:
      logError("Retry: {}, Error: {}", error_retry_count, ErrorHandler::toString(err));
      return BackoffRetry_error(max_retry);
   case ErrorStrategy::Abort:
      logError("Process Aborted after this error: {}", ErrorHandler::toString(err));
      std::abort();
      break;
   default:
      break;
   }

   return false;
}