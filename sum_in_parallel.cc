#include <atomic>
#include <chrono>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

using namespace std;

constexpr size_t kDefaultNumThreads = 8;

void SumThread(const vector<long long>& nums, const size_t start,
               const size_t length, atomic<long long>& result) {
  long long sum = 0;
  for (size_t i = 0; i < length && start + i < nums.size(); i++) {
    sum += nums[start + i];
  }
  result.fetch_add(sum, std::memory_order_relaxed);
}

long long SumInParallel(const vector<long long>& nums) {
  const size_t num_threads = thread::hardware_concurrency() != 0
                                 ? thread::hardware_concurrency()
                                 : kDefaultNumThreads;

  atomic<long long> result;

  const size_t nums_per_thread = (nums.size() + num_threads - 1) / num_threads;
  vector<thread> threads;
  for (size_t i = 0; i < num_threads; i++) {
    threads.emplace_back(SumThread, cref(nums), nums_per_thread * i,
                         nums_per_thread, ref(result));
  }

  for (thread& t : threads) {
    t.join();
  }

  return result.load(memory_order_relaxed);
}

long long Sum(const vector<long long>& nums) {
  long long result = 0;
  for (const long long num : nums) {
    result += num;
  }
  return result;
}

int main() {
  vector<long long> input(20'000'000);
  iota(input.begin(), input.end(), 1);

  constexpr long long kRuns = 100;
  auto bench = [&](const char* name, long long (*f)(const vector<long long>&)) {
    long long checksum = 0;
    const auto begin = chrono::steady_clock::now();
    for (long long i = 0; i < kRuns; i++) {
      checksum += f(input);
    }
    const auto end = chrono::steady_clock::now();
    const double avg_ms =
        chrono::duration<double, milli>(end - begin).count() / kRuns;
    cout << name << ": " << avg_ms << " ms/run (result " << checksum / kRuns
         << ")\n";
  };

  bench("Sum          ", Sum);
  bench("SumInParallel", SumInParallel);
}
