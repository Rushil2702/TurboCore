# Built-in automated checks

From the repository root, build with the command in `README.md`, then run `.\TurboCore.exe --test`.

The suite cross-checks all sorters against `std::sort`, search edge cases and sorted-input rejection, duplicate/frequency strategies, BigInteger parsing/arithmetic/comparisons/stream output, generic and fractional-nanosecond timing aggregation, number theory, and integer-list validation. It also exercises CSV escaping, append/version selection, column-count validation, preservation of old reports, data-structure workload equivalence, and the separate memory reserve/append report rows. Benchmark timing is adaptive and batched; temporary report files are removed after successful checks.
