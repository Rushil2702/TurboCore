#include "file_handling.hpp"

#include "big_integer.hpp"
#include "timing.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace turbocore {
namespace {
const char binaryMagic[] = {'T', 'C', 'B', '1'};
const std::size_t binaryHeaderBytes = 8;
const std::size_t binaryRecordBytes = 8;
const std::uint64_t maximumBinaryFileBytes = 64u * 1024u * 1024u;

void checkWrite(const std::ostream& output, const std::string& path) {
    if (!output) throw std::runtime_error("Failed writing file: " + path);
}

void writeUInt32(std::ostream& output, std::uint32_t value, const std::string& path) {
    char bytes[4];
    for (unsigned int i = 0; i < 4; ++i)
        bytes[i] = static_cast<char>((value >> (i * 8)) & 0xffu);
    output.write(bytes, sizeof(bytes));
    checkWrite(output, path);
}

std::uint32_t readUInt32(std::istream& input, const std::string& path) {
    char bytes[4];
    input.read(bytes, sizeof(bytes));
    if (input.gcount() != static_cast<std::streamsize>(sizeof(bytes)))
        throw std::invalid_argument("Truncated binary record in file: " + path);
    std::uint32_t value = 0;
    for (unsigned int i = 0; i < 4; ++i)
        value |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[i])) << (i * 8);
    return value;
}

void writeRecord(std::ostream& output, const FileRecord& record, const std::string& path) {
    writeUInt32(output, record.id, path);
    writeUInt32(output, record.value, path);
}

FileRecord readRecord(std::istream& input, const std::string& path) {
    const std::uint32_t id = readUInt32(input, path);
    const std::uint32_t value = readUInt32(input, path);
    return FileRecord(id, value);
}

std::uint32_t readHeader(std::istream& input, const std::string& path) {
    char magic[sizeof(binaryMagic)];
    input.read(magic, sizeof(magic));
    if (input.gcount() != static_cast<std::streamsize>(sizeof(magic)))
        throw std::invalid_argument("Missing or truncated TurboCore binary header: " + path);
    for (std::size_t i = 0; i < sizeof(binaryMagic); ++i)
        if (magic[i] != binaryMagic[i])
            throw std::invalid_argument("Unrecognized TurboCore binary file format: " + path);
    return readUInt32(input, path);
}

void writeHeader(std::ostream& output, std::uint32_t count, const std::string& path) {
    output.write(binaryMagic, sizeof(binaryMagic));
    checkWrite(output, path);
    writeUInt32(output, count, path);
}

std::string promptLine(std::istream& input, std::ostream& output, const std::string& prompt) {
    output << prompt;
    std::string line;
    if (!std::getline(input, line)) throw std::runtime_error("Input ended");
    return line;
}

std::string promptPath(std::istream& input, std::ostream& output, const std::string& prompt) {
    const std::string path = promptLine(input, output, prompt);
    if (path.empty()) throw std::invalid_argument("File path cannot be empty");
    return path;
}

std::uint32_t parseUInt32(const std::string& text, const std::string& field) {
    std::istringstream parser(text);
    std::string token, extraToken;
    unsigned long long value;
    char trailing;
    if (!(parser >> token) || (parser >> extraToken) || token[0] == '-')
        throw std::invalid_argument("Invalid or out-of-range " + field);
    std::istringstream number(token);
    if (!(number >> value) || (number >> trailing) || value > (std::numeric_limits<std::uint32_t>::max)())
        throw std::invalid_argument("Invalid or out-of-range " + field);
    return static_cast<std::uint32_t>(value);
}

std::size_t parseSize(const std::string& text, const std::string& field) {
    std::istringstream parser(text);
    std::string token, extraToken;
    unsigned long long value;
    char trailing;
    if (!(parser >> token) || (parser >> extraToken) || token[0] == '-')
        throw std::invalid_argument("Invalid or out-of-range " + field);
    std::istringstream number(token);
    if (!(number >> value) || (number >> trailing) || value > (std::numeric_limits<std::size_t>::max)())
        throw std::invalid_argument("Invalid or out-of-range " + field);
    return static_cast<std::size_t>(value);
}

void testRequire(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("File handling test failed: " + message);
}

class TemporaryFileCleanup {
public:
    explicit TemporaryFileCleanup(const std::vector<std::string>& paths) : paths_(paths) {}
    ~TemporaryFileCleanup() {
        for (std::size_t i = 0; i < paths_.size(); ++i) std::remove(paths_[i].c_str());
    }

private:
    const std::vector<std::string>& paths_;
};

bool exists(const std::string& path) {
    std::ifstream input(path.c_str(), std::ios::binary);
    return input.good();
}

bool samePath(const std::string& left, const std::string& right) {
#ifdef _WIN32
    const auto canonical = [](const std::string& path) {
        std::vector<char> buffer(32768, '\0');
        const DWORD length = GetFullPathNameA(path.c_str(), static_cast<DWORD>(buffer.size()), &buffer[0], 0);
        if (length == 0 || length >= buffer.size())
            throw std::invalid_argument("Could not verify that BigInteger input and output paths are separate");
        std::string result(&buffer[0], length);
        for (std::size_t i = 0; i < result.size(); ++i) {
            if (result[i] == '/') result[i] = '\\';
            else if (result[i] >= 'A' && result[i] <= 'Z') result[i] = static_cast<char>(result[i] - 'A' + 'a');
        }
        return result;
    };
    return canonical(left) == canonical(right);
#else
    return left == right;
#endif
}

std::uint64_t rawByteCount(const std::string& path) {
    std::ifstream input(path.c_str(), std::ios::in | std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open test file to count bytes: " + path);
    std::uint64_t count = 0;
    char value;
    while (input.get(value)) ++count;
    if (!input.eof()) throw std::runtime_error("Failed counting test bytes: " + path);
    return count;
}

std::string unusedTestStem() {
    for (unsigned int attempt = 0; attempt < 100; ++attempt) {
        const std::string stem = "turbocore_file_test_" + timing::newRunId() + "_" + std::to_string(attempt);
        if (!exists(stem + ".txt") && !exists(stem + ".empty") && !exists(stem + ".bin") &&
            !exists(stem + ".bad") && !exists(stem + ".big") && !exists(stem + ".result") &&
            !exists(stem + ".invalid") && !exists(stem + ".should_not_exist")) return stem;
    }
    throw std::runtime_error("Could not allocate unique temporary file names for tests");
}

} // namespace

void writeTextFile(const std::string& path, const std::string& text, bool append) {
    const std::ios::openmode mode = std::ios::out | (append ? std::ios::app : std::ios::trunc);
    std::ofstream output(path.c_str(), mode);
    if (!output) throw std::runtime_error("Cannot open text file for writing: " + path);
    output << text;
    checkWrite(output, path);
    output.close();
    if (output.fail()) throw std::runtime_error("Failed closing text file: " + path);
}

FileStatistics analyzeTextFile(const std::string& path) {
    std::ifstream text(path.c_str());
    if (!text) throw std::runtime_error("Cannot open text file for analysis: " + path);
    FileStatistics stats = {0, 0, 0};
    std::string line;
    while (std::getline(text, line)) ++stats.lines;
    if (text.bad()) throw std::runtime_error("Failed reading text lines: " + path);

    text.clear();
    text.seekg(0, std::ios::beg);
    std::string word;
    while (text >> word) ++stats.words;
    if (text.bad()) throw std::runtime_error("Failed reading text words: " + path);
    text.close();

    std::ifstream bytes(path.c_str(), std::ios::in | std::ios::binary);
    if (!bytes) throw std::runtime_error("Cannot reopen file for byte counting: " + path);
    char value;
    while (bytes.get(value)) ++stats.bytes;
    if (!bytes.eof()) throw std::runtime_error("Failed counting file bytes: " + path);
    return stats;
}

void writeBinaryRecords(const std::string& path, const std::vector<FileRecord>& records) {
    if (records.size() > (std::numeric_limits<std::uint32_t>::max)() ||
        static_cast<std::uint64_t>(records.size()) * binaryRecordBytes + binaryHeaderBytes > maximumBinaryFileBytes)
        throw std::length_error("Too many records for the TurboCore binary format");
    std::ofstream output(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("Cannot open binary file for writing: " + path);
    writeHeader(output, static_cast<std::uint32_t>(records.size()), path);
    for (std::size_t i = 0; i < records.size(); ++i) writeRecord(output, records[i], path);
    output.flush();
    checkWrite(output, path);
    output.close();
    if (output.fail()) throw std::runtime_error("Failed closing binary file: " + path);
}

std::vector<FileRecord> readBinaryRecords(const std::string& path) {
    std::ifstream input(path.c_str(), std::ios::in | std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open binary file for reading: " + path);
    const std::uint32_t count = readHeader(input, path);
    input.seekg(0, std::ios::end);
    const std::streamoff length = input.tellg();
    if (length < 0 || static_cast<std::uint64_t>(length) > maximumBinaryFileBytes || static_cast<std::uint64_t>(length) !=
            static_cast<std::uint64_t>(binaryHeaderBytes) + static_cast<std::uint64_t>(count) * binaryRecordBytes)
        throw std::invalid_argument("Binary file length does not match its record count: " + path);
    input.seekg(static_cast<std::streamoff>(binaryHeaderBytes), std::ios::beg);
    if (!input) throw std::runtime_error("Failed seeking in binary file: " + path);
    std::vector<FileRecord> records;
    records.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) records.push_back(readRecord(input, path));
    return records;
}

FilePositioningResult updateBinaryRecord(const std::string& path, std::size_t index,
                                         const FileRecord& replacement) {
    const std::vector<FileRecord> records = readBinaryRecords(path);
    if (index >= records.size()) throw std::out_of_range("Binary record index is outside the file");
    const std::streamoff offset = static_cast<std::streamoff>(binaryHeaderBytes + index * binaryRecordBytes);
    std::fstream file(path.c_str(), std::ios::in | std::ios::out | std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open binary file for positioned update: " + path);
    file.seekg(offset, std::ios::beg);
    if (!file) throw std::runtime_error("seekg failed for binary record: " + path);
    const std::streamoff readOffset = file.tellg();
    const FileRecord previous = readRecord(file, path);
    file.seekp(offset, std::ios::beg);
    if (!file) throw std::runtime_error("seekp failed for binary record: " + path);
    const std::streamoff writeOffset = file.tellp();
    writeRecord(file, replacement, path);
    file.flush();
    checkWrite(file, path);
    file.close();
    if (file.fail()) throw std::runtime_error("Failed closing positioned binary file: " + path);
    const std::vector<FileRecord> verified = readBinaryRecords(path);
    if (!(verified[index] == replacement)) throw std::runtime_error("Binary record update verification failed: " + path);
    FilePositioningResult result = {readOffset, writeOffset, previous, replacement};
    return result;
}

std::vector<std::string> processBigIntegerFile(const std::string& inputPath,
                                               const std::string& outputPath) {
    if (samePath(inputPath, outputPath))
        throw std::invalid_argument("BigInteger output must use a separate file path");
    std::ifstream input(inputPath.c_str());
    if (!input) throw std::runtime_error("Cannot open BigInteger input file: " + inputPath);
    std::vector<std::string> results;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        std::istringstream row(line);
        row >> std::ws;
        if (row.peek() == std::char_traits<char>::eof()) continue;
        BigInteger left, right;
        char operation;
        if (!(row >> left >> operation >> right))
            throw std::invalid_argument("Invalid BigInteger expression on input line " + std::to_string(lineNumber));
        row >> std::ws;
        if (row.peek() != std::char_traits<char>::eof())
            throw std::invalid_argument("Extra data on BigInteger input line " + std::to_string(lineNumber));
        if (operation == '+') results.push_back((left + right).toString());
        else if (operation == '-') results.push_back((left - right).toString());
        else if (operation == '*') results.push_back((left * right).toString());
        else throw std::invalid_argument("Unsupported BigInteger operation on input line " + std::to_string(lineNumber));
    }
    if (input.bad()) throw std::runtime_error("Failed reading BigInteger input file: " + inputPath);
    input.close();

    std::ofstream output(outputPath.c_str(), std::ios::out | std::ios::trunc);
    if (!output) throw std::runtime_error("Cannot open BigInteger output file: " + outputPath);
    for (std::size_t i = 0; i < results.size(); ++i) output << results[i] << '\n';
    checkWrite(output, outputPath);
    output.close();
    if (output.fail()) throw std::runtime_error("Failed closing BigInteger output file: " + outputPath);
    return results;
}

void FileHandlingLab::run(std::istream& input, std::ostream& output) const {
    for (;;) {
        output << "\nFile Handling Lab\n"
                  "1. Create/overwrite text sample (ofstream, ios::out)\n"
                  "2. Append a text line (ofstream, ios::app)\n"
                  "3. Analyze text file (ifstream, getline, formatted input)\n"
                  "4. Write and verify binary sample records\n"
                  "5. Update a binary record (fstream, seekg/seekp/tellg/tellp)\n"
                  "6. Calculate BigInteger expressions from a text file\n"
                  "0. Return\nChoice: ";
        std::string choice;
        if (!std::getline(input, choice) || choice == "0") return;
        try {
            if (choice == "1") {
                const std::string path = promptPath(input, output, "Path to create/overwrite: ");
                output << "Opening with ios::out truncates any existing file: " << path << "\n";
                writeTextFile(path, "TurboCore text sample\nFormatted integer: 42\n", false);
                output << "Text sample written and closed successfully.\n";
            } else if (choice == "2") {
                const std::string path = promptPath(input, output, "Path to append: ");
                const std::string line = promptLine(input, output, "Line to append: ");
                writeTextFile(path, line + "\n", true);
                output << "Opened with ios::app; line appended successfully.\n";
            } else if (choice == "3") {
                const std::string path = promptPath(input, output, "Text file path: ");
                std::ifstream file(path.c_str());
                if (!file) throw std::runtime_error("Cannot open text file: " + path);
                output << "Contents (read with getline):\n";
                std::string line;
                while (std::getline(file, line)) output << line << '\n';
                if (file.bad()) throw std::runtime_error("Failed reading text file: " + path);
                file.close();
                const FileStatistics stats = analyzeTextFile(path);
                output << "Lines=" << stats.lines << ", whitespace-delimited words=" << stats.words
                       << ", bytes=" << stats.bytes << " (raw file bytes, including line endings).\n";
            } else if (choice == "4") {
                const std::string path = promptPath(input, output, "Binary output path (existing file will be replaced): ");
                const std::vector<FileRecord> sample{FileRecord(101, 7), FileRecord(202, 42), FileRecord(303, 900)};
                writeBinaryRecords(path, sample);
                const std::vector<FileRecord> recovered = readBinaryRecords(path);
                if (recovered.size() != sample.size()) throw std::runtime_error("Binary round-trip record count mismatch");
                for (std::size_t i = 0; i < sample.size(); ++i)
                    if (!(sample[i] == recovered[i])) throw std::runtime_error("Binary round-trip verification failed");
                output << "Wrote and verified " << recovered.size()
                       << " records. Format: TCB1 header, count, then two little-endian uint32 fields per record.\n";
            } else if (choice == "5") {
                const std::string path = promptPath(input, output, "TurboCore binary file path: ");
                const std::size_t index = parseSize(promptLine(input, output, "Record index (zero-based): "), "record index");
                const std::uint32_t id = parseUInt32(promptLine(input, output, "Replacement record id: "), "record id");
                const std::uint32_t value = parseUInt32(promptLine(input, output, "Replacement record value: "), "record value");
                const FilePositioningResult result = updateBinaryRecord(path, index, FileRecord(id, value));
                output << "tellg() read offset=" << result.readOffset << ", tellp() write offset="
                       << result.writeOffset << "; seekg()/seekp() updated record " << index << " from ("
                       << result.previous.id << ',' << result.previous.value << ") to ("
                       << result.updated.id << ',' << result.updated.value << ").\n";
            } else if (choice == "6") {
                const std::string source = promptPath(input, output, "Input file (one: INTEGER OP INTEGER per line): ");
                const std::string destination = promptPath(input, output, "Separate output file (will be replaced): ");
                const std::vector<std::string> results = processBigIntegerFile(source, destination);
                output << "Validated and wrote " << results.size() << " BigInteger results to " << destination << ".\n";
            } else {
                output << "Invalid choice. Enter 0 through 6.\n";
                continue;
            }
        } catch (const std::exception& error) {
            output << "File lab error: " << error.what() << '\n';
        }
    }
}

void testFileHandling() {
    const std::string stem = unusedTestStem();
    const std::string textPath = stem + ".txt";
    const std::string emptyPath = stem + ".empty";
    const std::string binaryPath = stem + ".bin";
    const std::string malformedPath = stem + ".bad";
    const std::string bigInputPath = stem + ".big";
    const std::string bigOutputPath = stem + ".result";
    const std::string invalidInputPath = stem + ".invalid";
    const std::string shouldNotExistPath = stem + ".should_not_exist";
    std::vector<std::string> temporaryPaths{textPath, emptyPath, binaryPath, malformedPath,
        bigInputPath, bigOutputPath, invalidInputPath, shouldNotExistPath};
    const TemporaryFileCleanup cleanup(temporaryPaths);

    const std::string originalText = "one two\nthree\n";
    writeTextFile(textPath, originalText, false);
    writeTextFile(textPath, "four", true);
    FileStatistics stats = analyzeTextFile(textPath);
    testRequire(stats.lines == 3 && stats.words == 4 && stats.bytes == rawByteCount(textPath),
                "text overwrite, append, line/word/byte statistics");
    writeTextFile(textPath, "replacement\n", false);
    stats = analyzeTextFile(textPath);
    testRequire(stats.lines == 1 && stats.words == 1 && stats.bytes == rawByteCount(textPath),
                "text truncate mode replaces prior contents");
    writeTextFile(emptyPath, "", false);
    stats = analyzeTextFile(emptyPath);
    testRequire(stats.lines == 0 && stats.words == 0 && stats.bytes == 0, "empty-file statistics");

    const std::vector<FileRecord> originalRecords{FileRecord(1, 10), FileRecord(2, 20)};
    writeBinaryRecords(binaryPath, originalRecords);
    testRequire(readBinaryRecords(binaryPath) == originalRecords, "binary record round trip");
    const FilePositioningResult position = updateBinaryRecord(binaryPath, 1, FileRecord(9, 99));
    testRequire(position.readOffset == 16 && position.writeOffset == 16 &&
                position.previous == FileRecord(2, 20) && position.updated == FileRecord(9, 99),
                "binary seekg/seekp/tellg/tellp positioning");
    testRequire(readBinaryRecords(binaryPath)[1] == FileRecord(9, 99), "binary positioned write persists");
    {
        std::ofstream malformed(malformedPath.c_str(), std::ios::binary | std::ios::trunc);
        malformed.write("bad", 3);
        if (!malformed) throw std::runtime_error("Could not create malformed binary test input");
    }
    bool malformedRejected = false;
    try { readBinaryRecords(malformedPath); } catch (const std::invalid_argument&) { malformedRejected = true; }
    testRequire(malformedRejected, "invalid binary header rejected");
    bool missingRejected = false;
    try { analyzeTextFile(stem + ".missing"); } catch (const std::runtime_error&) { missingRejected = true; }
    testRequire(missingRejected, "missing text file reported as an error");

    writeTextFile(bigInputPath, "99999999999999999999 + 1\n-12 * -3\n5 - 8\n", false);
    const std::vector<std::string> bigResults = processBigIntegerFile(bigInputPath, bigOutputPath);
    testRequire(bigResults == std::vector<std::string>({"100000000000000000000", "36", "-3"}),
                "BigInteger text-file operations");
    {
        std::ifstream output(bigOutputPath.c_str());
        std::string line;
        testRequire(static_cast<bool>(std::getline(output, line)) && line == "100000000000000000000",
                    "BigInteger result is written to the output text file");
    }
    bool samePathRejected = false;
    try { processBigIntegerFile(bigInputPath, bigInputPath); }
    catch (const std::invalid_argument&) { samePathRejected = true; }
    testRequire(samePathRejected, "BigInteger input and output must be separate");
    writeTextFile(invalidInputPath, "1 + 2\n4 / 2\n", false);
    bool invalidExpressionRejected = false;
    try { processBigIntegerFile(invalidInputPath, shouldNotExistPath); }
    catch (const std::invalid_argument&) { invalidExpressionRejected = true; }
    testRequire(invalidExpressionRejected && !exists(shouldNotExistPath),
                "invalid BigInteger file rejected before opening output");

    FileHandlingLab lab;
    std::istringstream menuInput("invalid\n0\n");
    std::ostringstream menuOutput;
    lab.run(menuInput, menuOutput);
    testRequire(menuOutput.str().find("Invalid choice") != std::string::npos,
                "file lab handles invalid choices and returns on request");

}

} // namespace turbocore
