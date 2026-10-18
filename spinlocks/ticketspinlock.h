#ifndef TICKETSPINLOCK_H
#define TICKETSPINLOCK_H

#include <atomic>
#include "lock.h"

class TicketSpinLock final : public ILock {
    public:
        void Lock() override {
            int ticket = next_.fetch_add(1, std::memory_order_relaxed);

            while (serving_.load(std::memory_order_acquire) != ticket) {}
        }

        void Unlock() override {
            // Only the holder writes serving_, so an atomic RMW is unneeded.
            // Same ordering as fetch_add(release): relaxed read, release write.
            serving_.store(serving_.load(std::memory_order_relaxed) + 1,
                           std::memory_order_release);
        }
    private:
        // Separate cache lines to avoid false sharing between next_ and serving_.
        alignas(64) std::atomic<int> next_{0};
        alignas(64) std::atomic<int> serving_{0};
};

#endif // TICKETSPINLOCK_H
