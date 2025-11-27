#include "ErrorHandler.h"

#include <cstring> // strerror
#include <cerrno>  // errno
#include <string>  // std::string
#include <thread>
#include <chrono>

using namespace std::chrono;

bool ErrorHandler::BackoffRetry(uint8_t max_retry) noexcept
{
   if (retry_count < max_retry)
   {
      std::this_thread::sleep_for(milliseconds(RETRY_DELAY_MS));
      RETRY_DELAY_MS = (RETRY_DELAY_MS * 2 >= MAX_DELAY_MS)
                           ? MAX_DELAY_MS
                           : RETRY_DELAY_MS * 2;
      retry_count++;
      return true;
   }
   
   resetRetry();
   return false;
}

bool ErrorHandler::handleError(int err, uint8_t max_retry) noexcept
{
   char errmsg [LOG_BUF_SIZE];
   strerror_r(err, errmsg, LOG_BUF_SIZE);

   switch (errno_strategies[err])
   {
   case ErrorStrategy::Retry:
      logError("Retry: {}, Error: {}", retry_count, errmsg);
      return BackoffRetry(max_retry);
      break;
   case ErrorStrategy::Abort:
      logError("Process Aborted after this error: {}", errmsg);
      //std::abort();
      break;
   // case ErrorStrategy::Custom: // if needed
   //    break;
   default:
      break;
   }

   return false;
}

void ErrorHandler::handleError(ErrorName err) noexcept
{
   switch (error_strategies[static_cast<uint8_t>(err)])
   {
   case ErrorStrategy::Retry:
      // logRetry();
      // BackoffRetry();
      break;
   case ErrorStrategy::Abort:
      // logAbort();
      //std::abort();
      break;
   // case ErrorStrategy::Custom: // if needed
   //    break;
   default:
      break;
   } 
}