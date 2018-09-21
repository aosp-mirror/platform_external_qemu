#include "third_party/darwinn/port/blocking_counter.h"

#include <chrono>  // NOLINT
#include <thread>  // NOLINT
#include <vector>

#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace {

// Tests copied from absl/synchronization/blocking_counter_tests.cc.

void PauseAndDecreaseCounter(BlockingCounter* counter, int* done) {
  std::this_thread::sleep_for(std::chrono::seconds(2));
  *done = 1;
  counter->DecrementCount();
}

TEST(BlockingCounterTest, BasicFunctionality) {
  // This test verifies that BlockingCounter functions correctly. Starts a
  // number of threads that just sleep for a second and decrement a counter.

  // Initialize the counter.
  constexpr int num_workers = 10;
  BlockingCounter counter(num_workers);

  std::vector<std::thread> workers;
  std::vector<int> done(num_workers, 0);

  // Start a number of parallel tasks that will just wait for a seconds and
  // then decrement the count.
  workers.reserve(num_workers);
  for (int k = 0; k < num_workers; k++) {
    workers.emplace_back(
        [&counter, &done, k] { PauseAndDecreaseCounter(&counter, &done[k]); });
  }

  // Wait for the threads to have all finished.
  counter.Wait();

  // Check that all the workers have completed.
  for (int k = 0; k < num_workers; k++) {
    EXPECT_EQ(1, done[k]);
  }

  for (std::thread& w : workers) {
    w.join();
  }
}

}  // namespace
}  // namespace darwinn
}  // namespace platforms
