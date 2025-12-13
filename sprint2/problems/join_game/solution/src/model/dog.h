#pragma once

#include <cstdint>
#include <string>

namespace model {

using DogId = std::uint32_t;

class Dog {
public:
    Dog(DogId id, std::string name) : id_(id), name_(std::move(name)) {}
    
    DogId GetId() const { return id_; }
    const std::string& GetName() const { return name_; }

private:
    DogId id_;
    std::string name_;
};

} // namespace model





