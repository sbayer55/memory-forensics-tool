#include "app_logger.hpp"
#include <iostream>

namespace MemoryForensics {

AppLogger& AppLogger::Instance() {
    static AppLogger instance;
    return instance;
}

void AppLogger::Initialize(const std::string& logger_name, const std::string& log_file) {
    try {
        std::vector<spdlog::sink_ptr> sinks;
        
        // Console sink with colors
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);
        console_sink->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
        sinks.push_back(console_sink);
        
        // File sink if specified
        if (!log_file.empty()) {
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
            file_sink->set_level(spdlog::level::debug);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
            sinks.push_back(file_sink);
        }
        
        // Create logger with both sinks
        logger_ = std::make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());
        logger_->set_level(spdlog::level::info);
        logger_->flush_on(spdlog::level::warn);
        
        // Register as default logger
        spdlog::set_default_logger(logger_);
        
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}

void AppLogger::IncreaseIndent() {
    std::lock_guard<std::mutex> lock(indent_mutex_);
    ++indent_level_;
}

void AppLogger::DecreaseIndent() {
    std::lock_guard<std::mutex> lock(indent_mutex_);
    if (indent_level_ > 0) {
        --indent_level_;
    }
}

void AppLogger::ResetIndent() {
    std::lock_guard<std::mutex> lock(indent_mutex_);
    indent_level_ = 0;
}

void AppLogger::SetLevel(spdlog::level::level_enum level) {
    if (logger_) {
        logger_->set_level(level);
    }
}

std::string AppLogger::GetIndentedMessage(const std::string& message) const {
    std::lock_guard<std::mutex> lock(indent_mutex_);
    std::string indent;
    for (int i = 0; i < indent_level_; ++i) {
        indent += INDENT_STRING;
    }
    return indent + message;
}

} // namespace MemoryForensics