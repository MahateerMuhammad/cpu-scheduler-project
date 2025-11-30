#pragma once

#include <string>
#include <memory>

enum class ProcessState {
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
};

class Process {
public:
    Process(int pid, const std::string& name, int priority, int burstTime);
    ~Process();

    int getPid() const;
    const std::string& getName() const;
    int getPriority() const;
    int getEffectivePriority() const;
    int getBurstTime() const;
    int getRemainingTime() const;
    ProcessState getState() const;

    void setState(ProcessState state);
    void execute(int timeSlice); // simulate execution for given time slice
    void applyAging(int agingFactor); // boost priority based on waiting time

    // Timing info
    mutable int arrivalTime = 0;
    mutable int waitTime = 0;
    mutable int turnaroundTime = 0;

private:
    int pid_;
    std::string name_;
    int basePriority_;
    int effectivePriority_;
    int burstTime_;
    int remainingTime_;
    ProcessState state_ = ProcessState::NEW;
};
