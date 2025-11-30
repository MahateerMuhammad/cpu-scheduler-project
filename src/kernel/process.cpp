#include "process.h"
#include <iostream>

Process::Process(int pid, const std::string& name, int priority, int burstTime)
    : pid_(pid), name_(name), basePriority_(priority), effectivePriority_(priority),
      burstTime_(burstTime), remainingTime_(burstTime) {}

Process::~Process() {}

int Process::getPid() const { return pid_; }
const std::string& Process::getName() const { return name_; }
int Process::getPriority() const { return basePriority_; }
int Process::getEffectivePriority() const { return effectivePriority_; }
int Process::getBurstTime() const { return burstTime_; }
int Process::getRemainingTime() const { return remainingTime_; }
ProcessState Process::getState() const { return state_; }

void Process::setState(ProcessState state) { state_ = state; }

void Process::execute(int timeSlice) {
    if (state_ != ProcessState::RUNNING) return;
    int execTime = std::min(timeSlice, remainingTime_);
    remainingTime_ -= execTime;
    if (remainingTime_ == 0) {
        state_ = ProcessState::TERMINATED;
    }
}

void Process::applyAging(int agingFactor) {
    // Simple aging: increase effective priority based on wait time
    effectivePriority_ = basePriority_ + (waitTime / agingFactor);
}
