#include <array>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

template <typename T, size_t N>
class BlockingRingQueue {
 public:
  void Push(T item) {
    {
      unique_lock lock(mutex_);
      not_full_.wait(lock, [this]() -> bool { return Next(tail_) != head_; });
      elements_[tail_] = std::move(item);
      tail_ = Next(tail_);
    }
    not_empty_.notify_one();
  }

  T Pop() {
    T result;
    {
      unique_lock lock(mutex_);
      not_empty_.wait(lock, [this]() -> bool { return head_ != tail_; });
      result = std::move(elements_[head_]);
      head_ = Next(head_);
    }
    not_full_.notify_one();
    return result;
  }

 private:
  static inline size_t Next(size_t x) { return (x + 1) % (N + 1); }

  mutex mutex_;
  condition_variable not_empty_;
  condition_variable not_full_;
  array<T, N + 1> elements_;
  size_t head_{0};
  size_t tail_{0};
};

int main() {
  // Single thread: items come out in the order they went in.
  {
    BlockingRingQueue<int, 3> queue;
    for (int i = 1; i <= 3; i++) {
      queue.Push(i);
    }
    cout << "FIFO order:";
    for (int i = 0; i < 3; i++) {
      cout << " " << queue.Pop();
    }
    cout << " (expected 1 2 3)\n";
  }

  // Many producers and consumers through a small queue, so both Push and Pop
  // regularly have to block. Every item must arrive exactly once.
  {
    constexpr int kProducers = 4;
    constexpr int kConsumers = 4;
    constexpr long long kItemsPerProducer = 100'000;
    constexpr long long kTotalItems = kProducers * kItemsPerProducer;
    static_assert(kTotalItems % kConsumers == 0);

    BlockingRingQueue<long long, 4> queue;
    vector<long long> consumer_sums(kConsumers, 0);
    vector<thread> threads;

    for (int p = 0; p < kProducers; p++) {
      threads.emplace_back([&queue, p] {
        for (long long i = 0; i < kItemsPerProducer; i++) {
          queue.Push(p * kItemsPerProducer + i + 1);
        }
      });
    }
    for (int c = 0; c < kConsumers; c++) {
      threads.emplace_back([&queue, &consumer_sums, c] {
        for (long long i = 0; i < kTotalItems / kConsumers; i++) {
          consumer_sums[c] += queue.Pop();
        }
      });
    }
    for (thread& t : threads) {
      t.join();
    }

    long long sum = 0;
    for (const long long s : consumer_sums) {
      sum += s;
    }
    const long long expected = kTotalItems * (kTotalItems + 1) / 2;
    cout << "Producer/consumer sum: " << sum << " (expected " << expected
         << ") " << (sum == expected ? "OK" : "MISMATCH") << "\n";
  }
}
