#pragma once

#include <cstdint>
#include <string>
#include "model/dog.h"
#include "util/token.h"

namespace model {

using PlayerId = std::uint32_t;

class Player {
public:
    Player(PlayerId id, std::string name, util::Token token, DogId dog_id)
        : id_(id), name_(std::move(name)), token_(std::move(token)), dog_id_(dog_id) {}
    
    PlayerId GetId() const { return id_; }
    const std::string& GetName() const { return name_; }
    const util::Token& GetToken() const { return token_; }
    DogId GetDogId() const { return dog_id_; }

private:
    PlayerId id_;
    std::string name_;
    util::Token token_;
    DogId dog_id_;
};

} // namespace model
