#pragma once

#include <iosfwd>
#include <string>

namespace turbocore {

class BigInteger {
public:
    explicit BigInteger(const std::string& value = "0");
    explicit BigInteger(long long value);
    std::string toString() const;
    BigInteger operator+(const BigInteger& other) const;
    BigInteger operator-(const BigInteger& other) const;
    BigInteger operator*(const BigInteger& other) const;
    bool operator==(const BigInteger& other) const;
    bool operator!=(const BigInteger& other) const;
    bool operator<(const BigInteger& other) const;
    bool operator>(const BigInteger& other) const;
    friend std::istream& operator>>(std::istream& input, BigInteger& value);
    friend std::ostream& operator<<(std::ostream& output, const BigInteger& value);

private:
    bool negative_;
    std::string digits_;
    BigInteger(bool negative, const std::string& digits);
    static int compareMagnitude(const std::string& a, const std::string& b);
    static std::string addMagnitude(const std::string& a, const std::string& b);
    static std::string subtractMagnitude(const std::string& a, const std::string& b);
};

} // namespace turbocore
