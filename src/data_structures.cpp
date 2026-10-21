#include "data_structures.hpp"
#include "csv_report.hpp"
#include "timing.hpp"

#include <algorithm>
#include <chrono>
#include <deque>
#include <iomanip>
#include <iostream>
#include <list>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>
#include <queue>

namespace turbocore {
namespace {
template <typename Fn>
timing::Summary measure(Fn workload, unsigned long long expected, std::size_t maxBatch) {
    volatile unsigned long long sink = 0;
    return timing::measureAdaptive([&](std::size_t batch) {
        std::vector<unsigned long long> results(batch);
        const timing::Clock::time_point start = timing::Clock::now();
        for (std::size_t i = 0; i < batch; ++i) results[i] = workload();
        const timing::Clock::time_point stop = timing::Clock::now();
        unsigned long long checksum = 0;
        for (std::size_t i = 0; i < batch; ++i) {
            if (results[i] != expected) throw std::runtime_error("Data structure workload returned an incorrect result");
            checksum += results[i];
        }
        sink += checksum;
        return timing::elapsedNanoseconds(start, stop);
    }, 7, maxBatch);
}

std::string compiler() {
#ifdef __VERSION__
    return std::string("GCC ") + __VERSION__;
#else
    return "Unknown compiler";
#endif
}

void report(CsvReport& csv, const std::string& runId, const std::string& name, const std::string& complexity,
            const timing::Summary& s, std::size_t count) {
    const std::string note = s.belowResolution
        ? "one or more total batches below clock resolution; measured ns per workload"
        : "measured ns per workload; fixed-seed inputs prepared before timing; correctness checked after timing";
    csv.writeRow(std::vector<std::string>{runId, "data structure lab", name, complexity,
        std::to_string(count), "fixed-seed; same operation sequence per pair", compiler(), "7",
        std::to_string(s.batchOperations), timing::number(s.median), timing::number(s.minimum),
        timing::number(s.maximum), timing::number(s.mean), note});
    std::cout << name << ": batch=" << s.batchOperations << ", median " << timing::number(s.median)
              << " ns/workload, min " << timing::number(s.minimum) << ", max "
              << timing::number(s.maximum) << ", mean " << timing::number(s.mean) << "\n";
}

class BinarySearchTree {
    struct Node {
        explicit Node(int v) : value(v) {}
        int value;
        std::unique_ptr<Node> left, right;
    };
public:
    void insert(int value) {
        std::unique_ptr<Node>* link = &root_;
        while (*link) {
            if ((*link)->value == value) return;
            link = value < (*link)->value ? &(*link)->left : &(*link)->right;
        }
        link->reset(new Node(value));
    }
    bool contains(int value) const {
        const Node* node = root_.get();
        while (node) {
            if (node->value == value) return true;
            node = value < node->value ? node->left.get() : node->right.get();
        }
        return false;
    }
private:
    std::unique_ptr<Node> root_;
};

template <typename Container>
unsigned long long runStack(const std::vector<int>& values) {
    Container container;
    for (std::size_t i = 0; i < values.size(); ++i) container.push(values[i]);
    unsigned long long hash = 0;
    while (!container.empty()) { hash = hash * 131u + static_cast<unsigned int>(container.top()); container.pop(); }
    return hash;
}
unsigned long long runVectorStack(const std::vector<int>& values) {
    std::vector<int> container; container.reserve(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) container.push_back(values[i]);
    unsigned long long hash = 0;
    while (!container.empty()) { hash = hash * 131u + static_cast<unsigned int>(container.back()); container.pop_back(); }
    return hash;
}

template <typename Container>
unsigned long long runQueue(const std::vector<int>& values) {
    Container container;
    for (std::size_t i = 0; i < values.size(); ++i) container.push(values[i]);
    unsigned long long hash = 0;
    while (!container.empty()) { hash = hash * 131u + static_cast<unsigned int>(container.front()); container.pop(); }
    return hash;
}
unsigned long long runDequeQueue(const std::vector<int>& values) {
    std::deque<int> container;
    for (std::size_t i = 0; i < values.size(); ++i) container.push_back(values[i]);
    unsigned long long hash = 0;
    while (!container.empty()) { hash = hash * 131u + static_cast<unsigned int>(container.front()); container.pop_front(); }
    return hash;
}

unsigned long long runListWork(const std::vector<int>& values, int target) {
    std::list<int> container;
    for (std::size_t i = 0; i < values.size(); ++i) container.push_back(values[i]);
    std::list<int>::iterator it = std::find(container.begin(), container.end(), target);
    if (it == container.end()) return 0;
    container.erase(it);
    unsigned long long hash = container.size();
    for (std::list<int>::const_iterator item = container.begin(); item != container.end(); ++item)
        hash = hash * 131u + static_cast<unsigned int>(*item);
    return hash;
}
unsigned long long runVectorWork(const std::vector<int>& values, int target) {
    std::vector<int> container;
    for (std::size_t i = 0; i < values.size(); ++i) container.push_back(values[i]);
    std::vector<int>::iterator it = std::find(container.begin(), container.end(), target);
    if (it == container.end()) return 0;
    container.erase(it);
    unsigned long long hash = container.size();
    for (std::vector<int>::const_iterator item = container.begin(); item != container.end(); ++item)
        hash = hash * 131u + static_cast<unsigned int>(*item);
    return hash;
}

template <typename Tree>
unsigned long long runTree(const std::vector<int>& values, const std::vector<int>& queries) {
    Tree tree;
    for (std::size_t i = 0; i < values.size(); ++i) tree.insert(values[i]);
    unsigned long long outcomes = 0;
    for (std::size_t i = 0; i < queries.size(); ++i)
        outcomes = outcomes * 131u + (tree.contains(queries[i]) ? 1u : 0u);
    return outcomes;
}
struct SetTree {
    void insert(int value) { values.insert(value); }
    bool contains(int value) const { return values.find(value) != values.end(); }
    std::set<int> values;
};
}

void testDataStructures() {
    const std::vector<int> values{7, 2, 9, 1, 5, 8};
    if (runStack<std::stack<int> >(values) != runVectorStack(values)) throw std::runtime_error("Stack/vector LIFO test failed");
    if (runQueue<std::queue<int> >(values) != runDequeQueue(values)) throw std::runtime_error("Queue/deque FIFO test failed");
    if (runListWork(values, 5) != runVectorWork(values, 5)) throw std::runtime_error("List/vector find-erase test failed");
    const std::vector<int> queries{7, 2, 100, 1, -1};
    if (runTree<BinarySearchTree>(values, queries) != runTree<SetTree>(values, queries)) throw std::runtime_error("BST/set test failed");
}

void runDataStructureLab(const std::string& reportPath) {
    const std::string header = timing::csvHeader();
    CsvReport csv(compatibleReportPath(reportPath, header), header);
    const std::string runId = timing::newRunId();
    const std::size_t count = 2000;
    const std::size_t maxBatch = 1024;
    std::vector<int> values(count);
    std::iota(values.begin(), values.end(), 0);
    std::mt19937 random(445566u);
    std::shuffle(values.begin(), values.end(), random);
    const int target = values[count/2];

    const unsigned long long stackExpected = runStack<std::stack<int> >(values);
    if (runVectorStack(values) != stackExpected) throw std::runtime_error("Stack/vector LIFO preflight failed");
    report(csv, runId, "std::stack LIFO", "O(1) amortized push/pop", measure([&]() { return runStack<std::stack<int> >(values); }, stackExpected, maxBatch), count);
    report(csv, runId, "reserved vector LIFO", "O(1) amortized push/pop", measure([&]() { return runVectorStack(values); }, stackExpected, maxBatch), count);

    const unsigned long long queueExpected = runQueue<std::queue<int> >(values);
    if (runDequeQueue(values) != queueExpected) throw std::runtime_error("Queue/deque FIFO preflight failed");
    report(csv, runId, "std::queue enqueue/dequeue", "O(1) typical operations", measure([&]() { return runQueue<std::queue<int> >(values); }, queueExpected, maxBatch), count);
    report(csv, runId, "std::deque enqueue/dequeue", "O(1) typical operations", measure([&]() { return runDequeQueue(values); }, queueExpected, maxBatch), count);

    const unsigned long long listExpected = runListWork(values, target);
    if (runVectorWork(values, target) != listExpected) throw std::runtime_error("List/vector find-erase preflight failed");
    report(csv, runId, "std::list append/find/erase", "O(n) build/search, O(1) erase", measure([&]() { return runListWork(values, target); }, listExpected, maxBatch), count);
    report(csv, runId, "std::vector append/find/erase", "O(n) build/search/erase", measure([&]() { return runVectorWork(values, target); }, listExpected, maxBatch), count);

    const std::vector<int> queries(values.begin(), values.begin() + 250);
    const unsigned long long treeExpected = runTree<BinarySearchTree>(values, queries);
    if (runTree<SetTree>(values, queries) != treeExpected) throw std::runtime_error("BST/set preflight failed");
    report(csv, runId, "unbalanced BST insert/search", "O(log n) average, O(n) worst", measure([&]() { return runTree<BinarySearchTree>(values, queries); }, treeExpected, maxBatch), count);
    report(csv, runId, "std::set insert/search", "O(log n) operations", measure([&]() { return runTree<SetTree>(values, queries); }, treeExpected, maxBatch), count);
    csv.flush();
    std::cout << "Data structure run " << runId << " appended to " << csv.path()
              << ". Timings include workload setup and teardown; allocator and implementation details affect results.\n";
}

} // namespace turbocore
