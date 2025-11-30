#include "stats_widget.h"
#include <QVBoxLayout>
#include <QGroupBox>

StatsWidget::StatsWidget(QWidget* parent)
    : QWidget(parent) {
    
    // Create labels
    totalProcessesLabel_ = new QLabel("0");
    runningProcessesLabel_ = new QLabel("0");
    readyProcessesLabel_ = new QLabel("0");
    waitingProcessesLabel_ = new QLabel("0");
    terminatedProcessesLabel_ = new QLabel("0");
    cpuUtilizationLabel_ = new QLabel("0.0%");
    contextSwitchLabel_ = new QLabel("0");
    avgWaitTimeLabel_ = new QLabel("0.0 ms");
    avgTurnaroundTimeLabel_ = new QLabel("0.0 ms");
    
    // Create form layout
    QFormLayout* formLayout = new QFormLayout();
    formLayout->addRow("Total Processes:", totalProcessesLabel_);
    formLayout->addRow("Running:", runningProcessesLabel_);
    formLayout->addRow("Ready:", readyProcessesLabel_);
    formLayout->addRow("Waiting:", waitingProcessesLabel_);
    formLayout->addRow("Terminated:", terminatedProcessesLabel_);
    formLayout->addRow("CPU Utilization:", cpuUtilizationLabel_);
    formLayout->addRow("Context Switches:", contextSwitchLabel_);
    formLayout->addRow("Avg Wait Time:", avgWaitTimeLabel_);
    formLayout->addRow("Avg Turnaround:", avgTurnaroundTimeLabel_);
    
    // Create group box
    QGroupBox* groupBox = new QGroupBox("Scheduler Statistics");
    groupBox->setLayout(formLayout);
    
    // Set main layout
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(groupBox);
    mainLayout->addStretch();
    setLayout(mainLayout);
}

StatsWidget::~StatsWidget() {}

void StatsWidget::updateStats(const SchedulerStats& stats) {
    totalProcessesLabel_->setText(QString::number(stats.totalProcesses));
    runningProcessesLabel_->setText(QString::number(stats.runningProcesses));
    readyProcessesLabel_->setText(QString::number(stats.readyProcesses));
    waitingProcessesLabel_->setText(QString::number(stats.waitingProcesses));
    terminatedProcessesLabel_->setText(QString::number(stats.terminatedProcesses));
    
    cpuUtilizationLabel_->setText(
        QString::number(stats.cpuUtilization, 'f', 1) + "%");
    
    contextSwitchLabel_->setText(QString::number(stats.contextSwitchCount));
    
    avgWaitTimeLabel_->setText(
        QString::number(stats.averageWaitTime, 'f', 2) + " ms");
    
    avgTurnaroundTimeLabel_->setText(
        QString::number(stats.averageTurnaroundTime, 'f', 2) + " ms");
}
