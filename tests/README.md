# Built-in automated checks

From the repository root, build with the command in `README.md`, then run `.\TurboCore.exe --test`.

The suite cross-checks all sorters against `std::sort`, search edge cases and sorted-input rejection, duplicate/frequency strategies, BigInteger parsing/arithmetic/comparisons/stream output, generic timing aggregation, number theory, and integer-list validation. It also exercises CSV escaping, appending, schema-version path selection, preservation of old reports, and data-structure workload equivalence/report output. The data-structure report test records real timings only in a temporary file and removes it when successful.
