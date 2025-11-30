#pragma once

#include "process.h"
#include "spinlock.h"
#include <queue>
#include <vector>
#include <functional>

// Comparator for effective priority (higher priority = lower numeric value)
struct ProcessComparator {
    bool operator()(const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) const {
        // lower effectivePriority means higher priority
        return a->getEffectivePriority() > b->getEffectivePriority();
    }
};

class ReadyQueue {
public:
    ReadyQueue();
    ~ReadyQueue();

    void enqueue(const std::shared_ptr<Process>& proc);
    std::shared_ptr<Process> dequeue();
    std::shared_ptr<Process> peek() const;
    bool empty() const;
    void applyAging(int agingFactor);

private:
    std::priority_queue<std::shared_ptr<Process>, std::vector<std::shared_ptr<Process>>, ProcessComparator> queue_;
    Spinlock lock_;
};
