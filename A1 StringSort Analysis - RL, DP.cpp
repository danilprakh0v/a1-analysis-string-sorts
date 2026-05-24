#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

class ComparisonCounter {
 public:
  static void Reset() { character_comparisons_ = 0; }
  static void Add(long long value = 1) { character_comparisons_ += value; }
  static long long Get() { return character_comparisons_; }
 private:
  static long long character_comparisons_;
};

long long ComparisonCounter::character_comparisons_ = 0;

class StringGenerator {
 public:
  explicit StringGenerator(unsigned int seed = 1337)
      : random_engine_(seed),
        length_distribution_(10, 200),
        character_distribution_(0, static_cast<int>(GetAlphabet().size()) - 1) {}

  std::vector<std::string> GenerateRandomDataset(int size, int min_length,
                                                 int max_length) const {
    return GenerateBaseDataset(size, min_length, max_length);
  }

  std::vector<std::string> GenerateReverseSortedDataset(int size,
                                                       int min_length,
                                                       int max_length) const {
    std::vector<std::string> values =
        GenerateBaseDataset(size, min_length, max_length);
    std::sort(values.begin(), values.end());
    std::reverse(values.begin(), values.end());
    return values;
  }

  std::vector<std::string> GenerateNearlySortedDataset(int size, int min_length,
                                                       int max_length) const {
    std::vector<std::string> values =
        GenerateBaseDataset(size, min_length, max_length);
    std::sort(values.begin(), values.end());

    std::uniform_int_distribution<int> swap_count_distribution(
        1, std::max(1, size / 20));
    int swap_count = swap_count_distribution(random_engine_);

    std::uniform_int_distribution<int> index_distribution(0, size - 1);
    for (int index = 0; index < swap_count; ++index) {
      int first = index_distribution(random_engine_);
      int second = index_distribution(random_engine_);
      std::swap(values[first], values[second]);
    }
    return values;
  }

  std::vector<std::string> GeneratePrefixHeavyDataset(int size, int min_length,
                                                      int max_length) const {
    std::vector<std::string> values;
    values.reserve(size);
    std::uniform_int_distribution<int> prefix_length_distribution(5, 40);
    int prefix_length = prefix_length_distribution(random_engine_);
    std::string prefix = RandomString(prefix_length);
    for (int index = 0; index < size; ++index) {
      int current_length = RandomLength(min_length, max_length);
      int suffix_length = std::max(0, current_length - prefix_length);
      std::string value = prefix;
      value += RandomString(suffix_length);
      values.push_back(std::move(value));
    }
    return values;
  }

 private:
  int RandomLength(int min_length, int max_length) const {
    std::uniform_int_distribution<int> distribution(min_length, max_length);
    return distribution(random_engine_);
  }

  std::string RandomString(int length) const {
    std::string result;
    result.reserve(length);
    const std::string& alphabet = GetAlphabet();
    for (int index = 0; index < length; ++index) {
      result.push_back(alphabet[character_distribution_(random_engine_)]);
    }
    return result;
  }

  std::vector<std::string> GenerateBaseDataset(int size, int min_length,
                                               int max_length) const {
    std::vector<std::string> values;
    values.reserve(size);
    for (int index = 0; index < size; ++index) {
      values.push_back(RandomString(RandomLength(min_length, max_length)));
    }
    return values;
  }

  static const std::string& GetAlphabet() {
    static const std::string alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "!@#%:;^&*()-.";
    return alphabet;
  }

  mutable std::mt19937 random_engine_;
  mutable std::uniform_int_distribution<int> length_distribution_;
  mutable std::uniform_int_distribution<int> character_distribution_;
};

int LexicographicalCompare(const std::string& first,
                           const std::string& second) {
  const int first_size = static_cast<int>(first.size());
  const int second_size = static_cast<int>(second.size());
  const int limit = std::min(first_size, second_size);
  for (int index = 0; index < limit; ++index) {
    ComparisonCounter::Add();
    if (first[index] < second[index]) return -1;
    ComparisonCounter::Add();
    if (first[index] > second[index]) return 1;
  }
  if (first_size < second_size) return -1;
  if (first_size > second_size) return 1;
  return 0;
}

void StandardQuickSort(std::vector<std::string>& values, int left, int right) {
  if (left >= right) return;
  int i = left;
  int j = right;
  const std::string pivot = values[(left + right) / 2];
  while (i <= j) {
    while (LexicographicalCompare(values[i], pivot) < 0) ++i;
    while (LexicographicalCompare(values[j], pivot) > 0) --j;
    if (i <= j) {
      std::swap(values[i], values[j]);
      ++i;
      --j;
    }
  }
  if (left < j) StandardQuickSort(values, left, j);
  if (i < right) StandardQuickSort(values, i, right);
}

void StandardMergeSort(std::vector<std::string>& values,
                       std::vector<std::string>& buffer,
                       int left,
                       int right) {
  if (right - left <= 1) return;
  int middle = left + (right - left) / 2;
  StandardMergeSort(values, buffer, left, middle);
  StandardMergeSort(values, buffer, middle, right);
  int first = left, second = middle, index = left;
  while (first < middle && second < right) {
    if (LexicographicalCompare(values[first], values[second]) <= 0) {
      buffer[index++] = values[first++];
    } else {
      buffer[index++] = values[second++];
    }
  }
  while (first < middle) buffer[index++] = values[first++];
  while (second < right) buffer[index++] = values[second++];
  for (int current = left; current < right; ++current) values[current] = buffer[current];
}

int CharAt(const std::string& value, int depth) {
  if (depth >= static_cast<int>(value.size())) return -1;
  ComparisonCounter::Add();
  return static_cast<unsigned char>(value[depth]);
}

int LcpCompare(const std::string& first, const std::string& second) {
  const int first_size = static_cast<int>(first.size());
  const int second_size = static_cast<int>(second.size());
  const int limit = std::min(first_size, second_size);
  for (int index = 0; index < limit; ++index) {
    ComparisonCounter::Add();
    if (first[index] < second[index]) return -1;
    ComparisonCounter::Add();
    if (first[index] > second[index]) return 1;
  }
  if (first_size < second_size) return -1;
  if (first_size > second_size) return 1;
  return 0;
}

void StringMergeSort(std::vector<std::string>& values,
                     std::vector<std::string>& buffer,
                     int left,
                     int right) {
  if (right - left <= 1) return;
  int middle = left + (right - left) / 2;
  StringMergeSort(values, buffer, left, middle);
  StringMergeSort(values, buffer, middle, right);
  int first = left, second = middle, index = left;
  while (first < middle && second < right) {
    if (LcpCompare(values[first], values[second]) <= 0) {
      buffer[index++] = values[first++];
    } else {
      buffer[index++] = values[second++];
    }
  }
  while (first < middle) buffer[index++] = values[first++];
  while (second < right) buffer[index++] = values[second++];
  for (int current = left; current < right; ++current) values[current] = buffer[current];
}

void StringQuickSort(std::vector<std::string>& values,
                     int left,
                     int right,
                     int depth) {
  if (left >= right) return;
  int lower = left;
  int greater = right;
  int pivot = CharAt(values[left], depth);
  int index = left + 1;
  while (index <= greater) {
    int current = CharAt(values[index], depth);
    if (current < pivot) {
      std::swap(values[lower++], values[index++]);
    } else if (current > pivot) {
      std::swap(values[index], values[greater--]);
    } else {
      ++index;
    }
  }
  StringQuickSort(values, left, lower - 1, depth);
  if (pivot >= 0) StringQuickSort(values, lower, greater, depth + 1);
  StringQuickSort(values, greater + 1, right, depth);
}

void MsdRadixSort(std::vector<std::string>& values,
                  std::vector<std::string>& buffer,
                  int left,
                  int right,
                  int depth) {
  if (right - left <= 1) return;
  const int kRadix = 256;
  std::vector<int> count(kRadix + 2, 0);
  for (int index = left; index < right; ++index) ++count[CharAt(values[index], depth) + 2];
  for (int index = 1; index < static_cast<int>(count.size()); ++index) count[index] += count[index - 1];
  std::vector<int> start = count;
  for (int index = left; index < right; ++index) {
    int bucket = CharAt(values[index], depth) + 1;
    buffer[left + count[bucket]++] = values[index];
  }
  for (int index = left; index < right; ++index) values[index] = buffer[index];
  for (int bucket = 0; bucket < kRadix; ++bucket) {
    int begin = left + start[bucket + 1];
    int end = left + start[bucket + 2];
    if (end - begin > 1) MsdRadixSort(values, buffer, begin, end, depth + 1);
  }
}

void HybridSort(std::vector<std::string>& values,
                std::vector<std::string>& buffer,
                int left,
                int right,
                int depth) {
  const int kThreshold = 74;
  if (right - left <= 1) return;
  if (right - left < kThreshold) {
    StringQuickSort(values, left, right - 1, depth);
    return;
  }
  const int kRadix = 256;
  std::vector<int> count(kRadix + 2, 0);
  for (int index = left; index < right; ++index) ++count[CharAt(values[index], depth) + 2];
  for (int index = 1; index < static_cast<int>(count.size()); ++index) count[index] += count[index - 1];
  std::vector<int> start = count;
  for (int index = left; index < right; ++index) {
    int bucket = CharAt(values[index], depth) + 1;
    buffer[left + count[bucket]++] = values[index];
  }
  for (int index = left; index < right; ++index) values[index] = buffer[index];
  for (int bucket = 0; bucket < kRadix; ++bucket) {
    int begin = left + start[bucket + 1];
    int end = left + start[bucket + 2];
    if (end - begin > 1) HybridSort(values, buffer, begin, end, depth + 1);
  }
}

class StringSortTester {
 public:
  struct Result {
    std::string algorithm;
    std::string dataset;
    int size;
    double average_time_microseconds;
    double average_character_comparisons;
  };

  explicit StringSortTester(int repetitions = 10)
      : repetitions_(repetitions), generator_(1337) {}

  std::vector<Result> RunAllTests() {
    std::vector<Result> results;
    const std::vector<int> sizes = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900, 3000};

    int total_tests = static_cast<int>(sizes.size()) * 4 * 6;
    int current_test = 0;

    for (int size : sizes) {
      auto random_data = generator_.GenerateRandomDataset(size, 10, 200);
      auto reverse_data = generator_.GenerateReverseSortedDataset(size, 10, 200);
      auto nearly_sorted_data = generator_.GenerateNearlySortedDataset(size, 10, 200);
      auto prefix_heavy_data = generator_.GeneratePrefixHeavyDataset(size, 10, 200);

      const std::vector<std::pair<std::string, std::vector<std::string>>> datasets = {
          {"random", random_data},
          {"reverse_sorted", reverse_data},
          {"nearly_sorted", nearly_sorted_data},
          {"prefix_heavy", prefix_heavy_data}
      };

      for (const auto& dataset_entry : datasets) {
        results.push_back(Benchmark("quick_sort", dataset_entry.first, dataset_entry.second, &RunStandardQuickSort));
        std::cout << "\rProgress: " << ++current_test << "/" << total_tests << std::flush;

        results.push_back(Benchmark("merge_sort", dataset_entry.first, dataset_entry.second, &RunStandardMergeSort));
        std::cout << "\rProgress: " << ++current_test << "/" << total_tests << std::flush;

        results.push_back(Benchmark("string_quick_sort", dataset_entry.first, dataset_entry.second, &RunStringQuickSort));
        std::cout << "\rProgress: " << ++current_test << "/" << total_tests << std::flush;

        results.push_back(Benchmark("string_merge_sort", dataset_entry.first, dataset_entry.second, &RunStringMergeSort));
        std::cout << "\rProgress: " << ++current_test << "/" << total_tests << std::flush;

        results.push_back(Benchmark("msd_radix_sort", dataset_entry.first, dataset_entry.second, &RunMsdRadixSort));
        std::cout << "\rProgress: " << ++current_test << "/" << total_tests << std::flush;

        results.push_back(Benchmark("msd_radix_quick_sort", dataset_entry.first, dataset_entry.second, &RunHybridSort));
        std::cout << "\rProgress: " << ++current_test << "/" << total_tests << std::flush;
      }
    }
    std::cout << std::endl;
    return results;
  }

  void SaveCsv(const std::string& path, const std::vector<Result>& results) const {
    std::ofstream output(path);
    output << "algorithm,dataset,size,average_time_microseconds,average_character_comparisons\n";
    for (const Result& result : results) {
      output << result.algorithm << ',' << result.dataset << ',' << result.size << ',' << result.average_time_microseconds << ',' << result.average_character_comparisons << '\n';
    }
  }

 private:
  using SortFunction = void (*)(std::vector<std::string>&);
  static void RunStandardQuickSort(std::vector<std::string>& values) { if (!values.empty()) StandardQuickSort(values, 0, static_cast<int>(values.size()) - 1); }
  static void RunStandardMergeSort(std::vector<std::string>& values) { std::vector<std::string> buffer(values.size()); StandardMergeSort(values, buffer, 0, static_cast<int>(values.size())); }
  static void RunStringQuickSort(std::vector<std::string>& values) { if (!values.empty()) StringQuickSort(values, 0, static_cast<int>(values.size()) - 1, 0); }
  static void RunStringMergeSort(std::vector<std::string>& values) { std::vector<std::string> buffer(values.size()); StringMergeSort(values, buffer, 0, static_cast<int>(values.size())); }
  static void RunMsdRadixSort(std::vector<std::string>& values) { std::vector<std::string> buffer(values.size()); MsdRadixSort(values, buffer, 0, static_cast<int>(values.size()), 0); }
  static void RunHybridSort(std::vector<std::string>& values) { std::vector<std::string> buffer(values.size()); HybridSort(values, buffer, 0, static_cast<int>(values.size()), 0); }

  Result Benchmark(const std::string& algorithm, const std::string& dataset, const std::vector<std::string>& values, SortFunction sort_function) const {
    double total_time = 0.0;
    double total_comparisons = 0.0;
    for (int repetition = 0; repetition < repetitions_; ++repetition) {
      std::vector<std::string> copy = values;
      ComparisonCounter::Reset();
      const auto start = std::chrono::steady_clock::now();
      sort_function(copy);
      const auto finish = std::chrono::steady_clock::now();
      total_time += std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(finish - start).count();
      total_comparisons += static_cast<double>(ComparisonCounter::Get());
    }
    return Result{algorithm, dataset, static_cast<int>(values.size()), total_time / repetitions_, total_comparisons / repetitions_};
  }

  int repetitions_;
  StringGenerator generator_;
};

int main() {
  StringSortTester tester(10);
  auto results = tester.RunAllTests();
  tester.SaveCsv("results.csv", results);
  std::cout << "Saved results.csv" << std::endl;
  return 0;
}