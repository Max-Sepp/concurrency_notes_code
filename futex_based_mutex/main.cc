// Futex-based mutex demo / benchmark.
//
// Every thread increments a shared counter under the chosen lock, then does
// `work` steps of a dependent computation before releasing it, so the lock is
// held for a while. The final count proves mutual exclusion; the run time shows
// what sleeping in the kernel costs compared to spinning.
//
// `make run`, `make strace` and `make tsan` do all of the below; `make format`
// runs clang-format over the sources.
//
// Build:
//   g++ -std=c++20 -O2 -g -pthread -I.. -o mutex_bench main.cc
//
// Run (args: <simple|sleeping> [threads] [iterations per thread] [work]):
//   ./mutex_bench simple   8 1000000 100
//   ./mutex_bench sleeping 8 1000000 100
//
// Count the futex syscalls each run makes:
//   strace -f -c -e trace=futex ./mutex_bench simple 8 100000
//
// SimpleSleepingMutex has two states. A waiter that loses the exchange() goes
// to sleep with FUTEX_WAIT, which only blocks if the flag is still kLocked, so
// a release between the exchange() and the syscall is never missed. Unlock()
// can't tell whether anyone is waiting, though, so it pays for a FUTEX_WAKE
// syscall on every release, even when the lock was never contended.

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "simple_sleeping_mutex.h"
#include "sleeping_mutex.h"

using namespace std;

constexpr size_t kDefaultNumThreads = 8;
constexpr size_t kDefaultIterations = 1'000'000;
constexpr size_t kDefaultWork = 100;

// State the critical section works on. Only the lock protects it.
struct Shared {
  size_t counter = 0;
  uint64_t state = 1;
};

unique_ptr<ILock> MakeLock(const string& name) {
  if (name == "simple") return make_unique<SimpleSleepingMutex>();
  if (name == "sleeping") return make_unique<SleepingMutex>();
  return nullptr;
}

void Worker(ILock& lock, size_t iterations, size_t work, Shared& shared) {
  for (size_t i = 0; i < iterations; i++) {
    lock.Lock();
    shared.counter++;  // Deliberately non-atomic: only the lock keeps this
                       // correct.
    // Each LCG step depends on the last, so the compiler can't collapse the
    // loop and the lock stays held for roughly `work` multiply-adds.
    for (size_t w = 0; w < work; w++) {
      shared.state =
          shared.state * 6364136223846793005ULL + 1442695040888963407ULL;
    }
    lock.Unlock();
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    cerr << "usage: " << argv[0]
         << " <simple|sleeping> [threads] [iterations per thread] [work]\n";
    return EXIT_FAILURE;
  }

  const string name = argv[1];
  const size_t num_threads = argc > 2 ? stoul(argv[2]) : kDefaultNumThreads;
  const size_t iterations = argc > 3 ? stoul(argv[3]) : kDefaultIterations;
  const size_t work = argc > 4 ? stoul(argv[4]) : kDefaultWork;

  unique_ptr<ILock> lock = MakeLock(name);
  if (!lock) {
    cerr << "unknown lock '" << name << "', expected simple or sleeping\n";
    return EXIT_FAILURE;
  }

  Shared shared;

  const auto begin = chrono::steady_clock::now();
  vector<thread> threads;
  for (size_t i = 0; i < num_threads; i++) {
    threads.emplace_back(Worker, ref(*lock), iterations, work, ref(shared));
  }
  for (thread& t : threads) {
    t.join();
  }
  const auto end = chrono::steady_clock::now();

  const size_t expected = num_threads * iterations;
  const auto ms = chrono::duration_cast<chrono::milliseconds>(end - begin);

  cout << "lock:     " << name << '\n'
       << "threads:  " << num_threads << '\n'
       << "work:     " << work << '\n'
       << "counter:  " << shared.counter << " (expected " << expected << ")\n"
       << "time:     " << ms.count() << " ms\n";

  if (shared.counter != expected) {
    cerr << "mutual exclusion violated!\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
