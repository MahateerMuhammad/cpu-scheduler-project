#include "ready_queue.h"

ReadyQueue::ReadyQueue() {}
ReadyQueue::~ReadyQueue() {}

void ReadyQueue::enqueue(const std::shared_ptr<Process>& proc) {
    SpinlockGuard guard(lock_);
    queue_.push(proc);
}

std::shared_ptr<Process> ReadyQueue::dequeue() {
    SpinlockGuard guard(lock_);
    if (queue_.empty()) return nullptr;
    auto top = queue_.top();
    queue_.pop();
    return top;
}

std::shared_ptr<Process> ReadyQueue::peek() const {
    // Note: const method, cannot lock mutable lock_; use mutable lock for simplicity
    const_cast<Spinlock&>(lock_).lock();
    if (queue_.empty()) {
        const_cast<Spinlock&>(lock_).unlock();
        return nullptr;
    }
    auto top = queue_.top();
    const_cast<Spinlock&>(lock_).unlock();
    return top;
}

bool ReadyQueue::empty() const {
    const_cast<Spinlock&>(lock_).lock();
    bool isEmpty = queue_.empty();
    const_cast<Spinlock&>(lock_).unlock();
    return isEmpty;
}

void ReadyQueue::applyAging(int agingFactor) {
    // Apply aging to all processes in the queue by rebuilding the priority queue
    std::vector<std::shared_ptr<Process>> temp;
    {
        SpinlockGuard guard(lock_);
        while (!queue_.empty()) {
            auto proc = queue_.top();
            queue_.pop();
            proc->applyAging(agingFactor);
            temp.push_back(proc);
        }
        // Reinsert with updated priorities (effective priority stored inside Process)
        for (auto& p : temp) {
            queue_.push(p);
        }
    }
}
