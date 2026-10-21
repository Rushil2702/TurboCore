#pragma once

#include <string>
#include <map>
#include <utility>
#include <vector>

namespace turbocore {

class Algorithm {
public:
    virtual ~Algorithm() {}
    virtual std::string name() const = 0;
    virtual std::string complexity() const = 0;
};

int linearSearch(const std::vector<int>& values, int target);
int binarySearch(const std::vector<int>& sortedValues, int target);
int jumpSearch(const std::vector<int>& sortedValues, int target);

class SearchAlgorithm : public Algorithm {
public:
    virtual int find(const std::vector<int>& values, int target) const = 0;
};

class LinearSearchAlgorithm : public SearchAlgorithm {
public:
    std::string name() const override;
    std::string complexity() const override;
    int find(const std::vector<int>& values, int target) const override;
};

class BinarySearchAlgorithm : public SearchAlgorithm {
public:
    std::string name() const override;
    std::string complexity() const override;
    int find(const std::vector<int>& values, int target) const override;
};

class JumpSearchAlgorithm : public SearchAlgorithm {
public:
    std::string name() const override;
    std::string complexity() const override;
    int find(const std::vector<int>& values, int target) const override;
};

class SortAlgorithm : public Algorithm {
public:
    virtual void sort(std::vector<int>& values) const = 0;
};

class BubbleSort : public SortAlgorithm {
public:
    std::string name() const override;
    std::string complexity() const override;
    void sort(std::vector<int>& values) const override;
};

class MergeSort : public SortAlgorithm {
public:
    std::string name() const override;
    std::string complexity() const override;
    void sort(std::vector<int>& values) const override;
};

class StdSort : public SortAlgorithm {
public:
    std::string name() const override;
    std::string complexity() const override;
    void sort(std::vector<int>& values) const override;
};

class InsertionSort : public SortAlgorithm {
public:
    std::string name() const override;
    std::string complexity() const override;
    void sort(std::vector<int>& values) const override;
};

class QuickSort : public SortAlgorithm {
public:
    std::string name() const override;
    std::string complexity() const override;
    void sort(std::vector<int>& values) const override;
};

class SelectionSort : public SortAlgorithm {
public:
    std::string name() const override;
    std::string complexity() const override;
    void sort(std::vector<int>& values) const override;
};

class DuplicateDetector : public Algorithm {
public:
    virtual bool containsDuplicate(const std::vector<int>& values) const = 0;
};

class BruteForceDuplicates : public DuplicateDetector {
public:
    std::string name() const override;
    std::string complexity() const override;
    bool containsDuplicate(const std::vector<int>& values) const override;
};

class SortScanDuplicates : public DuplicateDetector {
public:
    std::string name() const override;
    std::string complexity() const override;
    bool containsDuplicate(const std::vector<int>& values) const override;
};

class HashDuplicates : public DuplicateDetector {
public:
    std::string name() const override;
    std::string complexity() const override;
    bool containsDuplicate(const std::vector<int>& values) const override;
};

class FrequencyCounter : public Algorithm {
public:
    virtual std::vector<std::pair<int, std::size_t> > count(const std::vector<int>& values) const = 0;
};

class MapFrequencyCounter : public FrequencyCounter {
public:
    std::string name() const override;
    std::string complexity() const override;
    std::vector<std::pair<int, std::size_t> > count(const std::vector<int>& values) const override;
};

class UnorderedFrequencyCounter : public FrequencyCounter {
public:
    std::string name() const override;
    std::string complexity() const override;
    std::vector<std::pair<int, std::size_t> > count(const std::vector<int>& values) const override;
};

} // namespace turbocore
