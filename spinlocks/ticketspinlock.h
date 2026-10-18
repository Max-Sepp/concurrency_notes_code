#ifndef TICKETSPINLOCK_H
#define TICKETSPINLOCK_H

#include <atomic>
#include "spinlock.h"

class TicketSpinLock final : public Spinlock {
    public:
        void Lock() override {
            int ticket = next_.fetch_add(1, std::memory_order_relaxed);

            while (serving_.load(std::memory_order_acquire) != ticket) {}
        }

        void Unlock() override {
            serving_.fetch_add(1, std::memory_order_release);
        }
    private:
        std::atomic<int> next_{0};
        std::atomic<int> serving_{0};
};

#endif // TICKETSPINLOCK_H
