#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>
#include <mutex>

namespace MemoryForensics {

class AppLogger {
public:
    static AppLogger& Instance();
    
    // Initialize logger with console and optional file output
    void Initialize(const std::string& logger_name = "MemoryForensics", 
                   const std::string& log_file = "");
    
    // Logging methods with automatic indentation
    template<typename... Args>
    void Debug(const std::string& format, Args&&... args);
    
    template<typename... Args>
    void Info(const std::string& format, Args&&... args);
    
    template<typename... Args>
    void Warn(const std::string& format, Args&&... args);
    
    template<typename... Args>
    void Error(const std::string& format, Args&&... args);
    
    // Indent management
    void IncreaseIndent();
    void DecreaseIndent();
    void ResetIndent();
    
    // Set logging level
    void SetLevel(spdlog::level::level_enum level);
    
    // Get underlying spdlog logger
    std::shared_ptr<spdlog::logger> GetLogger() const { return logger_; }

private:
    AppLogger() = default;
    ~AppLogger() = default;
    AppLogger(const AppLogger&) = delete;
    AppLogger& operator=(const AppLogger&) = delete;
    
    std::string GetIndentedMessage(const std::string& message) const;
    
    std::shared_ptr<spdlog::logger> logger_;
    mutable std::mutex indent_mutex_;
    int indent_level_ = 0;
    static constexpr const char* INDENT_STRING = "  ";
};

// RAII class for automatic indent management
class LogIndenter {
public:
    explicit LogIndenter(AppLogger& logger) : logger_(logger) {
        logger_.IncreaseIndent();
    }
    
    ~LogIndenter() {
        logger_.DecreaseIndent();
    }
    
private:
    AppLogger& logger_;
};

// Template implementations
template<typename... Args>
void AppLogger::Debug(const std::string& format, Args&&... args) {
    if (logger_) {
        logger_->debug(GetIndentedMessage(format), std::forward<Args>(args)...);
    }
}

template<typename... Args>
void AppLogger::Info(const std::string& format, Args&&... args) {
    if (logger_) {
        logger_->info(GetIndentedMessage(format), std::forward<Args>(args)...);
    }
}

template<typename... Args>
void AppLogger::Warn(const std::string& format, Args&&... args) {
    if (logger_) {
        logger_->warn(GetIndentedMessage(format), std::forward<Args>(args)...);
    }
}

template<typename... Args>
void AppLogger::Error(const std::string& format, Args&&... args) {
    if (logger_) {
        logger_->error(GetIndentedMessage(format), std::forward<Args>(args)...);
    }
}

// Convenience macros
#define LOG_DEBUG(...) MemoryForensics::AppLogger::Instance().Debug(__VA_ARGS__)
#define LOG_INFO(...) MemoryForensics::AppLogger::Instance().Info(__VA_ARGS__)
#define LOG_WARN(...) MemoryForensics::AppLogger::Instance().Warn(__VA_ARGS__)
#define LOG_ERROR(...) MemoryForensics::AppLogger::Instance().Error(__VA_ARGS__)
#define LOG_INDENT() MemoryForensics::LogIndenter _indent(MemoryForensics::AppLogger::Instance())

} // namespace MemoryForensics