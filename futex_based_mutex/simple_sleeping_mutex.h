#ifndef SIMPLESLEEPINGMUTEX_H
#define SIMPLESLEEPINGMUTEX_H

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>

#include "lock.h"

class SimpleSleepingMutex final : public ILock {
  enum Flag : int { kUnlocked = 0, kLocked };

 public:
  void Lock() override {
    while (flag_.exchange(kLocked) == kLocked) {
      FutexWait(kLocked);
    }
  }

  void Unlock() override {
    flag_.store(kUnlocked);
    FutexWake(1);
  }

 private:
  inline void FutexWait(Flag expected) {
    syscall(SYS_futex, reinterpret_cast<int*>(&flag_), FUTEX_WAIT_PRIVATE,
            expected, nullptr, nullptr, 0);
  }

  inline void FutexWake(int count) {
    syscall(SYS_futex, reinterpret_cast<int*>(&flag_), FUTEX_WAKE_PRIVATE,
            count, nullptr, nullptr, 0);
  }

  std::atomic<Flag> flag_;
};

#endif  // SIMPLESLEEPINGMUTEX_H
