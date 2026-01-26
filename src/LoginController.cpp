#include "LoginController.h"

LoginController::LoginController(SoupBinTcp &sbt, Builder_FIX &builder_fix, SessionManager &sess_mngr) noexcept : sbt_(sbt), builder_fix_(builder_fix), sess_mngr_(sess_mngr) {}