#include <atomic>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>

std::tuple<size_t, size_t> argParse(const int argc, char **argv)
{
  if (argc == 1 || argc > 3)
  {
    throw std::invalid_argument("usage: ./lab tries [seed]");
  }

  const int64_t tries = std::atoll(argv[1]);
  if (tries <= 0)
  {
    throw std::invalid_argument("tries count can't be not positive");
  }

  const int64_t seed = (argc == 3 ? std::atoll(argv[2]) : 0ull);
  if (seed < 0)
  {
    throw std::invalid_argument("seed can't be less than 0");
  }

  return {tries, seed};
}

auto calculateAsync(const size_t radius, const size_t threads_count, const size_t tries, const size_t seed)
{
  std::vector<std::thread> tasks;
  const auto               mutex   = std::make_shared<std::mutex>();
  const auto               counter = std::make_shared<size_t>(0);
  const auto               gen     = std::make_shared<std::mt19937>(seed);
  const auto               rng =
    std::make_shared<std::uniform_real_distribution<double>>(-static_cast<double>(radius), static_cast<double>(radius));

  auto count = [mutex, gen, counter, rng](size_t iters, const size_t radius) mutable
  {
    while (iters--)
    {
      std::lock_guard<std::mutex> guard(*mutex);
      const double                x = (*rng)(*gen);
      const double                y = (*rng)(*gen);
      *counter += (x * x + y * y <= radius * radius);
    }
  };

  const auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < threads_count - 1; ++i)
  {
    tasks.push_back(std::thread(count, tries / threads_count, radius));
  }
  tasks.push_back(std::thread(count, tries / threads_count + tries % threads_count, radius));

  return std::async(
    [tasks = std::move(tasks), counter, tries, radius, start]() mutable -> std::pair<double, double>
    {
      for (auto &task : tasks)
      {
        task.join();
      }
      const auto   end        = std::chrono::high_resolution_clock::now();
      const double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
      const double area       = (static_cast<double>(*counter) / tries) * radius * radius * 4;
      return {elapsed_ms, area};
    });
}

int main(int argc, char **argv)
{
  const auto [tries, seed] = argParse(argc, argv);
  std::vector<std::future<std::pair<double, double>>> result;

  for (size_t radius = 0, threads_count = 0; std::cin >> radius >> threads_count;)
  {
    if (radius <= 0 || threads_count <= 0)
    {
      throw std::invalid_argument("radius or threads count cant be not positive");
    }

    result.push_back(calculateAsync(radius, threads_count, tries, seed));
  }

  for (auto &answ : result)
  {
    const auto [time_ms, area] = answ.get();
    std::cout << std::fixed << std::setprecision(3) << time_ms << " " << area << "\n";
  }
}
