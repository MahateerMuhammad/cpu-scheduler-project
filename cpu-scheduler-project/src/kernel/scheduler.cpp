#include "scheduler.h"
#include "process.h"
#include <thread>
#include <chrono>
#include <algorithm>
#include <cstdlib>

Scheduler::Scheduler() {}
Scheduler::~Scheduler() { stop(); }

void Scheduler::setTimeQuantum(int ms) { timeQuantumMs_ = ms; }
void Scheduler::setAgingFactor(int seconds) { agingFactorSec_ = seconds; }

std::shared_ptr<Process> Scheduler::createProcess(const std::string& name, int priority, int burstTime) {
    static int nextPid = 1;
    static auto startTime = std::chrono::steady_clock::now();
    
    auto proc = std::make_shared<Process>(nextPid++, name, priority, burstTime);
    proc->setState(ProcessState::NEW);
    
    // Set arrival time
    auto now = std::chrono::steady_clock::now();
    proc->arrivalTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
    
    {
        SpinlockGuard guard(lock_);
        allProcesses_.push_back(proc);
        readyQueue_.enqueue(proc);
        proc->setState(ProcessState::READY);
    }
    updateStats();
    return proc;
}

void Scheduler::terminateProcess(int pid) {
    SpinlockGuard guard(lock_);
    for (auto& p : allProcesses_) {
        if (p->getPid() == pid) {
            p->setState(ProcessState::TERMINATED);
            break;
        }
    }
    updateStats();
}

void Scheduler::blockProcess(int pid) {
    SpinlockGuard guard(lock_);
    for (auto& p : allProcesses_) {
        if (p->getPid() == pid && p->getState() == ProcessState::RUNNING) {
            p->setState(ProcessState::WAITING);
            break;
        }
    }
    updateStats();
}

void Scheduler::unblockProcess(int pid) {
    SpinlockGuard guard(lock_);
    for (auto& p : allProcesses_) {
        if (p->getPid() == pid && p->getState() == ProcessState::WAITING) {
            p->setState(ProcessState::READY);
            readyQueue_.enqueue(p);
            break;
        }
    }
    updateStats();
}

void Scheduler::start() {
    if (running_) return;
    running_ = true;
    paused_ = false;
    std::thread(&Scheduler::schedulerLoop, this).detach();
}

void Scheduler::pause() { paused_ = true; }

void Scheduler::stop() { running_ = false; }

void Scheduler::setStatsCallback(StatsCallback cb) { statsCallback_ = cb; }

void Scheduler::schedulerLoop() {
    while (running_) {
        if (paused_) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); continue; }
        
        selectNextProcess();
        
        if (currentProcess_) {
            currentProcess_->execute(timeQuantumMs_);
            
            // Simulate I/O blocking: 10% chance (less aggressive)
            ioSimulationCounter_++;
            if (ioSimulationCounter_ % 10 == 0 && 
                currentProcess_->getState() != ProcessState::TERMINATED &&
                currentProcess_->getRemainingTime() > 500) { // Only block if enough time left
                
                // Block current process for I/O
                int currentPid = currentProcess_->getPid();
                currentProcess_->setState(ProcessState::WAITING);
                
                // Add to blocked list with short I/O time (100-300ms)
                int ioTime = 100 + (rand() % 200);
                {
                    SpinlockGuard guard(lock_);
                    blockedProcesses_.push_back({currentProcess_, ioTime});
                }
                
                currentProcess_ = nullptr; // Release CPU
            }
            else if (currentProcess_->getState() == ProcessState::TERMINATED) {
                currentProcess_ = nullptr;
            }
        }
        
        // Check blocked processes and unblock if I/O complete
        {
            SpinlockGuard guard(lock_);
            auto it = blockedProcesses_.begin();
            while (it != blockedProcesses_.end()) {
                it->second -= timeQuantumMs_; // Decrease I/O time
                
                if (it->second <= 0) {
                    // I/O complete, unblock process
                    auto proc = it->first;
                    proc->setState(ProcessState::READY);
                    readyQueue_.enqueue(proc);
                    it = blockedProcesses_.erase(it);
                } else {
                    ++it;
                }
            }
        }
        
        applyAging();
        updateStats();
        std::this_thread::sleep_for(std::chrono::milliseconds(timeQuantumMs_));
    }
}

void Scheduler::selectNextProcess() {
    if (!currentProcess_ || currentProcess_->getState() == ProcessState::TERMINATED) {
        SpinlockGuard guard(lock_);
        if (!readyQueue_.empty()) {
            currentProcess_ = readyQueue_.dequeue();
            currentProcess_->setState(ProcessState::RUNNING);
        }
    }
}

void Scheduler::contextSwitch(std::shared_ptr<Process> next) {
    // Not used in this simple implementation; placeholder for future extension
    (void)next;
}

void Scheduler::applyAging() {
    SpinlockGuard guard(lock_);
    readyQueue_.applyAging(agingFactorSec_);
}

void Scheduler::updateStats() {
    static auto startTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    int currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
    
    // Simple stats calculation
    SchedulerStats newStats{};
    newStats.totalProcesses = static_cast<int>(allProcesses_.size());
    int running = 0, ready = 0, waiting = 0, terminated = 0;
    int totalWait = 0, totalTurnaround = 0;
    
    for (const auto& p : allProcesses_) {
        // Update wait time for READY processes
        if (p->getState() == ProcessState::READY) {
            p->waitTime += timeQuantumMs_;
        }
        
        // Update turnaround time for non-terminated processes
        if (p->getState() != ProcessState::TERMINATED) {
            p->turnaroundTime = currentTime - p->arrivalTime;
        }
        
        switch (p->getState()) {
            case ProcessState::RUNNING: running++; break;
            case ProcessState::READY: ready++; break;
            case ProcessState::WAITING: waiting++; break;
            case ProcessState::TERMINATED: terminated++; break;
            default: break;
        }
        totalWait += p->waitTime;
        totalTurnaround += p->turnaroundTime;
    }
    
    newStats.runningProcesses = running;
    newStats.readyProcesses = ready;
    newStats.waitingProcesses = waiting;
    newStats.terminatedProcesses = terminated;
    newStats.contextSwitchCount = stats_.contextSwitchCount;
    newStats.cpuUtilization = (running > 0) ? 100.0 : 0.0;
    
    if (newStats.totalProcesses > 0) {
        newStats.averageWaitTime = static_cast<double>(totalWait) / newStats.totalProcesses;
        newStats.averageTurnaroundTime = static_cast<double>(totalTurnaround) / newStats.totalProcesses;
    }
    
    stats_ = newStats;
    if (statsCallback_) {
        statsCallback_(stats_);
    }
}

std::vector<std::shared_ptr<Process>> Scheduler::getProcessList() const {
    return allProcesses_;
}

SchedulerStats Scheduler::getStats() const {
    return stats_;
}

