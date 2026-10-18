#ifndef SIMPLELOCK_H
#define SIMPLELOCK_H

#include <atomic>

#include "spinlock.h"

class SimpleLock final : public Spinlock {
  public:
      void Lock() override {
          while (lock_flag_.exchange(true, std::memory_order_acquire)) {}
      }

      void Unlock() override {
          lock_flag_.store(false, std::memory_order_release);
      }
  private:
      std::atomic<bool> lock_flag_{false};
};

#endif // SIMPLELOCK_H
