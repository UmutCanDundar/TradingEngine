#include "SessionManager.h"

SessionManager::SessionManager() noexcept : session_contexts_(initialize_SessionContext()), session_auths_(initialize_SessionAuth()) {}