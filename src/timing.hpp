#pragma once

#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace turbocore {
namespace timing {

typedef std::chrono::steady_clock Clock;

struct Summary {
    double median;
    double minimum;
    double maximum;
    double mean;
    double targetBatchNanoseconds;
    std::size_t batchOperations;
    std::size_t unresolvedSamples;
    bool targetReached;
    bool reliable;
};

inline Summary summarize(const std::vector<double>& input, std::size_t batchOperations = 1,
                        std::size_t unresolvedSamples = 0, bool targetReached = true,
                        double targetBatchNanoseconds = 5000000.0) {
    if (input.empty()) throw std::invalid_argument("Cannot summarize no timing samples");
    std::vector<double> values;
    values.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (!std::isfinite(input[i]) || input[i] < 0.0)
            throw std::invalid_argument("Timing samples must be finite and non-negative");
        if (input[i] <= 0.0) ++unresolvedSamples;
        else values.push_back(input[i]);
    }
    std::sort(values.begin(), values.end());
    Summary result;
    if (values.empty()) {
        const double unavailable = std::numeric_limits<double>::quiet_NaN();
        result.minimum = result.maximum = result.median = result.mean = unavailable;
    } else {
        result.minimum = values.front();
        result.maximum = values.back();
        result.median = values[values.size() / 2];
        if (values.size() % 2 == 0)
            result.median = (values[values.size() / 2 - 1] + result.median) / 2.0;
        double total = 0.0;
        for (std::size_t i = 0; i < values.size(); ++i) total += values[i];
        result.mean = total / static_cast<double>(values.size());
    }
    result.batchOperations = batchOperations;
    result.targetBatchNanoseconds = targetBatchNanoseconds;
    result.unresolvedSamples = unresolvedSamples;
    result.targetReached = targetReached;
    result.reliable = unresolvedSamples == 0 && targetReached && !values.empty();
    return result;
}

inline double elapsedNanoseconds(Clock::time_point start, Clock::time_point stop) {
    return std::chrono::duration<double, std::nano>(stop - start).count();
}

inline std::string newRunId() {
    const long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    static unsigned int sequence = 0;
    std::ostringstream id;
    id << microseconds << '-' << sequence++;
    return id.str();
}

// runBatch(count) must prepare inputs before starting its own timer, time only
// the repeated work, verify outputs afterward, and return total elapsed ns.
template <typename BatchRunner>
Summary measureAdaptive(BatchRunner runBatch, int repetitions, std::size_t maxBatch,
                        double targetBatchNanoseconds = 5000000.0) {
    if (repetitions <= 0 || maxBatch == 0 || !std::isfinite(targetBatchNanoseconds) || targetBatchNanoseconds <= 0.0)
        throw std::invalid_argument("Timing repetitions and maximum batch must be positive");
    std::size_t batch = 1;
    double calibration = 0.0;
    while (true) {
        calibration = runBatch(batch);
        if (!std::isfinite(calibration) || calibration < 0.0)
            throw std::runtime_error("Timer returned an invalid calibration duration");
        if (calibration >= targetBatchNanoseconds || batch >= maxBatch) break;
        batch = batch > maxBatch / 2 ? maxBatch : batch * 2;
    }
    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(repetitions));
    bool targetReached = calibration >= targetBatchNanoseconds;
    for (int sample = 0; sample < repetitions; ++sample) {
        const double elapsed = runBatch(batch);
        if (!std::isfinite(elapsed) || elapsed < 0.0)
            throw std::runtime_error("Timer returned an invalid duration");
        if (elapsed < targetBatchNanoseconds) targetReached = false;
        samples.push_back(elapsed / static_cast<double>(batch));
    }
    return summarize(samples, batch, 0, targetReached, targetBatchNanoseconds);
}

inline std::string number(double value) {
    if (!std::isfinite(value)) return "NA";
    std::ostringstream output;
    output << std::fixed << std::setprecision(6) << value;
    return output.str();
}

inline const std::string& csvHeader() {
    static const std::string header =
        "run_id,experiment,implementation,complexity,input_size,workload,compiler,repetitions,batch_operations,target_batch_ns,median_ns_per_operation,min_ns_per_operation,max_ns_per_operation,mean_ns_per_operation,unresolved_samples,measurement_reliable,timing_method_version,timing_note";
    return header;
}

static const char* const methodVersion = "adaptive-steady-clock-v2";

} // namespace timing
} // namespace turbocore
