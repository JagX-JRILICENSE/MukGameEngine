#pragma once

#include "Core.h"
#include <memory>
#include <string>
#include <iostream>
#include <mutex>

namespace Muk {

class Logger {
public:
    enum class Level { Trace, Info, Warn, Error, Critical };

    Logger(const std::string& name) : m_Name(name) {}

    void SetLevel(Level level) { m_Level = level; }

    template<typename... Args>
    void trace(const std::string& fmt, Args&&... args) {
        log(Level::Trace, fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    void info(const std::string& fmt, Args&&... args) {
        log(Level::Info, fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    void warn(const std::string& fmt, Args&&... args) {
        log(Level::Warn, fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    void error(const std::string& fmt, Args&&... args) {
        log(Level::Error, fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    void critical(const std::string& fmt, Args&&... args) {
        log(Level::Critical, fmt, std::forward<Args>(args)...);
    }

private:
    template<typename... Args>
    void log(Level level, const std::string& fmt, Args&&... args) {
        if (level < m_Level) return;
        std::lock_guard<std::mutex> lock(m_Mutex);
        // Simple formatting for now (placeholder - replace with fmt later)
        std::cout << "[" << m_Name << "] " << LevelToString(level) << ": " << fmt << std::endl;
    }

    static const char* LevelToString(Level level) {
        switch (level) {
            case Level::Trace: return "TRACE";
            case Level::Info: return "INFO";
            case Level::Warn: return "WARN";
            case Level::Error: return "ERROR";
            case Level::Critical: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }

    std::string m_Name;
    Level m_Level = Level::Trace;
    std::mutex m_Mutex;
};

class Log {
public:
    static void Init();
    static std::shared_ptr<Logger>& GetCoreLogger() { return s_CoreLogger; }
    static std::shared_ptr<Logger>& GetClientLogger() { return s_ClientLogger; }

private:
    static std::shared_ptr<Logger> s_CoreLogger;
    static std::shared_ptr<Logger> s_ClientLogger;
};

} // namespace Muk
