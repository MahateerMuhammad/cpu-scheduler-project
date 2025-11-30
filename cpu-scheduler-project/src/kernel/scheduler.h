#pragma once

#include "process.h"
#include "ready_queue.h"
#include "spinlock.h"
#include <vector>
#include <memory>
#include <functional>
#include <atomic>

// Statistics structure for reporting to GUI
struct SchedulerStats {
    int totalProcesses = 0;
    int runningProcesses = 0;
    int readyProcesses = 0;
    int waitingProcesses = 0;
    int terminatedProcesses = 0;
    double cpuUtilization = 0.0; // percentage
    int contextSwitchCount = 0;
    double averageWaitTime = 0.0;
    double averageTurnaroundTime = 0.0;
};

class Scheduler {
public:
    using StatsCallback = std::function<void(const SchedulerStats&)>;

    Scheduler();
    ~Scheduler();

    // Configuration
    void setTimeQuantum(int ms);
    void setAgingFactor(int seconds);

    // Process management
    std::shared_ptr<Process> createProcess(const std::string& name, int priority, int burstTime);
    void terminateProcess(int pid);
    void blockProcess(int pid);
    void unblockProcess(int pid);

    // Control
    void start();
    void pause();
    void stop();

    // Callback registration
    void setStatsCallback(StatsCallback cb);

    // GUI access methods
    std::vector<std::shared_ptr<Process>> getProcessList() const;
    SchedulerStats getStats() const;

private:
    void schedulerLoop(); // runs in background thread
    void selectNextProcess();
    void contextSwitch(std::shared_ptr<Process> next);
    void updateStats();
    void applyAging();

    // Internal data
    ReadyQueue readyQueue_;
    std::vector<std::shared_ptr<Process>> allProcesses_;
    std::shared_ptr<Process> currentProcess_ = nullptr;
    Spinlock lock_;
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    int timeQuantumMs_ = 100; // default 100ms
    int agingFactorSec_ = 5;   // default 5 seconds
    SchedulerStats stats_;
    StatsCallback statsCallback_ = nullptr;
    
    // I/O simulation
    std::vector<std::pair<std::shared_ptr<Process>, int>> blockedProcesses_; // process, remaining I/O time
    int ioSimulationCounter_ = 0;
};
