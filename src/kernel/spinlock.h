#pragma once

#include <atomic>

class Spinlock {
public:
    Spinlock() = default;
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            // busy-wait
        }
    }
    void unlock() {
        flag.clear(std::memory_order_release);
    }
private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

// RAII guard for Spinlock
class SpinlockGuard {
public:
    explicit SpinlockGuard(Spinlock& lock) : lock_(lock) { lock_.lock(); }
    ~SpinlockGuard() { lock_.unlock(); }
private:
    Spinlock& lock_;
};
