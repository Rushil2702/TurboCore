# TurboCore

TurboCore is a C++14 command-line performance engineering and computational toolkit for MinGW.org GCC 6.3.0. It uses the standard library only and contains no UI.

## Build and test (PowerShell, from the repository root)

```powershell
g++ --version
g++ -std=c++14 -O2 -Wall -Wextra -pedantic src\main.cpp src\algorithms.cpp src\big_integer.cpp src\csv_report.cpp src\data_structures.cpp src\number_theory.cpp -o TurboCore.exe
.\TurboCore.exe --test
```

The test command runs algorithm edge cases, BigInteger arithmetic and stream output, number theory, integer input parsing, data structure equivalence, CSV escaping/versioning, and generic benchmark statistics. Test execution also runs a small data-structure timing/report workload; timings are informational and not fixed expected values.

## Commands

```powershell
.\TurboCore.exe --help
.\TurboCore.exe --benchmark
.\TurboCore.exe --structures
.\TurboCore.exe --algorithms
.\TurboCore.exe --memory
.\TurboCore.exe --history
.\TurboCore.exe --number-theory gcd 48 18
.\TurboCore.exe --number-theory factorial 20
.\TurboCore.exe --number-theory prime 97
```

Running without arguments opens the interactive terminal menu. Search, BigInteger, and number-theory prompts validate input and display errors without terminating the menu.

## Algorithm and benchmark lab

Search: Linear Search O(n); Binary Search and Jump Search require ascending-sorted input and reject unsorted data. Their reported complexity includes the O(n) sortedness validation.

Sort: Bubble Sort, Insertion Sort, and Selection Sort O(n²); Merge Sort O(n log n); Quick Sort average O(n log n), worst O(n²); and `std::sort` O(n log n).

Duplicate detection uses pairwise comparison, sort-and-scan, and `unordered_set`. Frequency counters compare `std::map` and `std::unordered_map`.

Benchmarks use fixed-seed inputs at sizes 100, 500, and 1500 over random, sorted, reverse, and duplicate-heavy patterns, with seven repeated measurements. Quadratic algorithms are bounded by the maximum size. Each operation is checked against a reference before timing; measured results are checked after timing. Input creation/copies and CSV writes are outside the timed region. Search methods share the same sorted view; each sorting method receives the same original vector. Reports include median, minimum, maximum, mean, compiler information, and an explicit note when a sample falls below the clock's nanosecond resolution. A zero nanosecond duration is an observed below-resolution sample, not a fabricated substitute.

`benchmark_results.csv` stores algorithm results. When an existing CSV has a different header, TurboCore preserves it and chooses a versioned filename such as `benchmark_results_v2.csv`. `--history` displays all sequential report versions.

## Data structure lab

`--structures` compares stack versus vector LIFO, queue versus deque FIFO, list versus vector append/find/erase, and an unbalanced binary search tree versus `std::set`. Each pair receives the same fixed-seed operations and is checked for equivalent results. CSV output is saved in `data_structure_results.csv`, with schema-versioned files when needed.

These timings include container construction, operations, and destruction. STL adaptors may share underlying implementations; the BST uses randomized insertion order and is not balanced. Results depend on allocator, library, compiler, and machine, so they illustrate workload behavior rather than universally ranking containers.

## Computation and memory

`BigInteger` stores normalized signed decimal digits and supports string and `long long` constructors, arithmetic, comparisons, and stream insertion. Invalid decimal strings throw `std::invalid_argument`.

The number-theory module provides 64-bit unsigned GCD, factorial with BigInteger output (input capped at 1000), and 32-bit primality checking. These bounds keep CLI work predictable.

`--memory` compares vector append timings with and without `reserve`, reports observed capacity changes, and estimates element payload bytes as `capacity * sizeof(int)`. It saves rows to `memory_results.csv`; the `reserve()` call itself is excluded from the insertion timer. It does not measure allocator metadata, peak resident memory, or exact allocation counts. `--history` displays algorithm, structure, and memory reports.

## OOP syllabus map

See [docs/syllabus_mapping.md](docs/syllabus_mapping.md) for concrete classes, functions, and source locations.
