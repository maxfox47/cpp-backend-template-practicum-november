#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include "model/dog.h"
#include "model/player.h"
#include "model/map.h"

namespace model {

class GameSession {
public:
    explicit GameSession(Map map) : map_(std::move(map)) {}
    
    const Map& GetMap() const { return map_; }
    
    void AddDog(std::shared_ptr<Dog> dog) {
        dogs_[dog->GetId()] = dog;
    }
    
    void AddPlayer(std::shared_ptr<Player> player) {
        players_[player->GetId()] = player;
    }
    
    std::shared_ptr<Dog> GetDog(DogId id) const {
        auto it = dogs_.find(id);
        if (it != dogs_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    std::shared_ptr<Player> GetPlayer(PlayerId id) const {
        auto it = players_.find(id);
        if (it != players_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    std::unordered_map<PlayerId, std::shared_ptr<Player>> GetPlayers() const {
        return players_;
    }
    
    const std::unordered_map<DogId, std::shared_ptr<Dog>>& GetDogs() const {
        return dogs_;
    }
    
    void Tick(double delta_time_ms) {
        for (auto& [dog_id, dog] : dogs_) {
            auto [new_pos, should_stop] = map_.MoveDog(dog->GetPosition(), dog->GetSpeed(), delta_time_ms);
            dog->SetPosition(new_pos);
            if (should_stop) {
                dog->SetSpeed(model::Speed(0.0, 0.0));
            }
        }
    }

private:
    Map map_;
    std::unordered_map<DogId, std::shared_ptr<Dog>> dogs_;
    std::unordered_map<PlayerId, std::shared_ptr<Player>> players_;
};

} // namespace model

