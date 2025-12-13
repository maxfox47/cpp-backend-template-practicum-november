#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <random>
#include <sstream>
#include <iomanip>
#include "util/tagged.h"

namespace detail {
struct TokenTag {};
} // namespace detail

namespace util {

using Token = Tagged<detail::TokenTag>;

class PlayerTokens {
public:
    PlayerTokens() : generator1_(GetRandomSeed()), generator2_(GetRandomSeed()) {}
    
    Token GenerateToken() {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        
        uint64_t part1 = dist(generator1_);
        uint64_t part2 = dist(generator2_);
        
        std::ostringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(16) << part1
            << std::setw(16) << part2;
        
        return Token(oss.str());
    }

private:
    static std::mt19937_64::result_type GetRandomSeed() {
        std::random_device rd;
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(rd);
    }
    
    std::mt19937_64 generator1_;
    std::mt19937_64 generator2_;
};

} // namespace util
