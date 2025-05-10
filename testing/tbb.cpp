#include <algorithm>
#include <chrono>
#include <iostream>
#include <oneapi/tbb/parallel_invoke.h>
#include <random>
#include <vector>

int main() {
  constexpr size_t N = 1 << 20;
  std::vector<int> v1(N), v2(N);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, 255);

  auto fill_vectors = [&]() {
    for (auto &x : v1)
      x = dist(gen);
    for (auto &x : v2)
      x = dist(gen);
  };

  fill_vectors();
  auto t0 = std::chrono::high_resolution_clock::now();
  std::sort(v1.begin(), v1.end());
  std::sort(v2.begin(), v2.end());
  auto t1 = std::chrono::high_resolution_clock::now();
  auto seq_us =
      std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

  fill_vectors();
  auto t2 = std::chrono::high_resolution_clock::now();
  tbb::parallel_invoke([&] { std::sort(v1.begin(), v1.end()); },
                       [&] { std::sort(v2.begin(), v2.end()); });
  auto t3 = std::chrono::high_resolution_clock::now();
  auto par_us =
      std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();

  std::cout << "Sequential sort time: " << seq_us << " μs\n";
  std::cout << "Parallel sort time:   " << par_us << " μs\n";

  return 0;
}
