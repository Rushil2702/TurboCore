#pragma once

#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

namespace turbocore {

std::string compatibleReportPath(const std::string& preferredPath, const std::string& header);

class CsvReport {
public:
    CsvReport(const std::string& path, const std::string& header);
    void writeRow(const std::vector<std::string>& cells);
    void flush();
    const std::string& path() const;
private:
    std::string path_;
    std::string header_;
    std::size_t columnCount_;
    std::ofstream stream_;
};

std::string csvEscape(const std::string& value);

} // namespace turbocore
