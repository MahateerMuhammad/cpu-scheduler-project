#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimer>
#include <memory>
#include "../kernel/scheduler.h"
#include "process_table_widget.h"
#include "stats_widget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onStartClicked();
    void onPauseClicked();
    void onStopClicked();
    void onAddProcessClicked();
    void onKillProcessClicked();
    void onApplyConfigClicked();
    void onUpdateTimer();

private:
    void setupUI();
    void updateProcessTable();
    void updateStatistics();
    void logMessage(const std::string& msg);
    
    // Scheduler
    std::shared_ptr<Scheduler> scheduler_;
    
    // Control buttons
    QPushButton* startButton_;
    QPushButton* pauseButton_;
    QPushButton* stopButton_;
    QPushButton* addProcessButton_;
    QPushButton* killProcessButton_;
    
    // Configuration
    QSpinBox* timeQuantumSpinBox_;
    QSpinBox* agingFactorSpinBox_;
    QPushButton* applyConfigButton_;
    
    // Display widgets
    ProcessTableWidget* processTable_;
    StatsWidget* statsWidget_;
    QTextEdit* logViewer_;
    
    // Update timer
    QTimer* updateTimer_;
    
    // State tracking
    bool schedulerRunning_;
};
