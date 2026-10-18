// Spinlock demo / benchmark.
//
// Every thread increments a shared counter under the chosen lock, then does
// `work` steps of a dependent computation before releasing it, so the lock is
// held for a while. The final count proves mutual exclusion; the run time and
// perf counters show how much cache-coherence traffic each implementation
// generates while spinning.
//
// Build:
//   g++ -std=c++20 -O2 -g -pthread -o spinlock_bench main.cc
//
// Run (args: <simple|local|ticket> [threads] [iterations per thread] [work]):
//   ./spinlock_bench simple 8 1000000 100
//   ./spinlock_bench local  8 1000000 100
//   ./spinlock_bench ticket 8 1000000 100
//
// Compare cache behaviour under perf (needs kernel.perf_event_paranoid <= 2,
// e.g. `sudo sysctl kernel.perf_event_paranoid=1`):
//   EVENTS=cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,cycles,instructions
//   perf stat -e $EVENTS ./spinlock_bench simple 8 1000000
//   perf stat -e $EVENTS ./spinlock_bench local  8 1000000
//
// Cache-line contention in more detail (needs perf_event_paranoid <= 1):
//   perf c2c record ./spinlock_bench simple 8 1000000 && perf c2c report
//
// SimpleLock spins on exchange(). Every attempt is a write, so the waiters keep
// pulling the cache line into Modified state and away from each other (the
// line "ping-pongs"). LocalSpinLock (test-and-test-and-set) spins on a plain
// load instead. Waiters share the line in Shared state and only read from their
// own L1 until the holder's release invalidates it.
//
// The pay-off depends on how long the lock is held. With work = 0 the run is
// all handoffs, and LocalSpinLock can miss more: on release every waiter misses
// at once and they all race to exchange() (a "thundering herd"). Raise `work`
// and SimpleLock's waiters keep stealing the line from the holder while
// LocalSpinLock's waiters stay quiet in their own caches.
//
// TicketSpinLock hands the lock out in FIFO order. Each waiter takes one
// fetch_add() on next_ and then only reads serving_, so there is no exchange()
// race on release. Every waiter still spins on the same serving_ line, though,
// so each Unlock() invalidates it in all of their caches. Strict FIFO also means
// the next owner is always a different core, and that core is often
// descheduled once threads > cores.

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "localspinlock.h"
#include "simplelock.h"
#include "ticketspinlock.h"

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
  if (name == "simple") return make_unique<SimpleLock>();
  if (name == "local") return make_unique<LocalSpinLock>();
  if (name == "ticket") return make_unique<TicketSpinLock>();
  return nullptr;
}

void Worker(ILock& lock, size_t iterations, size_t work, Shared& shared) {
  for (size_t i = 0; i < iterations; i++) {
    lock.Lock();
    shared.counter++;  // Deliberately non-atomic: only the lock keeps this correct.
    // Each LCG step depends on the last, so the compiler can't collapse the
    // loop and the lock stays held for roughly `work` multiply-adds.
    for (size_t w = 0; w < work; w++) {
      shared.state = shared.state * 6364136223846793005ULL + 1442695040888963407ULL;
    }
    lock.Unlock();
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    cerr << "usage: " << argv[0]
         << " <simple|local|ticket> [threads] [iterations per thread] [work]\n";
    return EXIT_FAILURE;
  }

  const string name = argv[1];
  const size_t num_threads =
      argc > 2 ? stoul(argv[2]) : kDefaultNumThreads;
  const size_t iterations = argc > 3 ? stoul(argv[3]) : kDefaultIterations;
  const size_t work = argc > 4 ? stoul(argv[4]) : kDefaultWork;

  unique_ptr<ILock> lock = MakeLock(name);
  if (!lock) {
    cerr << "unknown lock '" << name << "', expected simple, local or ticket\n";
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
