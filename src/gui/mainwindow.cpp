#include "mainwindow.h"
#include "../utils/logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QScrollBar>
#include <QTime>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), schedulerRunning_(false) {
    
    // Initialize scheduler
    scheduler_ = std::make_shared<Scheduler>();
    
    // Setup UI
    setupUI();
    
    // Setup update timer (100ms updates)
    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::onUpdateTimer);
    updateTimer_->start(100);
    
    // Set window properties
    setWindowTitle("CPU Scheduler Simulator");
    resize(1200, 800);
    
    logMessage("CPU Scheduler application started");
}

MainWindow::~MainWindow() {
    if (scheduler_) {
        scheduler_->stop();
    }
}

void MainWindow::setupUI() {
    // Create central widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    // Top section: Control panel
    QGroupBox* controlGroup = new QGroupBox("Control Panel");
    QHBoxLayout* controlLayout = new QHBoxLayout();
    
    startButton_ = new QPushButton("Start Scheduler");
    pauseButton_ = new QPushButton("Pause");
    stopButton_ = new QPushButton("Stop");
    addProcessButton_ = new QPushButton("Add Process");
    killProcessButton_ = new QPushButton("Kill Selected");
    
    pauseButton_->setEnabled(false);
    stopButton_->setEnabled(false);
    
    controlLayout->addWidget(startButton_);
    controlLayout->addWidget(pauseButton_);
    controlLayout->addWidget(stopButton_);
    controlLayout->addWidget(addProcessButton_);
    controlLayout->addWidget(killProcessButton_);
    controlLayout->addStretch();
    
    controlGroup->setLayout(controlLayout);
    mainLayout->addWidget(controlGroup);
    
    // Configuration panel
    QGroupBox* configGroup = new QGroupBox("Configuration");
    QHBoxLayout* configLayout = new QHBoxLayout();
    
    configLayout->addWidget(new QLabel("Time Quantum (ms):"));
    timeQuantumSpinBox_ = new QSpinBox();
    timeQuantumSpinBox_->setRange(10, 1000);
    timeQuantumSpinBox_->setValue(100);
    timeQuantumSpinBox_->setSingleStep(10);
    configLayout->addWidget(timeQuantumSpinBox_);
    
    configLayout->addWidget(new QLabel("Aging Factor (sec):"));
    agingFactorSpinBox_ = new QSpinBox();
    agingFactorSpinBox_->setRange(1, 60);
    agingFactorSpinBox_->setValue(5);
    configLayout->addWidget(agingFactorSpinBox_);
    
    applyConfigButton_ = new QPushButton("Apply");
    configLayout->addWidget(applyConfigButton_);
    configLayout->addStretch();
    
    configGroup->setLayout(configLayout);
    mainLayout->addWidget(configGroup);
    
    // Middle section: Process table and statistics
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    
    // Process table
    QGroupBox* tableGroup = new QGroupBox("Process Table");
    QVBoxLayout* tableLayout = new QVBoxLayout();
    processTable_ = new ProcessTableWidget();
    tableLayout->addWidget(processTable_);
    tableGroup->setLayout(tableLayout);
    splitter->addWidget(tableGroup);
    
    // Statistics widget
    statsWidget_ = new StatsWidget();
    splitter->addWidget(statsWidget_);
    
    splitter->setStretchFactor(0, 3);  // Process table gets more space
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter, 3);  // Give more space to this section
    
    // Bottom section: Log viewer
    QGroupBox* logGroup = new QGroupBox("Scheduler Log");
    QVBoxLayout* logLayout = new QVBoxLayout();
    logViewer_ = new QTextEdit();
    logViewer_->setReadOnly(true);
    logViewer_->setMaximumHeight(150);
    logLayout->addWidget(logViewer_);
    logGroup->setLayout(logLayout);
    mainLayout->addWidget(logGroup, 1);
    
    // Connect signals
    connect(startButton_, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(pauseButton_, &QPushButton::clicked, this, &MainWindow::onPauseClicked);
    connect(stopButton_, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(addProcessButton_, &QPushButton::clicked, this, &MainWindow::onAddProcessClicked);
    connect(killProcessButton_, &QPushButton::clicked, this, &MainWindow::onKillProcessClicked);
    connect(applyConfigButton_, &QPushButton::clicked, this, &MainWindow::onApplyConfigClicked);
}

void MainWindow::onStartClicked() {
    scheduler_->start();
    schedulerRunning_ = true;
    
    startButton_->setEnabled(false);
    pauseButton_->setEnabled(true);
    stopButton_->setEnabled(true);
    
    logMessage("Scheduler started");
}

void MainWindow::onPauseClicked() {
    scheduler_->pause();
    pauseButton_->setText("Resume");
    
    disconnect(pauseButton_, &QPushButton::clicked, this, &MainWindow::onPauseClicked);
    connect(pauseButton_, &QPushButton::clicked, this, [this]() {
        scheduler_->start();
        pauseButton_->setText("Pause");
        disconnect(pauseButton_, nullptr, this, nullptr);
        connect(pauseButton_, &QPushButton::clicked, this, &MainWindow::onPauseClicked);
        logMessage("Scheduler resumed");
    });
    
    logMessage("Scheduler paused");
}

void MainWindow::onStopClicked() {
    scheduler_->stop();
    schedulerRunning_ = false;
    
    startButton_->setEnabled(true);
    pauseButton_->setEnabled(false);
    stopButton_->setEnabled(false);
    pauseButton_->setText("Pause");
    
    logMessage("Scheduler stopped");
}

void MainWindow::onAddProcessClicked() {
    // Dialog for process name
    bool ok;
    QString name = QInputDialog::getText(this, "Add Process", 
                                         "Process Name:", 
                                         QLineEdit::Normal, 
                                         QString("Process_%1").arg(rand() % 1000), 
                                         &ok);
    if (!ok || name.isEmpty()) return;
    
    // Dialog for priority
    int priority = QInputDialog::getInt(this, "Add Process",
                                        "Priority (0-10, 0=highest):",
                                        5, 0, 10, 1, &ok);
    if (!ok) return;
    
    // Dialog for burst time
    int burstTime = QInputDialog::getInt(this, "Add Process",
                                          "Burst Time (ms):",
                                          500, 100, 10000, 100, &ok);
    if (!ok) return;
    
    // Create process
    auto proc = scheduler_->createProcess(name.toStdString(), priority, burstTime);
    
    logMessage("Created process: " + name.toStdString() + 
               " (PID=" + std::to_string(proc->getPid()) + 
               ", Priority=" + std::to_string(priority) + 
               ", Burst=" + std::to_string(burstTime) + "ms)");
}

void MainWindow::onKillProcessClicked() {
    int pid = processTable_->getSelectedPid();
    if (pid < 0) {
        QMessageBox::warning(this, "Kill Process", 
                            "Please select a process to kill.");
        return;
    }
    
    scheduler_->terminateProcess(pid);
    logMessage("Terminated process PID=" + std::to_string(pid));
}

void MainWindow::onApplyConfigClicked() {
    int timeQuantum = timeQuantumSpinBox_->value();
    int agingFactor = agingFactorSpinBox_->value();
    
    scheduler_->setTimeQuantum(timeQuantum);
    scheduler_->setAgingFactor(agingFactor);
    
    logMessage("Configuration updated: TimeQuantum=" + 
               std::to_string(timeQuantum) + "ms, AgingFactor=" + 
               std::to_string(agingFactor) + "s");
}

void MainWindow::onUpdateTimer() {
    updateProcessTable();
    updateStatistics();
}

void MainWindow::updateProcessTable() {
    if (scheduler_) {
        auto processes = scheduler_->getProcessList();
        processTable_->updateProcessList(processes);
    }
}

void MainWindow::updateStatistics() {
    if (scheduler_) {
        auto stats = scheduler_->getStats();
        statsWidget_->updateStats(stats);
    }
}

void MainWindow::logMessage(const std::string& msg) {
    Logger::instance().log(msg, LogLevel::INFO);
    
    // Also display in GUI
    QString timestamp = QTime::currentTime().toString("HH:mm:ss");
    logViewer_->append("[" + timestamp + "] " + QString::fromStdString(msg));
    
    // Auto-scroll to bottom
    logViewer_->verticalScrollBar()->setValue(
        logViewer_->verticalScrollBar()->maximum());
}
