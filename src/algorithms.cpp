#include "algorithms.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>

namespace turbocore {

int linearSearch(const std::vector<int>& values, int target) {
    for (std::size_t i = 0; i < values.size(); ++i)
        if (values[i] == target) return static_cast<int>(i);
    return -1;
}

int binarySearch(const std::vector<int>& values, int target) {
    if (!std::is_sorted(values.begin(), values.end()))
        throw std::invalid_argument("Binary Search requires input sorted in ascending order");
    std::size_t first = 0, last = values.size();
    while (first < last) {
        const std::size_t middle = first + (last - first) / 2;
        if (values[middle] < target) first = middle + 1;
        else last = middle;
    }
    return first < values.size() && values[first] == target ? static_cast<int>(first) : -1;
}

int jumpSearch(const std::vector<int>& values, int target) {
    if (!std::is_sorted(values.begin(), values.end()))
        throw std::invalid_argument("Jump Search requires input sorted in ascending order");
    const std::size_t n = values.size();
    if (n == 0) return -1;
    const std::size_t block = std::max<std::size_t>(1, static_cast<std::size_t>(std::sqrt(static_cast<double>(n))));
    std::size_t start = 0, end = std::min(block, n);
    while (start < n && values[end - 1] < target) {
        start = end;
        end = end > n - block ? n : end + block;
    }
    for (std::size_t i = start; i < end; ++i) {
        if (values[i] == target) return static_cast<int>(i);
        if (values[i] > target) break;
    }
    return -1;
}

std::string LinearSearchAlgorithm::name() const { return "Linear Search"; }
std::string LinearSearchAlgorithm::complexity() const { return "O(n)"; }
int LinearSearchAlgorithm::find(const std::vector<int>& values, int target) const { return linearSearch(values, target); }
std::string BinarySearchAlgorithm::name() const { return "Binary Search"; }
std::string BinarySearchAlgorithm::complexity() const { return "O(n) validation + O(log n) search"; }
int BinarySearchAlgorithm::find(const std::vector<int>& values, int target) const { return binarySearch(values, target); }
std::string JumpSearchAlgorithm::name() const { return "Jump Search"; }
std::string JumpSearchAlgorithm::complexity() const { return "O(n) validation + O(sqrt(n)) search"; }
int JumpSearchAlgorithm::find(const std::vector<int>& values, int target) const { return jumpSearch(values, target); }

std::string BubbleSort::name() const { return "Bubble Sort"; }
std::string BubbleSort::complexity() const { return "O(n^2)"; }
void BubbleSort::sort(std::vector<int>& values) const {
    for (std::size_t end = values.size(); end > 1; --end) {
        bool changed = false;
        for (std::size_t i = 1; i < end; ++i) {
            if (values[i - 1] > values[i]) {
                std::swap(values[i - 1], values[i]);
                changed = true;
            }
        }
        if (!changed) break;
    }
}

namespace {
void mergeRange(std::vector<int>& values, std::vector<int>& scratch,
                std::size_t first, std::size_t last) {
    if (last - first < 2) return;
    const std::size_t middle = first + (last - first) / 2;
    mergeRange(values, scratch, first, middle);
    mergeRange(values, scratch, middle, last);
    std::size_t left = first, right = middle, out = first;
    while (left < middle && right < last)
        scratch[out++] = values[left] <= values[right] ? values[left++] : values[right++];
    while (left < middle) scratch[out++] = values[left++];
    while (right < last) scratch[out++] = values[right++];
    for (std::size_t i = first; i < last; ++i) values[i] = scratch[i];
}
}

std::string MergeSort::name() const { return "Merge Sort"; }
std::string MergeSort::complexity() const { return "O(n log n)"; }
void MergeSort::sort(std::vector<int>& values) const {
    std::vector<int> scratch(values.size());
    mergeRange(values, scratch, 0, values.size());
}

std::string StdSort::name() const { return "std::sort"; }
std::string StdSort::complexity() const { return "O(n log n)"; }
void StdSort::sort(std::vector<int>& values) const {
    std::sort(values.begin(), values.end());
}

std::string InsertionSort::name() const { return "Insertion Sort"; }
std::string InsertionSort::complexity() const { return "O(n^2), O(n) best"; }
void InsertionSort::sort(std::vector<int>& values) const {
    for (std::size_t i = 1; i < values.size(); ++i) {
        const int value = values[i];
        std::size_t j = i;
        while (j > 0 && values[j - 1] > value) { values[j] = values[j - 1]; --j; }
        values[j] = value;
    }
}

namespace {
void quickSortRange(std::vector<int>& values, std::size_t first, std::size_t last) {
    while (last - first > 1) {
        const int a = values[first], b = values[first + (last - first) / 2], c = values[last - 1];
        const int pivot = std::max(std::min(a, b), std::min(std::max(a, b), c));
        std::size_t lower = first, current = first, upper = last;
        while (current < upper) {
            if (values[current] < pivot) std::swap(values[lower++], values[current++]);
            else if (values[current] > pivot) std::swap(values[current], values[--upper]);
            else ++current;
        }
        // Recurse into the smaller partition to keep stack use bounded; loop over the larger.
        if (lower - first < last - upper) {
            quickSortRange(values, first, lower);
            first = upper;
        } else {
            quickSortRange(values, upper, last);
            last = lower;
        }
    }
}
}

std::string QuickSort::name() const { return "Quick Sort"; }
std::string QuickSort::complexity() const { return "O(n log n) average, O(n^2) worst"; }
void QuickSort::sort(std::vector<int>& values) const { quickSortRange(values, 0, values.size()); }

std::string SelectionSort::name() const { return "Selection Sort"; }
std::string SelectionSort::complexity() const { return "O(n^2)"; }
void SelectionSort::sort(std::vector<int>& values) const {
    for (std::size_t i = 0; i < values.size(); ++i) {
        std::size_t smallest = i;
        for (std::size_t j = i + 1; j < values.size(); ++j)
            if (values[j] < values[smallest]) smallest = j;
        if (smallest != i) std::swap(values[i], values[smallest]);
    }
}

std::string BruteForceDuplicates::name() const { return "Duplicate brute force"; }
std::string BruteForceDuplicates::complexity() const { return "O(n^2)"; }
bool BruteForceDuplicates::containsDuplicate(const std::vector<int>& values) const {
    for (std::size_t i = 0; i < values.size(); ++i)
        for (std::size_t j = i + 1; j < values.size(); ++j)
            if (values[i] == values[j]) return true;
    return false;
}

std::string SortScanDuplicates::name() const { return "Duplicate sort-and-scan"; }
std::string SortScanDuplicates::complexity() const { return "O(n log n)"; }
bool SortScanDuplicates::containsDuplicate(const std::vector<int>& values) const {
    std::vector<int> copy(values);
    std::sort(copy.begin(), copy.end());
    for (std::size_t i = 1; i < copy.size(); ++i)
        if (copy[i - 1] == copy[i]) return true;
    return false;
}

std::string HashDuplicates::name() const { return "Duplicate unordered_set"; }
std::string HashDuplicates::complexity() const { return "O(n) average"; }
bool HashDuplicates::containsDuplicate(const std::vector<int>& values) const {
    std::unordered_set<int> seen;
    for (std::size_t i = 0; i < values.size(); ++i)
        if (!seen.insert(values[i]).second) return true;
    return false;
}

std::string MapFrequencyCounter::name() const { return "Frequency std::map"; }
std::string MapFrequencyCounter::complexity() const { return "O(n log k)"; }
std::vector<std::pair<int, std::size_t> > MapFrequencyCounter::count(const std::vector<int>& values) const {
    std::map<int, std::size_t> result;
    for (std::size_t i = 0; i < values.size(); ++i) ++result[values[i]];
    return std::vector<std::pair<int, std::size_t> >(result.begin(), result.end());
}

std::string UnorderedFrequencyCounter::name() const { return "Frequency std::unordered_map"; }
std::string UnorderedFrequencyCounter::complexity() const { return "O(n) average"; }
std::vector<std::pair<int, std::size_t> > UnorderedFrequencyCounter::count(const std::vector<int>& values) const {
    std::unordered_map<int, std::size_t> counts;
    for (std::size_t i = 0; i < values.size(); ++i) ++counts[values[i]];
    return std::vector<std::pair<int, std::size_t> >(counts.begin(), counts.end());
}

} // namespace turbocore
