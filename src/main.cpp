#include "gui/mainwindow.h"
#include "utils/logger.h"
#include <QApplication>
#include <iostream>

int main(int argc, char *argv[]) {
    // Create Qt application
    QApplication app(argc, argv);
    
    // Set application metadata
    QApplication::setApplicationName("CPU Scheduler Simulator");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("CPU Scheduler Project");
    
    // Initialize logger
    Logger::instance().setLogFile("sched_stats.log");
    Logger::instance().log("=== CPU Scheduler Application Starting ===", LogLevel::INFO);
    
    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();
    
    Logger::instance().log("Main window displayed", LogLevel::INFO);
    
    // Run event loop
    int result = app.exec();
    
    Logger::instance().log("=== CPU Scheduler Application Exiting ===", LogLevel::INFO);
    
    return result;
}
