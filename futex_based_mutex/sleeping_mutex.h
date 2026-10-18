#ifndef SLEEPINGMUTEX_H
#define SLEEPINGMUTEX_H

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>

#include "lock.h"

class SleepingMutex final : public ILock {
 public:
  // Non-optimised loop body:
  //   CompareExchange(kLockedNoWaiters, kLockedWaiters);
  //   FutexWait(kLockedWaiters);
  //   prev_value = CompareExchange(kUnlocked, kLockedWaiters);
  // The `if` below skips the CAS when the flag is already kLockedWaiters, and
  // skips the syscall when the CAS finds the lock already released.
  void Lock() override {
    int prev_value = CompareExchange(kUnlocked, kLockedNoWaiters);
    if (prev_value == kUnlocked) return;

    do {
      if (prev_value == kLockedWaiters ||
          CompareExchange(kLockedNoWaiters, kLockedWaiters) != kUnlocked) {
        FutexWait(kLockedWaiters);
      }

      // kLockedWaiters, since other threads may still be asleep.
      prev_value = CompareExchange(kUnlocked, kLockedWaiters);
    } while (prev_value != kUnlocked);
  }

  void Unlock() override {
    if (flag_.exchange(kUnlocked) == kLockedWaiters) {
      FutexWake(1);
    }
  }

 private:
  enum Flag : int { kUnlocked = 0, kLockedNoWaiters, kLockedWaiters };

  inline void FutexWait(Flag expected) {
    syscall(SYS_futex, reinterpret_cast<int*>(&flag_), FUTEX_WAIT_PRIVATE,
            expected, nullptr, nullptr, 0);
  }

  inline void FutexWake(int count) {
    syscall(SYS_futex, reinterpret_cast<int*>(&flag_), FUTEX_WAKE_PRIVATE,
            count, nullptr, nullptr, 0);
  }

  inline Flag CompareExchange(Flag expected, Flag desired) {
    flag_.compare_exchange_strong(expected, desired);
    return expected;
  }

  std::atomic<Flag> flag_;
};

#endif  // SLEEPINGMUTEX_H
