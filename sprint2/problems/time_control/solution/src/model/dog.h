#pragma once

#include <cstdint>
#include <string>
#include "model/position.h"

namespace model {

using DogId = std::uint32_t;

class Dog {
public:
    Dog(DogId id, std::string name, Position position, Speed speed, Direction direction)
        : id_(id), name_(std::move(name)), position_(position), speed_(speed), direction_(direction) {}
    
    DogId GetId() const { return id_; }
    const std::string& GetName() const { return name_; }
    const Position& GetPosition() const { return position_; }
    const Speed& GetSpeed() const { return speed_; }
    Direction GetDirection() const { return direction_; }
    
    void SetPosition(Position position) { position_ = position; }
    void SetSpeed(Speed speed) { speed_ = speed; }
    void SetDirection(Direction direction) { direction_ = direction; }

private:
    DogId id_;
    std::string name_;
    Position position_;
    Speed speed_;
    Direction direction_;
};

} // namespace model

