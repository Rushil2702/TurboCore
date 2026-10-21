#include "csv_report.hpp"

#include <sstream>
#include <stdexcept>

namespace turbocore {
namespace {
bool readFirstLine(const std::string& path, std::string& line, bool& exists) {
    std::ifstream input(path.c_str());
    exists = input.good();
    if (!exists) return false;
    return static_cast<bool>(std::getline(input, line));
}

std::string numberedPath(const std::string& path, unsigned int version) {
    const std::string::size_type dot = path.find_last_of('.');
    std::ostringstream suffix;
    suffix << "_v" << version;
    if (dot == std::string::npos) return path + suffix.str();
    return path.substr(0, dot) + suffix.str() + path.substr(dot);
}
}

std::string compatibleReportPath(const std::string& preferredPath, const std::string& header) {
    std::string candidate = preferredPath;
    for (unsigned int version = 1; version < 1000; ++version) {
        std::string firstLine;
        bool exists = false;
        const bool hasHeader = readFirstLine(candidate, firstLine, exists);
        if (!exists || !hasHeader || firstLine == header) return candidate;
        candidate = numberedPath(preferredPath, version + 1);
    }
    throw std::runtime_error("Could not find a compatible CSV report filename");
}

std::string csvEscape(const std::string& value) {
    bool quote = false;
    std::string escaped;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '"') { escaped += "\"\""; quote = true; }
        else { escaped += value[i]; if (value[i] == ',' || value[i] == '\n' || value[i] == '\r') quote = true; }
    }
    return quote ? "\"" + escaped + "\"" : escaped;
}

CsvReport::CsvReport(const std::string& path, const std::string& header)
    : path_(path), header_(header) {
    std::string firstLine;
    bool exists = false;
    const bool hasHeader = readFirstLine(path_, firstLine, exists);
    if (exists && hasHeader && firstLine != header_)
        throw std::runtime_error("CSV schema mismatch in " + path_);
    stream_.open(path_.c_str(), std::ios::out | std::ios::app);
    if (!stream_) throw std::runtime_error("Cannot open CSV report " + path_);
    if (!exists || !hasHeader) {
        stream_ << header_ << '\n';
        if (!stream_) throw std::runtime_error("Failed writing CSV header " + path_);
    }
}

void CsvReport::writeRow(const std::vector<std::string>& cells) {
    for (std::size_t i = 0; i < cells.size(); ++i) {
        if (i) stream_ << ',';
        stream_ << csvEscape(cells[i]);
    }
    stream_ << '\n';
    if (!stream_) throw std::runtime_error("Failed writing CSV report " + path_);
}

const std::string& CsvReport::path() const { return path_; }
void CsvReport::flush() {
    stream_.flush();
    if (!stream_) throw std::runtime_error("Failed flushing CSV report " + path_);
}

} // namespace turbocore
