#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if (outFile_.is_open()) {
        outFile_.close();
    }
}

void Logger::log(const std::string& message, LogLevel level) {
    std::lock_guard<std::mutex> lock(mtx_);
    
    // Generate timestamp
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream timestamp;
    timestamp << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    timestamp << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    // Format log level
    std::string levelStr;
    switch (level) {
        case LogLevel::INFO:  levelStr = "INFO "; break;
        case LogLevel::DEBUG: levelStr = "DEBUG"; break;
        case LogLevel::ERROR: levelStr = "ERROR"; break;
    }
    
    // Format complete log message
    std::string logLine = "[" + timestamp.str() + "] [" + levelStr + "] " + message;
    
    // Output to console
    std::cout << logLine << std::endl;
    
    // Output to file (open if not already open)
    if (!outFile_.is_open()) {
        outFile_.open(logFilePath_, std::ios::app);
    }
    
    if (outFile_.is_open()) {
        outFile_ << logLine << std::endl;
        outFile_.flush(); // Ensure immediate write
    }
}

void Logger::setLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mtx_);
    
    // Close current file if open
    if (outFile_.is_open()) {
        outFile_.close();
    }
    
    // Update path and open new file
    logFilePath_ = path;
    outFile_.open(logFilePath_, std::ios::app);
}
