# OOP and C++ syllabus mapping

| Topic | Concrete use |
|---|---|
| Classes and encapsulation | `BigInteger` keeps sign and normalized digits private (`src/big_integer.hpp`); `BinarySearchTree` owns private nodes (`src/data_structures.cpp`). |
| Constructors and destructors | `BigInteger` has string and `long long` constructors; `Algorithm` has a virtual destructor; RAII `unique_ptr` manages BST nodes and polymorphic algorithm objects. |
| Function overloading | The `BigInteger` constructors overload initialization by decimal string or built-in integer. |
| Operator overloading | `BigInteger` supports arithmetic/comparison operators and friend `operator<<` for normal stream output. |
| Friend functions | `operator<<(std::ostream&, const BigInteger&)` is a friend, implemented in `src/big_integer.cpp`. |
| Inheritance and abstract classes | `Algorithm` and the `SearchAlgorithm`, `SortAlgorithm`, `DuplicateDetector`, and `FrequencyCounter` interfaces are abstract bases in `src/algorithms.hpp`. |
| Virtual functions and polymorphism | Benchmark and catalog vectors hold derived algorithms through base pointers; virtual calls invoke the selected implementation. |
| RTTI | `algorithmCategory` in `src/main.cpp` uses `dynamic_cast` to report the category of a catalog entry. |
| Exceptions | BigInteger parsing, unsorted searches, CSV schema errors, factorial bounds, and CLI validation use standard exceptions caught by `main` or the menu loop. |
| Templates and generic programming | `Statistics<T>`, `summarize<T>`, and generic benchmark `measure` in `src/main.cpp`; templated stack/queue/tree workloads in `src/data_structures.cpp`. |
| STL | `vector`, `list`, `deque`, `stack`, `queue`, `set`, `map`, `unordered_map`, `unordered_set`, `unique_ptr`, algorithms, and chrono. |
| Namespaces | Public modules use `turbocore` and `turbocore::number_theory`; implementation helpers use anonymous namespaces. |
| File handling and streams | `CsvReport` in `src/csv_report.cpp` writes escaped, versioned CSV; main loads report history; console and string streams parse/display data and test friend output. |
| Performance engineering | `src/main.cpp` times algorithm implementations; `src/data_structures.cpp` compares identical operation sequences for container pairs; memory experiment reports vector capacities and payload estimates. |

The RTTI use supports the algorithm catalog rather than exposing implementation-specific `typeid().name()` strings. The container comparisons explicitly describe their operation mix and implementation limitations.
