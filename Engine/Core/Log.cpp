#include "Log.h"

namespace Muk {

std::shared_ptr<Logger> Log::s_CoreLogger;
std::shared_ptr<Logger> Log::s_ClientLogger;

void Log::Init() {
    s_CoreLogger = std::make_shared<Logger>("MUK");
    s_ClientLogger = std::make_shared<Logger>("APP");
    s_CoreLogger->SetLevel(Logger::Level::Trace);
    s_ClientLogger->SetLevel(Logger::Level::Trace);
    MUK_CORE_INFO("Muk Game Engine Logger initialized");
}

} // namespace Muk
