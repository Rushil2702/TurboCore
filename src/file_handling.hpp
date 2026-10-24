#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace turbocore {

struct FileStatistics {
    std::size_t lines;
    std::size_t words;
    std::uint64_t bytes;
};

struct FileRecord {
    std::uint32_t id;
    std::uint32_t value;
    FileRecord(std::uint32_t recordId = 0, std::uint32_t recordValue = 0)
        : id(recordId), value(recordValue) {}
    bool operator==(const FileRecord& other) const {
        return id == other.id && value == other.value;
    }
};

struct FilePositioningResult {
    std::streamoff readOffset;
    std::streamoff writeOffset;
    FileRecord previous;
    FileRecord updated;
};

FileStatistics analyzeTextFile(const std::string& path);
void writeTextFile(const std::string& path, const std::string& text, bool append);
void writeBinaryRecords(const std::string& path, const std::vector<FileRecord>& records);
std::vector<FileRecord> readBinaryRecords(const std::string& path);
FilePositioningResult updateBinaryRecord(const std::string& path, std::size_t index,
                                         const FileRecord& replacement);
std::vector<std::string> processBigIntegerFile(const std::string& inputPath,
                                               const std::string& outputPath);

class FileHandlingLab {
public:
    void run(std::istream& input, std::ostream& output) const;
};

void testFileHandling();

} // namespace turbocore
