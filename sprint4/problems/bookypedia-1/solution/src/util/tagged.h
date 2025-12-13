#pragma once

namespace util {

template <typename Value, typename Tag>
class Tagged {
public:
    constexpr Tagged() = default;
    explicit constexpr Tagged(Value value)
        : value_{value} {
    }

    constexpr const Value& operator*() const noexcept {
        return value_;
    }

    constexpr Value& operator*() noexcept {
        return value_;
    }

    constexpr bool operator==(const Tagged& other) const noexcept {
        return value_ == other.value_;
    }

    constexpr auto operator<=>(const Tagged&) const = default;

protected:
    Value value_{};
};

}  // namespace util

