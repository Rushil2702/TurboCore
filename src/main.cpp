#include "algorithms.hpp"
#include "big_integer.hpp"
#include "csv_report.hpp"
#include "data_structures.hpp"
#include "number_theory.hpp"
#include "timing.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <climits>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <cstdint>

namespace {
using namespace turbocore;
typedef std::chrono::steady_clock Clock;
void runMemoryExperiment(const std::string& reportPath, std::size_t count);

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("Test failed: " + message);
}

template <typename T>
struct Statistics {
    double median, minimum, maximum, mean;
};

template <typename T>
Statistics<T> summarize(std::vector<T> samples) {
    if (samples.empty()) throw std::invalid_argument("Cannot summarize no samples");
    std::sort(samples.begin(), samples.end());
    Statistics<T> result;
    result.minimum = static_cast<double>(samples.front());
    result.maximum = static_cast<double>(samples.back());
    result.median = static_cast<double>(samples[samples.size() / 2]);
    if (samples.size() % 2 == 0)
        result.median = (static_cast<double>(samples[samples.size()/2-1]) + result.median) / 2.0;
    double total = 0;
    for (std::size_t i = 0; i < samples.size(); ++i) total += static_cast<double>(samples[i]);
    result.mean = total / samples.size();
    return result;
}

std::string compilerInfo() {
#ifdef __VERSION__
    return std::string("GCC ") + __VERSION__;
#else
    return "Unknown compiler";
#endif
}

std::vector<int> makeInput(std::size_t size, const std::string& pattern, unsigned int seed) {
    std::vector<int> values;
    values.reserve(size);
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(-100000, 100000);
    for (std::size_t i = 0; i < size; ++i) {
        if (pattern == "sorted") values.push_back(static_cast<int>(i));
        else if (pattern == "reverse") values.push_back(static_cast<int>(size - i));
        else if (pattern == "duplicate-heavy") values.push_back(static_cast<int>(generator() % 16));
        else values.push_back(distribution(generator));
    }
    return values;
}

std::vector<std::pair<int, std::size_t> > normalizeCounts(std::vector<std::pair<int, std::size_t> > counts) {
    std::sort(counts.begin(), counts.end());
    return counts;
}

std::vector<int> parseIntList(const std::string& line) {
    std::istringstream input(line);
    std::vector<int> values;
    std::string token;
    while (input >> token) {
        std::istringstream number(token);
        long long parsed;
        char extra;
        if (!(number >> parsed) || (number >> extra) || parsed < INT_MIN || parsed > INT_MAX)
            throw std::invalid_argument("Enter only in-range 32-bit integers separated by spaces");
        values.push_back(static_cast<int>(parsed));
    }
    return values;
}

bool fileContains(const std::string& path, const std::string& text) {
    std::ifstream input(path.c_str());
    std::string line;
    while (std::getline(input, line)) if (line.find(text) != std::string::npos) return true;
    return false;
}

std::string algorithmCategory(const Algorithm& algorithm) {
    if (dynamic_cast<const SearchAlgorithm*>(&algorithm)) return "search";
    if (dynamic_cast<const SortAlgorithm*>(&algorithm)) return "sort";
    if (dynamic_cast<const DuplicateDetector*>(&algorithm)) return "duplicate detection";
    if (dynamic_cast<const FrequencyCounter*>(&algorithm)) return "frequency counting";
    return "other";
}

void showAlgorithmCatalog() {
    std::vector<std::unique_ptr<Algorithm> > algorithms;
    algorithms.emplace_back(new LinearSearchAlgorithm()); algorithms.emplace_back(new BinarySearchAlgorithm()); algorithms.emplace_back(new JumpSearchAlgorithm());
    algorithms.emplace_back(new BubbleSort()); algorithms.emplace_back(new InsertionSort()); algorithms.emplace_back(new MergeSort());
    algorithms.emplace_back(new QuickSort()); algorithms.emplace_back(new SelectionSort()); algorithms.emplace_back(new StdSort());
    algorithms.emplace_back(new BruteForceDuplicates()); algorithms.emplace_back(new SortScanDuplicates()); algorithms.emplace_back(new HashDuplicates());
    algorithms.emplace_back(new MapFrequencyCounter()); algorithms.emplace_back(new UnorderedFrequencyCounter());
    for (std::size_t i = 0; i < algorithms.size(); ++i)
        std::cout << algorithmCategory(*algorithms[i]) << ": " << algorithms[i]->name()
                  << " [" << algorithms[i]->complexity() << "]\n";
}

void testCsvReports() {
    const std::string base = "tests_csv_report_tmp.csv";
    const std::string versioned = "tests_csv_report_tmp_v2.csv";
    std::remove(base.c_str()); std::remove(versioned.c_str());
    {
        std::ofstream old(base.c_str());
        old << "old,schema\nkeep,this\n";
    }
    const std::string header = "name,value";
    require(compatibleReportPath(base, header) == versioned, "incompatible CSV preserves history with versioned path");
    {
        CsvReport report(versioned, header);
        report.writeRow(std::vector<std::string>{"comma,value", "quote\"value"});
    }
    {
        CsvReport report(versioned, header);
        report.writeRow(std::vector<std::string>{"second", "row"});
    }
    require(fileContains(versioned, "\"comma,value\",\"quote\"\"value\""), "CSV escaping and file output");
    require(fileContains(versioned, "second,row"), "CSV append behavior");
    require(fileContains(base, "keep,this"), "old incompatible history remains intact");
    bool wrongColumnsRejected = false;
    try {
        CsvReport report(versioned, header);
        report.writeRow(std::vector<std::string>{"one"});
    } catch (const std::invalid_argument&) { wrongColumnsRejected = true; }
    require(wrongColumnsRejected, "CSV rejects rows with a mismatched number of columns");
    std::remove(base.c_str()); std::remove(versioned.c_str());
}

void testDataStructureReport() {
    const std::string path = "tests_data_structures_tmp.csv";
    std::remove(path.c_str());
    std::remove("tests_data_structures_tmp_v2.csv");
    const std::string actualPath = compatibleReportPath(path, timing::csvHeader());
    runDataStructureLab(path);
    require(fileContains(actualPath, timing::csvHeader()),
            "data structure CSV header");
    require(fileContains(actualPath, "std::stack LIFO"), "data structure CSV row");
    std::remove(actualPath.c_str());
}

void testMemoryReport() {
    const std::string path = "tests_memory_tmp.csv";
    std::remove(path.c_str());
    const std::string header = "run_id,experiment,element_count,repetitions,batch_operations,median_ns_per_vector,min_ns_per_vector,max_ns_per_vector,mean_ns_per_vector,capacity,capacity_changes,payload_bytes,compiler,timing_note";
    const std::string actualPath = compatibleReportPath(path, header);
    runMemoryExperiment(path, 2000);
    require(fileContains(actualPath, header), "memory report header");
    require(fileContains(actualPath, "vector append without reserve") &&
            fileContains(actualPath, "vector reserve allocation") &&
            fileContains(actualPath, "vector append after reserve"), "memory report separates reserve and append rows");
    std::remove(actualPath.c_str());
}

void runTests() {
    const std::vector<int> unsorted{7, -2, 4, 4, 0, 99, -10};
    require(linearSearch(unsorted, 4) == 2, "linear search hit");
    require(linearSearch(unsorted, 8) == -1, "linear search miss");
    const std::vector<int> sorted{-10, -2, 0, 4, 4, 7, 99};
    require(binarySearch(sorted, 4) == 3, "binary search first duplicate");
    require(binarySearch(sorted, 8) == -1, "binary search miss");
    require(jumpSearch(sorted, 4) == 3 && jumpSearch(sorted, 8) == -1, "jump search hit and miss");
    bool unsortedRejected = false;
    try { binarySearch(unsorted, 4); } catch (const std::invalid_argument&) { unsortedRejected = true; }
    require(unsortedRejected, "binary search rejects unsorted input");
    bool jumpRejected = false;
    try { jumpSearch(unsorted, 4); } catch (const std::invalid_argument&) { jumpRejected = true; }
    require(jumpRejected, "jump search rejects unsorted input");
    require(parseIntList(" -2147483648 0 2147483647 ") == std::vector<int>({INT_MIN, 0, INT_MAX}), "integer-list parsing");
    const std::string invalidLists[] = {"1 nope", "2147483648", "-2147483649", "12x", "9223372036854775808"};
    for (std::size_t i = 0; i < sizeof(invalidLists)/sizeof(invalidLists[0]); ++i) {
        bool rejected = false;
        try { parseIntList(invalidLists[i]); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "invalid and out-of-range integer-list input rejected");
    }
    std::vector<std::unique_ptr<SearchAlgorithm> > searches;
    searches.emplace_back(new LinearSearchAlgorithm()); searches.emplace_back(new BinarySearchAlgorithm()); searches.emplace_back(new JumpSearchAlgorithm());
    require(searches[0]->find(unsorted, 99) == 5 && searches[1]->find(sorted, 99) == 6 && searches[2]->find(sorted, 99) == 6, "polymorphic search");
    require(algorithmCategory(*searches[0]) == "search", "RTTI algorithm category");

    std::vector<std::unique_ptr<SortAlgorithm> > sorters;
    sorters.emplace_back(new BubbleSort()); sorters.emplace_back(new MergeSort()); sorters.emplace_back(new StdSort());
    sorters.emplace_back(new InsertionSort()); sorters.emplace_back(new QuickSort()); sorters.emplace_back(new SelectionSort());
    std::vector<std::vector<int> > sortCases{std::vector<int>(), std::vector<int>{1}, unsorted,
    std::vector<int>{3, 3, 3}, std::vector<int>{-1, 0, 1}, std::vector<int>{9, 8, 7, 6, 5, 4, 3, 2, 1}};
    std::vector<int> randomValues;
    std::mt19937 sortGenerator(556677u);
    for (int i = 0; i < 200; ++i) randomValues.push_back(static_cast<int>(sortGenerator() % 43) - 21);
    sortCases.push_back(randomValues);
    for (std::size_t a = 0; a < sorters.size(); ++a)
        for (std::size_t c = 0; c < sortCases.size(); ++c) {
            std::vector<int> expected = sortCases[c], actual = sortCases[c];
            std::sort(expected.begin(), expected.end()); sorters[a]->sort(actual);
            require(actual == expected, sorters[a]->name() + " edge case");
        }

    const Statistics<int> evenStats = summarize(std::vector<int>{8, 2, 4, 6});
    require(evenStats.median == 5 && evenStats.minimum == 2 && evenStats.maximum == 8 && evenStats.mean == 5, "benchmark statistics aggregation");
    const timing::Summary preciseStats = timing::summarize(std::vector<double>{1.25, 2.5, 3.75, 5.0}, 16);
    require(preciseStats.median == 3.125 && preciseStats.minimum == 1.25 && preciseStats.maximum == 5.0 &&
            preciseStats.mean == 3.125 && preciseStats.batchOperations == 16 && !preciseStats.belowResolution,
            "batched nanosecond statistics preserve fractional precision");
    require(timing::number(0.000001) == "0.000001", "timing formatter preserves sub-nanosecond precision");
    const timing::Summary unresolvedStats = timing::summarize(std::vector<double>{0.0, 0.0}, 128, true);
    require(unresolvedStats.median == 0.0 && unresolvedStats.minimum == 0.0 && unresolvedStats.maximum == 0.0 &&
            unresolvedStats.mean == 0.0 && unresolvedStats.belowResolution,
            "below-resolution samples remain zero and are explicitly flagged");

    std::vector<std::unique_ptr<DuplicateDetector> > detectors;
    detectors.emplace_back(new BruteForceDuplicates()); detectors.emplace_back(new SortScanDuplicates()); detectors.emplace_back(new HashDuplicates());
    for (std::size_t i = 0; i < detectors.size(); ++i) {
        require(detectors[i]->containsDuplicate(std::vector<int>{1, 2, 1}), "duplicate found");
        require(!detectors[i]->containsDuplicate(std::vector<int>{-1, 0, 2}), "unique values accepted");
        require(!detectors[i]->containsDuplicate(std::vector<int>()), "empty values accepted");
    }
    MapFrequencyCounter mapCounter; UnorderedFrequencyCounter hashCounter;
    require(normalizeCounts(mapCounter.count(unsorted)) == normalizeCounts(hashCounter.count(unsorted)), "frequency implementations agree");
    const std::vector<std::pair<int, std::size_t> > frequencies = mapCounter.count(unsorted);
    require(std::find(frequencies.begin(), frequencies.end(), std::make_pair(4, static_cast<std::size_t>(2))) != frequencies.end(), "frequency count value");

    require(BigInteger("000123").toString() == "123", "leading zeros");
    require(BigInteger("-000").toString() == "0", "negative zero normalization");
    require(BigInteger("+0009").toString() == "9", "explicit positive sign");
    require(BigInteger(-42).toString() == "-42", "numeric constructor overload");
    require(BigInteger((std::numeric_limits<long long>::min)()).toString() == "-9223372036854775808", "minimum built-in signed value");
    const std::string fiftyNines(50, '9');
    require((BigInteger(fiftyNines) * BigInteger("9")).toString() == "8" + std::string(49, '9') + "1", "very large multiplication");
    require((BigInteger("99999999999999999999") + BigInteger("1")).toString() == "100000000000000000000", "long carry chain");
    require((BigInteger("-12") + BigInteger("5")).toString() == "-7", "mixed-sign addition");
    require((BigInteger("-12") + BigInteger("20")).toString() == "8", "opposite-sign addition");
    require((BigInteger("12") + BigInteger("-5")).toString() == "7", "positive plus negative");
    require((BigInteger("0") + BigInteger("-5")).toString() == "-5", "zero plus negative");
    require((BigInteger("5") - BigInteger("12")).toString() == "-7", "negative subtraction");
    require((BigInteger("-5") - BigInteger("12")).toString() == "-17", "negative minus positive");
    require((BigInteger("100000000000000000000") - BigInteger("1")).toString() == "99999999999999999999", "long borrow chain");
    require((BigInteger("-12345678901234567890") * BigInteger("9")).toString() == "-111111110111111111010", "large multiplication sign");
    require((BigInteger("-123456789") * BigInteger("-987654321")).toString() == "121932631112635269", "negative multiplication");
    require((BigInteger("0") * BigInteger("-999")).toString() == "0", "zero multiplication");
    require(BigInteger("00042") == BigInteger("42") && BigInteger("-4") != BigInteger("4"), "equality operators");
    require(BigInteger("-10") < BigInteger("-2") && BigInteger("5") > BigInteger("-3"), "ordering operators");
    std::ostringstream bigIntegerOutput;
    bigIntegerOutput << BigInteger("-12345678901234567890");
    require(bigIntegerOutput.str() == "-12345678901234567890", "friend stream insertion");
    require(number_theory::gcd(48, 18) == 6 && number_theory::gcd(0, 15) == 15, "GCD");
    require(number_theory::factorial(0).toString() == "1" && number_theory::factorial(20).toString() == "2432902008176640000", "BigInteger factorial");
    require(number_theory::isPrime(2) && number_theory::isPrime(97) && !number_theory::isPrime(1) && !number_theory::isPrime(100), "prime checking");
    bool factorialLimitRejected = false;
    try { number_theory::factorial(1001); } catch (const std::out_of_range&) { factorialLimitRejected = true; }
    require(factorialLimitRejected, "factorial work limit");
    testDataStructures();
    const Statistics<int> sampleStats = summarize(std::vector<int>{1, 9, 3});
    require(sampleStats.median == 3 && sampleStats.minimum == 1 && sampleStats.maximum == 9, "generic statistics helper");
    const std::string invalidInputs[] = {"", "+", "-", "1x", " 1", "--1"};
    for (std::size_t i = 0; i < sizeof(invalidInputs)/sizeof(invalidInputs[0]); ++i) {
        bool rejected = false;
        try { BigInteger invalid(invalidInputs[i]); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "invalid BigInteger input rejected");
    }
    testCsvReports();
    testDataStructureReport();
    testMemoryReport();
    std::cout << "All tests passed.\n";
}

void appendTimingRow(CsvReport& report, const std::string& runId, const std::string& experiment,
                     const std::string& name, const std::string& complexity, std::size_t size,
                     const std::string& workload, const timing::Summary& stats, int repetitions) {
    const std::string note = stats.belowResolution
        ? "one or more total batches were below clock resolution; values are measured ns per operation"
        : "measured ns per operation; inputs prepared before timing; correctness checked after timing";
    report.writeRow(std::vector<std::string>{runId, experiment, name, complexity,
        std::to_string(size), workload, compilerInfo(), std::to_string(repetitions),
        std::to_string(stats.batchOperations), timing::number(stats.median),
        timing::number(stats.minimum), timing::number(stats.maximum), timing::number(stats.mean), note});
}

std::size_t cappedBatch(std::size_t operations, std::size_t cap) {
    return std::max<std::size_t>(1, std::min(operations, cap));
}

void runBenchmark() {
    const int repetitions = 7;
    const std::size_t sizes[] = {256, 1024, 2048};
    const std::string patterns[] = {"random", "sorted", "reverse", "duplicate-heavy"};
    volatile unsigned long long benchmarkSink = 0;
    std::vector<std::unique_ptr<SortAlgorithm> > sorters;
    sorters.emplace_back(new BubbleSort()); sorters.emplace_back(new MergeSort()); sorters.emplace_back(new StdSort());
    sorters.emplace_back(new InsertionSort()); sorters.emplace_back(new QuickSort()); sorters.emplace_back(new SelectionSort());
    std::vector<std::unique_ptr<DuplicateDetector> > detectors;
    detectors.emplace_back(new BruteForceDuplicates()); detectors.emplace_back(new SortScanDuplicates()); detectors.emplace_back(new HashDuplicates());
    std::vector<std::unique_ptr<FrequencyCounter> > counters;
    counters.emplace_back(new MapFrequencyCounter()); counters.emplace_back(new UnorderedFrequencyCounter());
    std::vector<std::unique_ptr<SearchAlgorithm> > searches;
    searches.emplace_back(new LinearSearchAlgorithm()); searches.emplace_back(new BinarySearchAlgorithm()); searches.emplace_back(new JumpSearchAlgorithm());

    const std::string header = timing::csvHeader();
    const std::string reportPath = compatibleReportPath("benchmark_results.csv", header);
    CsvReport file(reportPath, header);
    const std::string runId = timing::newRunId();

    for (std::size_t si = 0; si < sizeof(sizes)/sizeof(sizes[0]); ++si)
        for (std::size_t pi = 0; pi < sizeof(patterns)/sizeof(patterns[0]); ++pi) {
            const std::vector<int> input = makeInput(sizes[si], patterns[pi], 20261021u + static_cast<unsigned int>(si * 13 + pi));
            std::vector<int> expected(input); std::sort(expected.begin(), expected.end());
            const int target = input.empty() ? 0 : input[input.size() / 2];
            for (std::size_t a = 0; a < searches.size(); ++a) {
                // All search implementations receive the same prepared ascending vector.
                const std::vector<int>& searchInput = expected;
                const std::vector<int>::const_iterator found = std::find(searchInput.begin(), searchInput.end(), target);
                const int expectedIndex = found == searchInput.end() ? -1 : static_cast<int>(found - searchInput.begin());
                if (searches[a]->find(searchInput, target) != expectedIndex) throw std::runtime_error("Search correctness failed");
                const std::size_t maxBatch = cappedBatch(8192, 2000000 / std::max<std::size_t>(1, input.size()));
                const timing::Summary stats = timing::measureAdaptive([&](std::size_t batch) {
                    std::vector<int> results(batch);
                    const Clock::time_point start = Clock::now();
                    for (std::size_t i = 0; i < batch; ++i) results[i] = searches[a]->find(searchInput, target);
                    const Clock::time_point stop = Clock::now();
                    unsigned long long checksum = 0;
                    for (std::size_t i = 0; i < batch; ++i) {
                        if (results[i] != expectedIndex) throw std::runtime_error("Search result changed during measurement");
                        checksum += static_cast<unsigned int>(results[i] + 1);
                    }
                    benchmarkSink += checksum;
                    return timing::elapsedNanoseconds(start, stop);
                }, repetitions, maxBatch);
                appendTimingRow(file, runId, "algorithm benchmark", searches[a]->name(), searches[a]->complexity(),
                                input.size(), patterns[pi] + "; sorted search view", stats, repetitions);
            }
            std::string fastestName;
            double fastestMedian = 0.0;
            for (std::size_t a = 0; a < sorters.size(); ++a) {
                std::vector<int> check(input); sorters[a]->sort(check);
                if (check != expected) throw std::runtime_error(sorters[a]->name() + " correctness check failed");
                const bool quadratic = sorters[a]->name() == "Bubble Sort" ||
                    sorters[a]->name() == "Insertion Sort" || sorters[a]->name() == "Selection Sort";
                const std::size_t nSquared = input.size() * input.size();
                std::size_t maxBatch = quadratic
                    ? cappedBatch(128, 8000000 / std::max<std::size_t>(1, nSquared)) : 128;
                if (patterns[pi] == "sorted" && sorters[a]->name() != "Selection Sort") maxBatch = 128;
                const timing::Summary actual = timing::measureAdaptive([&](std::size_t batch) {
                    std::vector<std::vector<int> > copies(batch, input);
                    const Clock::time_point start = Clock::now();
                    for (std::size_t i = 0; i < batch; ++i) sorters[a]->sort(copies[i]);
                    const Clock::time_point stop = Clock::now();
                    unsigned long long checksum = 0;
                    for (std::size_t i = 0; i < batch; ++i) {
                        if (copies[i] != expected) throw std::runtime_error("Sort output changed during measurement");
                        if (!copies[i].empty()) checksum += static_cast<unsigned int>(copies[i][i % copies[i].size()]);
                    }
                    benchmarkSink += checksum;
                    return timing::elapsedNanoseconds(start, stop);
                }, repetitions, maxBatch);
                appendTimingRow(file, runId, "algorithm benchmark", sorters[a]->name(), sorters[a]->complexity(),
                                input.size(), patterns[pi], actual, repetitions);
                if (fastestName.empty() || actual.median < fastestMedian) {
                    fastestName = sorters[a]->name(); fastestMedian = actual.median;
                }
                std::cout << sorters[a]->name() << " n=" << input.size() << " " << patterns[pi]
                            << " batch=" << actual.batchOperations << " median=" << timing::number(actual.median)
                            << " ns/op, min=" << timing::number(actual.minimum) << ", max="
                            << timing::number(actual.maximum) << ", mean=" << timing::number(actual.mean) << "\n";
                if (a + 1 == sorters.size())
                    std::cout << "  Fastest sort for this input: " << fastestName << " (median " << timing::number(fastestMedian) << " ns/op)\n";
            }
            bool expectedDuplicate = false;
            for (std::size_t i = 0; i < input.size() && !expectedDuplicate; ++i)
                for (std::size_t j = i + 1; j < input.size(); ++j) if (input[i] == input[j]) { expectedDuplicate = true; break; }
            for (std::size_t a = 0; a < detectors.size(); ++a) {
                if (detectors[a]->containsDuplicate(input) != expectedDuplicate) throw std::runtime_error("Duplicate detection correctness failed");
                const bool expectedResult = expectedDuplicate;
                const std::size_t maxBatch = detectors[a]->name() == "Duplicate brute force"
                    ? cappedBatch(64, 1000000 / std::max<std::size_t>(1, input.size() * input.size())) : 128;
                const timing::Summary stats = timing::measureAdaptive([&](std::size_t batch) {
                    std::vector<unsigned char> results(batch);
                    const Clock::time_point start = Clock::now();
                    for (std::size_t i = 0; i < batch; ++i)
                        results[i] = detectors[a]->containsDuplicate(input) ? 1 : 0;
                    const Clock::time_point stop = Clock::now();
                    unsigned long long checksum = 0;
                    for (std::size_t i = 0; i < batch; ++i) {
                        if ((results[i] != 0) != expectedResult) throw std::runtime_error("Duplicate check failed during measurement");
                        checksum += results[i];
                    }
                    benchmarkSink += checksum;
                    return timing::elapsedNanoseconds(start, stop);
                }, repetitions, maxBatch);
                appendTimingRow(file, runId, "algorithm benchmark", detectors[a]->name(), detectors[a]->complexity(),
                                input.size(), patterns[pi], stats, repetitions);
            }
            const std::vector<std::pair<int, std::size_t> > expectedCounts = MapFrequencyCounter().count(input);
            for (std::size_t a = 0; a < counters.size(); ++a) {
                if (normalizeCounts(counters[a]->count(input)) != normalizeCounts(expectedCounts)) throw std::runtime_error("Frequency correctness failed");
                const std::size_t maxBatch = cappedBatch(128, 200000 / std::max<std::size_t>(1, input.size()));
                const timing::Summary stats = timing::measureAdaptive([&](std::size_t batch) {
                    std::vector<std::vector<std::pair<int, std::size_t> > > results(batch);
                    const Clock::time_point start = Clock::now();
                    for (std::size_t i = 0; i < batch; ++i) results[i] = counters[a]->count(input);
                    const Clock::time_point stop = Clock::now();
                    unsigned long long checksum = 0;
                    for (std::size_t i = 0; i < batch; ++i) {
                        if (normalizeCounts(results[i]) != normalizeCounts(expectedCounts))
                            throw std::runtime_error("Frequency failed during measurement");
                        for (std::size_t j = 0; j < results[i].size(); ++j) checksum += results[i][j].second;
                    }
                    benchmarkSink += checksum;
                    return timing::elapsedNanoseconds(start, stop);
                }, repetitions, maxBatch);
                appendTimingRow(file, runId, "algorithm benchmark", counters[a]->name(), counters[a]->complexity(),
                                input.size(), patterns[pi], stats, repetitions);
            }
        }
    std::cout << "Benchmark run " << runId << " appended to " << reportPath
              << " (compatible schema; prior rows preserved).\n";
    file.flush();
    (void)benchmarkSink;
}

void runMemoryExperiment(const std::string& reportPath, std::size_t count) {
    if (count == 0) throw std::invalid_argument("Memory experiment count must be positive");
    const std::size_t bytesPerVector = count * sizeof(int);
    const std::size_t maxBatch = cappedBatch(128, (64u * 1024u * 1024u) / std::max<std::size_t>(1, bytesPerVector));
    const int repetitions = 7;
    const std::string runId = timing::newRunId();
    volatile unsigned long long memorySink = 0;

    const timing::Summary growth = timing::measureAdaptive([&](std::size_t batch) {
        std::vector<std::vector<int> > values(batch);
        const Clock::time_point start = Clock::now();
        for (std::size_t b = 0; b < batch; ++b)
            for (std::size_t i = 0; i < count; ++i) values[b].push_back(static_cast<int>(i));
        const Clock::time_point stop = Clock::now();
        unsigned long long checksum = 0;
        for (std::size_t b = 0; b < batch; ++b) {
            if (values[b].size() != count || values[b].back() != static_cast<int>(count - 1))
                throw std::runtime_error("Vector growth workload correctness failure");
            checksum += values[b].back();
        }
        memorySink += checksum;
        return timing::elapsedNanoseconds(start, stop);
    }, repetitions, maxBatch);

    const timing::Summary reserveTiming = timing::measureAdaptive([&](std::size_t batch) {
        std::vector<std::vector<int> > values(batch);
        const Clock::time_point start = Clock::now();
        for (std::size_t b = 0; b < batch; ++b) values[b].reserve(count);
        const Clock::time_point stop = Clock::now();
        unsigned long long checksum = 0;
        for (std::size_t b = 0; b < batch; ++b) {
            if (values[b].capacity() < count) throw std::runtime_error("vector::reserve failed to provide requested capacity");
            checksum += values[b].capacity();
        }
        memorySink += checksum;
        return timing::elapsedNanoseconds(start, stop);
    }, repetitions, maxBatch);

    const timing::Summary reservedAppend = timing::measureAdaptive([&](std::size_t batch) {
        std::vector<std::vector<int> > values(batch);
        for (std::size_t b = 0; b < batch; ++b) values[b].reserve(count);
        const Clock::time_point start = Clock::now();
        for (std::size_t b = 0; b < batch; ++b)
            for (std::size_t i = 0; i < count; ++i) values[b].push_back(static_cast<int>(i));
        const Clock::time_point stop = Clock::now();
        unsigned long long checksum = 0;
        for (std::size_t b = 0; b < batch; ++b) {
            if (values[b].size() != count || values[b].back() != static_cast<int>(count - 1))
                throw std::runtime_error("Reserved vector workload correctness failure");
            checksum += values[b].back();
        }
        memorySink += checksum;
        return timing::elapsedNanoseconds(start, stop);
    }, repetitions, maxBatch);
    (void)memorySink;

    std::size_t growthChanges = 0, previousCapacity = 0;
    std::vector<int> growthProbe;
    for (std::size_t i = 0; i < count; ++i) {
        growthProbe.push_back(static_cast<int>(i));
        if (growthProbe.capacity() != previousCapacity) { ++growthChanges; previousCapacity = growthProbe.capacity(); }
    }
    std::vector<int> reserveProbe;
    reserveProbe.reserve(count);
    const std::size_t reservedCapacity = reserveProbe.capacity();
    const std::string header = "run_id,experiment,element_count,repetitions,batch_operations,median_ns_per_vector,min_ns_per_vector,max_ns_per_vector,mean_ns_per_vector,capacity,capacity_changes,payload_bytes,compiler,timing_note";
    const std::string actualPath = compatibleReportPath(reportPath, header);
    CsvReport csv(actualPath, header);
    const std::string estimate = "payload estimate is capacity*sizeof(int), excludes allocator overhead; each value is ns per vector operation";
    const std::string growthNote = estimate + (growth.belowResolution ? "; below clock resolution in some batches" : "");
    const std::string reserveNote = estimate + (reserveTiming.belowResolution ? "; below clock resolution in some batches" : "");
    const std::string appendNote = estimate + (reservedAppend.belowResolution ? "; below clock resolution in some batches" : "");
    const std::size_t reserveChanges = reservedCapacity > 0 ? 1 : 0;
    const auto writeMemoryRow = [&](const std::string& name, const timing::Summary& stats,
                                    std::size_t capacity, std::size_t changes, const std::string& note) {
        csv.writeRow(std::vector<std::string>{runId, name, std::to_string(count), std::to_string(repetitions),
            std::to_string(stats.batchOperations), timing::number(stats.median), timing::number(stats.minimum),
            timing::number(stats.maximum), timing::number(stats.mean), std::to_string(capacity),
            std::to_string(changes), std::to_string(capacity * sizeof(int)), compilerInfo(), note});
    };
    writeMemoryRow("vector append without reserve", growth, growthProbe.capacity(), growthChanges, growthNote);
    writeMemoryRow("vector reserve allocation", reserveTiming, reservedCapacity, reserveChanges, reserveNote);
    writeMemoryRow("vector append after reserve", reservedAppend, reservedCapacity, 0, appendNote);
    csv.flush();
    std::cout << "Memory run " << runId << ": median ns/vector (" << count << " appends) growth="
              << timing::number(growth.median) << ", reserve() allocation=" << timing::number(reserveTiming.median)
              << ", append after reserve=" << timing::number(reservedAppend.median)
              << ". Capacity payload estimates exclude allocator overhead. Report appended to " << actualPath << ".\n";
}

void runMemoryExperiment() { runMemoryExperiment("memory_results.csv", 100000); }

bool showReportSeries(const std::string& basePath) {
    bool found = false;
    const std::string::size_type dot = basePath.find_last_of('.');
    const std::string stem = dot == std::string::npos ? basePath : basePath.substr(0, dot);
    const std::string extension = dot == std::string::npos ? "" : basePath.substr(dot);
    for (unsigned int version = 1; version < 1000; ++version) {
        std::ostringstream path;
        path << stem;
        if (version > 1) path << "_v" << version;
        path << extension;
        std::ifstream file(path.str().c_str());
        if (!file) {
            if (version == 1) continue;
            break;
        }
        found = true;
        std::cout << "--- " << path.str() << " ---\n";
        std::string line; while (std::getline(file, line)) std::cout << line << '\n';
    }
    return found;
}

void showHistory() {
    const bool algorithmReports = showReportSeries("benchmark_results.csv");
    const bool structureReports = showReportSeries("data_structure_results.csv");
    const bool memoryReports = showReportSeries("memory_results.csv");
    if (!algorithmReports && !structureReports && !memoryReports) std::cout << "No benchmark history found.\n";
}

std::vector<int> readValues() {
    std::cout << "Enter space-separated integers: ";
    std::string line;
    if (!std::getline(std::cin, line)) throw std::runtime_error("Input ended unexpectedly");
    return parseIntList(line);
}

int readSingleInt(const std::string& prompt) {
    std::cout << prompt;
    std::string line, extra;
    if (!std::getline(std::cin, line)) throw std::runtime_error("Input ended unexpectedly");
    std::istringstream input(line);
    int value;
    if (!(input >> value) || (input >> extra)) throw std::invalid_argument("Please enter one valid 32-bit integer");
    return value;
}

void runSearchDemo() {
    const std::vector<int> values = readValues();
    const int target = readSingleInt("Target: ");
    std::cout << "1. Linear Search  2. Binary Search  3. Jump Search (sorted input required)\nChoice: ";
    std::string choice;
    if (!std::getline(std::cin, choice)) throw std::runtime_error("Input ended unexpectedly");
    std::unique_ptr<SearchAlgorithm> algorithm;
    if (choice == "1") algorithm.reset(new LinearSearchAlgorithm());
    else if (choice == "2") algorithm.reset(new BinarySearchAlgorithm());
    else if (choice == "3") algorithm.reset(new JumpSearchAlgorithm());
    else throw std::invalid_argument("Choose 1, 2, or 3");
    const int index = algorithm->find(values, target);
    if (index < 0) std::cout << "Value not found.\n";
    else std::cout << algorithm->name() << " found the value at index " << index << ".\n";
}

void runBigIntegerDemo() {
    std::string left, operation, right;
    std::cout << "First integer: "; if (!std::getline(std::cin, left)) throw std::runtime_error("Input ended unexpectedly");
    std::cout << "Operation (+, -, *): "; if (!std::getline(std::cin, operation)) throw std::runtime_error("Input ended unexpectedly");
    std::cout << "Second integer: "; if (!std::getline(std::cin, right)) throw std::runtime_error("Input ended unexpectedly");
    const BigInteger a(left), b(right);
    if (operation == "+") std::cout << "Result: " << (a + b).toString() << '\n';
    else if (operation == "-") std::cout << "Result: " << (a - b).toString() << '\n';
    else if (operation == "*") std::cout << "Result: " << (a * b).toString() << '\n';
    else throw std::invalid_argument("Operation must be +, -, or *");
}

std::uint64_t parseUnsigned(const std::string& text, std::uint64_t maximum) {
    if (text.empty() || text[0] == '-') throw std::invalid_argument("Expected a non-negative integer");
    std::istringstream input(text);
    unsigned long long value;
    char extra;
    if (!(input >> value) || (input >> extra) || value > maximum)
        throw std::invalid_argument("Integer argument is invalid or out of range");
    return static_cast<std::uint64_t>(value);
}

void runNumberTheory(const std::string& operation, const std::string& first, const std::string& second = "") {
    if (operation == "gcd") {
        const std::uint64_t a = parseUnsigned(first, (std::numeric_limits<std::uint64_t>::max)());
        const std::uint64_t b = parseUnsigned(second, (std::numeric_limits<std::uint64_t>::max)());
        std::cout << number_theory::gcd(a, b) << '\n';
    } else if (operation == "factorial") {
        const std::uint64_t n = parseUnsigned(first, 1000);
        std::cout << number_theory::factorial(static_cast<unsigned int>(n)) << '\n';
    } else if (operation == "prime") {
        const std::uint64_t n = parseUnsigned(first, (std::numeric_limits<std::uint32_t>::max)());
        std::cout << (number_theory::isPrime(static_cast<std::uint32_t>(n)) ? "prime" : "not prime") << '\n';
    } else throw std::invalid_argument("Use gcd, factorial, or prime");
}

void interactiveMenu() {
    for (;;) {
        std::cout << "\nTurboCore\n1. Tests\n2. Benchmark\n3. Memory experiment\n4. Show benchmark history\n"
                        "5. Search demo\n6. BigInteger calculator\n7. Number theory\n8. Data structure lab\n9. Algorithm catalog\n0. Exit\nChoice: ";
        std::string choice;
        if (!std::getline(std::cin, choice)) return;
        if (choice == "0") return;
        try {
            if (choice == "1") runTests();
            else if (choice == "2") runBenchmark();
            else if (choice == "3") runMemoryExperiment();
            else if (choice == "4") showHistory();
            else if (choice == "5") runSearchDemo();
            else if (choice == "6") runBigIntegerDemo();
            else if (choice == "7") {
                std::string operation, first, second;
                std::cout << "Operation (gcd/factorial/prime): "; std::getline(std::cin, operation);
                std::cout << "First value: "; std::getline(std::cin, first);
                if (operation == "gcd") { std::cout << "Second value: "; std::getline(std::cin, second); }
                runNumberTheory(operation, first, second);
            }
            else if (choice == "8") runDataStructureLab("data_structure_results.csv");
            else if (choice == "9") showAlgorithmCatalog();
            else std::cout << "Invalid choice. Enter a number from 0 to 9.\n";
        } catch (const std::exception& error) { std::cerr << "Error: " << error.what() << '\n'; }
    }
}
}

int main(int argc, char* argv[]) {
    try {
        if (argc == 1) { interactiveMenu(); return 0; }
        const std::string command = argv[1];
        if (command == "--test") runTests();
        else if (command == "--benchmark") runBenchmark();
        else if (command == "--memory") runMemoryExperiment();
        else if (command == "--history") showHistory();
        else if (command == "--structures") runDataStructureLab("data_structure_results.csv");
        else if (command == "--algorithms") showAlgorithmCatalog();
        else if (command == "--number-theory" && argc >= 3) {
            const std::string operation = argv[2];
            if ((operation == "gcd" && argc == 5) || ((operation == "prime" || operation == "factorial") && argc == 4))
                runNumberTheory(operation, argv[3], argc == 5 ? argv[4] : "");
            else throw std::invalid_argument("Usage: --number-theory gcd A B | factorial N | prime N");
        }
        else if (command == "--help") std::cout << "Usage: TurboCore.exe [--test|--benchmark|--structures|--algorithms|--memory|--history|--number-theory gcd A B|factorial N|prime N|--help]\nNo arguments opens the interactive menu.\n";
        else { std::cerr << "Unknown command: " << command << "\n"; return 2; }
    } catch (const std::exception& error) { std::cerr << "Error: " << error.what() << '\n'; return 1; }
    return 0;
}
