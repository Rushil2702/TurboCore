#include "data_structures.hpp"
#include "csv_report.hpp"

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
typedef std::chrono::steady_clock Clock;
struct Summary { double median, minimum, maximum, mean; };

Summary summarize(std::vector<long long> values) {
    std::sort(values.begin(), values.end());
    Summary result;
    result.minimum = static_cast<double>(values.front());
    result.maximum = static_cast<double>(values.back());
    result.median = static_cast<double>(values[values.size()/2]);
    if (values.size() % 2 == 0) result.median = (values[values.size()/2-1] + result.median) / 2.0;
    double total = 0;
    for (std::size_t i = 0; i < values.size(); ++i) total += values[i];
    result.mean = total / values.size();
    return result;
}

template <typename Fn>
Summary measure(Fn workload, unsigned long long expected) {
    std::vector<long long> samples;
    for (int i = 0; i < 7; ++i) {
        const Clock::time_point start = Clock::now();
        const unsigned long long actual = workload();
        const Clock::time_point stop = Clock::now();
        if (actual != expected) throw std::runtime_error("Data structure workload returned an incorrect result");
        samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start).count());
    }
    return summarize(samples);
}

std::string compiler() {
#ifdef __VERSION__
    return std::string("GCC ") + __VERSION__;
#else
    return "Unknown compiler";
#endif
}

void report(CsvReport& csv, const std::string& name, const std::string& complexity,
            const Summary& s, std::size_t count) {
    std::ostringstream n, median, minimum, maximum, mean;
    n << count;
    median << std::fixed << std::setprecision(2) << s.median;
    minimum << std::fixed << std::setprecision(2) << s.minimum;
    maximum << std::fixed << std::setprecision(2) << s.maximum;
    mean << std::fixed << std::setprecision(2) << s.mean;
    csv.writeRow(std::vector<std::string>{name, complexity, n.str(), "fixed-seed operation workload", compiler(), "7",
        median.str(), minimum.str(), maximum.str(), mean.str(), s.minimum < 1 ? "below clock resolution in at least one sample" : "measured"});
    std::cout << name << ": median " << median.str() << " ns, min " << minimum.str()
              << " ns, max " << maximum.str() << " ns, mean " << mean.str() << " ns\n";
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
    const std::string header = "algorithm,complexity,input_size,pattern,compiler,repetitions,median_ns,min_ns,max_ns,mean_ns,timing_note";
    CsvReport csv(compatibleReportPath(reportPath, header), header);
    const std::size_t count = 2000;
    std::vector<int> values(count);
    std::iota(values.begin(), values.end(), 0);
    std::mt19937 random(445566u);
    std::shuffle(values.begin(), values.end(), random);
    const int target = values[count/2];

    const unsigned long long stackExpected = runStack<std::stack<int> >(values);
    if (runVectorStack(values) != stackExpected) throw std::runtime_error("Stack/vector LIFO preflight failed");
    report(csv, "std::stack LIFO", "O(1) amortized push/pop", measure([&]() { return runStack<std::stack<int> >(values); }, stackExpected), count);
    report(csv, "reserved vector LIFO", "O(1) amortized push/pop", measure([&]() { return runVectorStack(values); }, stackExpected), count);

    const unsigned long long queueExpected = runQueue<std::queue<int> >(values);
    if (runDequeQueue(values) != queueExpected) throw std::runtime_error("Queue/deque FIFO preflight failed");
    report(csv, "std::queue enqueue/dequeue", "O(1) typical operations", measure([&]() { return runQueue<std::queue<int> >(values); }, queueExpected), count);
    report(csv, "std::deque enqueue/dequeue", "O(1) typical operations", measure([&]() { return runDequeQueue(values); }, queueExpected), count);

    const unsigned long long listExpected = runListWork(values, target);
    if (runVectorWork(values, target) != listExpected) throw std::runtime_error("List/vector find-erase preflight failed");
    report(csv, "std::list append/find/erase", "O(n) build/search, O(1) erase", measure([&]() { return runListWork(values, target); }, listExpected), count);
    report(csv, "std::vector append/find/erase", "O(n) build/search/erase", measure([&]() { return runVectorWork(values, target); }, listExpected), count);

    const std::vector<int> queries(values.begin(), values.begin() + 250);
    const unsigned long long treeExpected = runTree<BinarySearchTree>(values, queries);
    if (runTree<SetTree>(values, queries) != treeExpected) throw std::runtime_error("BST/set preflight failed");
    report(csv, "unbalanced BST insert/search", "O(log n) average, O(n) worst", measure([&]() { return runTree<BinarySearchTree>(values, queries); }, treeExpected), count);
    report(csv, "std::set insert/search", "O(log n) operations", measure([&]() { return runTree<SetTree>(values, queries); }, treeExpected), count);
    csv.flush();
    std::cout << "Data structure report saved to " << csv.path()
              << ". Comparisons include container setup and destruction; allocator and implementation details affect results.\n";
}

} // namespace turbocore
