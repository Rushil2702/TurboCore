#include "number_theory.hpp"

#include <stdexcept>
#include <string>

namespace turbocore {
namespace number_theory {

std::uint64_t gcd(std::uint64_t left, std::uint64_t right) {
    while (right != 0) {
        const std::uint64_t remainder = left % right;
        left = right;
        right = remainder;
    }
    return left;
}

BigInteger factorial(unsigned int value) {
    // Bound work and output size for the interactive executable.
    if (value > 1000) throw std::out_of_range("Factorial input is limited to 1000");
    BigInteger result(1LL);
    for (unsigned int i = 2; i <= value; ++i) result = result * BigInteger(static_cast<long long>(i));
    return result;
}

bool isPrime(std::uint32_t value) {
    if (value < 2) return false;
    if (value % 2 == 0) return value == 2;
    for (std::uint64_t divisor = 3; divisor <= value / divisor; divisor += 2)
        if (value % divisor == 0) return false;
    return true;
}

} // namespace number_theory
} // namespace turbocore
