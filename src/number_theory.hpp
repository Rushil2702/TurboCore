#pragma once

#include "big_integer.hpp"

#include <cstdint>

namespace turbocore {
namespace number_theory {

std::uint64_t gcd(std::uint64_t left, std::uint64_t right);
BigInteger factorial(unsigned int value);
bool isPrime(std::uint32_t value);

} // namespace number_theory
} // namespace turbocore
