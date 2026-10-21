#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iomanip>
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
    std::size_t batchOperations;
    bool belowResolution;
};

inline Summary summarize(const std::vector<double>& input, std::size_t batchOperations = 1,
                         bool belowResolution = false) {
    if (input.empty()) throw std::invalid_argument("Cannot summarize no timing samples");
    std::vector<double> values(input);
    std::sort(values.begin(), values.end());
    Summary result;
    result.minimum = values.front();
    result.maximum = values.back();
    result.median = values[values.size() / 2];
    if (values.size() % 2 == 0)
        result.median = (values[values.size() / 2 - 1] + result.median) / 2.0;
    double total = 0.0;
    for (std::size_t i = 0; i < values.size(); ++i) total += values[i];
    result.mean = total / static_cast<double>(values.size());
    result.batchOperations = batchOperations;
    result.belowResolution = belowResolution;
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
    if (repetitions <= 0 || maxBatch == 0)
        throw std::invalid_argument("Timing repetitions and maximum batch must be positive");
    std::size_t batch = 1;
    double calibration = 0.0;
    while (true) {
        calibration = runBatch(batch);
        if (calibration < 0.0) throw std::runtime_error("Timer returned a negative calibration duration");
        if (calibration >= targetBatchNanoseconds || batch >= maxBatch) break;
        batch = batch > maxBatch / 2 ? maxBatch : batch * 2;
    }
    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(repetitions));
    bool belowResolution = calibration <= 0.0;
    for (int sample = 0; sample < repetitions; ++sample) {
        const double elapsed = runBatch(batch);
        if (elapsed < 0.0) throw std::runtime_error("Timer returned a negative duration");
        if (elapsed <= 0.0) belowResolution = true;
        samples.push_back(elapsed / static_cast<double>(batch));
    }
    return summarize(samples, batch, belowResolution);
}

inline std::string number(double value) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(6) << value;
    return output.str();
}

inline const std::string& csvHeader() {
    static const std::string header =
        "run_id,experiment,implementation,complexity,input_size,workload,compiler,repetitions,batch_operations,median_ns_per_operation,min_ns_per_operation,max_ns_per_operation,mean_ns_per_operation,timing_note";
    return header;
}

} // namespace timing
} // namespace turbocore
