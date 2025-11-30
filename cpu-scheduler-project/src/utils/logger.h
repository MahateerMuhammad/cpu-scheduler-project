#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

enum class LogLevel { INFO, DEBUG, ERROR };

class Logger {
public:
    static Logger& instance();
    void log(const std::string& message, LogLevel level = LogLevel::INFO);
    void setLogFile(const std::string& path);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream outFile_;
    std::mutex mtx_;
    std::string logFilePath_ = "sched_stats.log";
};
