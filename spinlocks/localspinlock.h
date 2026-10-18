#ifndef LOCALSPINLOCK_H
#define LOCALSPINLOCK_H

#include <atomic>

#include "spinlock.h"

class LocalSpinLock final : public Spinlock {
    public:
        void Lock() override {
            while (lock_flag_.exchange(true, std::memory_order_acquire)) {
                while (lock_flag_.load(std::memory_order_relaxed)) {}
            }
        }

        void Unlock() override {
            lock_flag_.store(false, std::memory_order_release);
        }
    private:
        std::atomic<bool> lock_flag_{false};
};

#endif // LOCALSPINLOCK_H
