#include "ErrorHandler.h"

void ErrorHandler::BackoffRetry() noexcept
{
   std::this_thread::sleep_for(milliseconds(RETRY_DELAY_MS));
   RETRY_DELAY_MS = (RETRY_DELAY_MS * 2 >= MAX_DELAY_MS)
                        ? MAX_DELAY_MS
                        : RETRY_DELAY_MS * 2;
}

void ErrorHandler::logRetry(const char *msg) noexcept
{
   char buffer[LOG_BUF_SIZE];
   snprintf(buffer, sizeof(buffer), "%s => RETRIED", msg);
   LOG_ERROR(std::string(buffer));
}

void ErrorHandler::logAbort(const char *msg) noexcept
{
   char buffer[LOG_BUF_SIZE];
   snprintf(buffer, sizeof(buffer), "%s => ABORTED", msg);
   LOG_ERROR(std::string(buffer));
}

void ErrorHandler::handleError(int err) noexcept
{
   switch (errno_strategies[err])
   {
   case ErrorStrategy::Retry:
      logRetry(strerror(err));
      BackoffRetry();
      break;
   case ErrorStrategy::Abort:
      logAbort(strerror(err));
      std::abort();
      break;
   case ErrorStrategy::Custom:
      // sonra yapılacak
      break;
   default:
      break;
   }
}

void ErrorHandler::handleError(ErrorName err) noexcept
{
   switch (error_strategies[static_cast<int>(err)])
   {
   case ErrorStrategy::Retry:
      logRetry();
      BackoffRetry();
      break;
   case ErrorStrategy::Abort:
      logAbort();
      std::abort();
      break;
   case ErrorStrategy::Custom:
      // sonra yapılacak
      break;
   default:
      break;
   }
}