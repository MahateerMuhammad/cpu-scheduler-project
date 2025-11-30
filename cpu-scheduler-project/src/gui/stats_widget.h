#pragma once

#include <QWidget>
#include <QLabel>
#include <QFormLayout>
#include "../kernel/scheduler.h"

class StatsWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatsWidget(QWidget* parent = nullptr);
    ~StatsWidget();

    void updateStats(const SchedulerStats& stats);

private:
    QLabel* totalProcessesLabel_;
    QLabel* runningProcessesLabel_;
    QLabel* readyProcessesLabel_;
    QLabel* waitingProcessesLabel_;
    QLabel* terminatedProcessesLabel_;
    QLabel* cpuUtilizationLabel_;
    QLabel* contextSwitchLabel_;
    QLabel* avgWaitTimeLabel_;
    QLabel* avgTurnaroundTimeLabel_;
};
