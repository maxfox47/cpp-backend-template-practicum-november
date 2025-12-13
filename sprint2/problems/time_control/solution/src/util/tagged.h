#pragma once

#include <string>
#include <cstdint>

namespace util {

template <typename Tag>
class Tagged {
public:
    using Type = std::string;
    
    explicit Tagged(Type value) : value_(std::move(value)) {}
    
    const Type& GetValue() const { return value_; }
    Type& GetValue() { return value_; }
    
    bool operator==(const Tagged& other) const { return value_ == other.value_; }
    bool operator!=(const Tagged& other) const { return value_ != other.value_; }

private:
    Type value_;
};

} // namespace util

