#include "big_integer.hpp"

#include <algorithm>
#include <ostream>
#include <stdexcept>

namespace turbocore {
namespace {
std::string normalize(const std::string& input) {
    if (input.empty()) throw std::invalid_argument("BigInteger cannot be empty");
    std::size_t pos = (input[0] == '+' || input[0] == '-') ? 1 : 0;
    if (pos == input.size()) throw std::invalid_argument("BigInteger requires digits");
    for (std::size_t i = pos; i < input.size(); ++i)
        if (input[i] < '0' || input[i] > '9') throw std::invalid_argument("Invalid BigInteger digit");
    while (pos + 1 < input.size() && input[pos] == '0') ++pos;
    return input.substr(pos);
}
}

BigInteger::BigInteger(const std::string& value)
    : negative_(!value.empty() && value[0] == '-'), digits_(normalize(value)) {
    if (digits_ == "0") negative_ = false;
}

BigInteger::BigInteger(long long value) : BigInteger(std::to_string(value)) {}

BigInteger::BigInteger(bool negative, const std::string& digits)
    : negative_(negative && digits != "0"), digits_(digits) {}

std::string BigInteger::toString() const { return (negative_ ? "-" : "") + digits_; }

int BigInteger::compareMagnitude(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
    if (a == b) return 0;
    return a < b ? -1 : 1;
}

std::string BigInteger::addMagnitude(const std::string& a, const std::string& b) {
    std::string out;
    int carry = 0;
    std::size_t i = a.size(), j = b.size();
    while (i || j || carry) {
        int sum = carry;
        if (i) sum += a[--i] - '0';
        if (j) sum += b[--j] - '0';
        out.push_back(static_cast<char>('0' + sum % 10));
        carry = sum / 10;
    }
    std::reverse(out.begin(), out.end());
    return out.empty() ? "0" : out;
}

std::string BigInteger::subtractMagnitude(const std::string& a, const std::string& b) {
    std::string out;
    int borrow = 0;
    std::size_t i = a.size(), j = b.size();
    while (i) {
        int digit = (a[--i] - '0') - borrow - (j ? b[--j] - '0' : 0);
        borrow = digit < 0;
        if (borrow) digit += 10;
        out.push_back(static_cast<char>('0' + digit));
    }
    std::reverse(out.begin(), out.end());
    std::size_t first = out.find_first_not_of('0');
    return first == std::string::npos ? "0" : out.substr(first);
}

BigInteger BigInteger::operator+(const BigInteger& other) const {
    if (negative_ == other.negative_) return BigInteger(negative_, addMagnitude(digits_, other.digits_));
    const int cmp = compareMagnitude(digits_, other.digits_);
    if (cmp == 0) return BigInteger("0");
    return cmp > 0 ? BigInteger(negative_, subtractMagnitude(digits_, other.digits_))
                    : BigInteger(other.negative_, subtractMagnitude(other.digits_, digits_));
}

BigInteger BigInteger::operator-(const BigInteger& other) const {
    BigInteger negated = other;
    if (negated.digits_ != "0") negated.negative_ = !negated.negative_;
    return *this + negated;
}

BigInteger BigInteger::operator*(const BigInteger& other) const {
    std::string product(digits_.size() + other.digits_.size(), '0');
    for (std::size_t i = digits_.size(); i > 0; --i) {
        int carry = 0;
        for (std::size_t j = other.digits_.size(); j > 0; --j) {
            const std::size_t p = i + j - 1;
            const int value = (digits_[i - 1] - '0') * (other.digits_[j - 1] - '0') +
                                (product[p] - '0') + carry;
            product[p] = static_cast<char>('0' + value % 10);
            carry = value / 10;
        }
        product[i - 1] = static_cast<char>(product[i - 1] + carry);
    }
    std::size_t first = product.find_first_not_of('0');
    const std::string result = first == std::string::npos ? "0" : product.substr(first);
    return BigInteger(negative_ != other.negative_, result);
}

bool BigInteger::operator==(const BigInteger& other) const {
    return negative_ == other.negative_ && digits_ == other.digits_;
}
bool BigInteger::operator!=(const BigInteger& other) const { return !(*this == other); }
bool BigInteger::operator<(const BigInteger& other) const {
    if (negative_ != other.negative_) return negative_;
    const int magnitude = compareMagnitude(digits_, other.digits_);
    return negative_ ? magnitude > 0 : magnitude < 0;
}
bool BigInteger::operator>(const BigInteger& other) const { return other < *this; }

std::ostream& operator<<(std::ostream& output, const BigInteger& value) {
    if (value.negative_) output << '-';
    return output << value.digits_;
}

} // namespace turbocore
