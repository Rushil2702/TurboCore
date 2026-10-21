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

Benchmarks use fixed-seed inputs at sizes 256, 1024, and 2048 over random, sorted, reverse, and duplicate-heavy patterns. Quadratic algorithms use smaller adaptive batches and are capped to keep total work practical. Each implementation is checked against a reference before timing, then results are checked again after timing. Inputs and result buffers are prepared before the timed interval; repeated operations run in adaptive batches until a target duration is reached or a workload-specific cap applies. `std::chrono::steady_clock` durations are retained as floating-point nanoseconds, and reports show median, minimum, maximum, and mean nanoseconds per complete operation/workload with six decimal places. Batch size and a run identifier are included. If the timer still cannot distinguish a batch, the report says so and preserves the measured zero rather than inventing a value.

Algorithm and data-structure CSVs share a versioned schema with run ID, experiment, implementation, input/workload, compiler, repetitions, batch size, and timing statistics. Each run appends rows to the compatible report; it does not replace prior results. If a report has a different header, TurboCore preserves it and chooses the next versioned filename (for example `benchmark_results_v3.csv`). Every row is checked against its header's column count. `--history` displays sequential report versions. Earlier CSV files are not rewritten or silently mixed into the new schema.

## Data structure lab

`--structures` compares stack versus vector LIFO, queue versus deque FIFO, list versus vector append/find/erase, and an unbalanced binary search tree versus `std::set`. Each pair receives the same fixed-seed operations and is checked for equivalent results outside the timed interval. Repeated adaptive batches measure the same complete workload, reporting nanoseconds per workload. CSV output is appended to `data_structure_results.csv`, with schema-versioned files when needed.

These timings include container construction, operations, and destruction. STL adaptors may share underlying implementations; the BST uses randomized insertion order and is not balanced. Results depend on allocator, library, compiler, and machine, so they illustrate workload behavior rather than universally ranking containers.

## Computation and memory

`BigInteger` stores normalized signed decimal digits and supports string and `long long` constructors, arithmetic, comparisons, and stream insertion. Invalid decimal strings throw `std::invalid_argument`.

The number-theory module provides 64-bit unsigned GCD, factorial with BigInteger output (input capped at 1000), and 32-bit primality checking. These bounds keep CLI work predictable.

`--memory` separately measures adaptive batches of vector append without reservation, `reserve()` allocation, and append after reservation. Each vector is validated after its timed workload. It reports repeated-sample median, minimum, maximum, mean, batch size, and observed capacity changes. Payload bytes use `capacity * sizeof(int)` and exclude allocator metadata; the experiment does not measure peak resident memory or exact allocation counts. Memory reports append to compatible history and use a versioned file when the schema differs. `--history` displays algorithm, structure, and memory reports.

## OOP syllabus map

See [docs/syllabus_mapping.md](docs/syllabus_mapping.md) for concrete classes, functions, and source locations.
