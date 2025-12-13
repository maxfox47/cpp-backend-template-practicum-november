#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <optional>
#include "model/player.h"
#include "model/game_session.h"
#include "util/token.h"

namespace application {

class Players {
public:
    Players() = default;
    
    std::shared_ptr<model::Player> AddPlayer(std::string name, util::Token token, model::DogId dog_id) {
        auto player = std::make_shared<model::Player>(next_player_id_++, std::move(name), std::move(token), dog_id);
        players_[player->GetId()] = player;
        return player;
    }
    
    std::shared_ptr<model::Player> FindByToken(const util::Token& token) const {
        for (const auto& [id, player] : players_) {
            if (player->GetToken().GetValue() == token.GetValue()) {
                return player;
            }
        }
        return nullptr;
    }
    
    std::shared_ptr<model::Player> FindById(model::PlayerId id) const {
        auto it = players_.find(id);
        if (it != players_.end()) {
            return it->second;
        }
        return nullptr;
    }

private:
    model::PlayerId next_player_id_ = 0;
    std::unordered_map<model::PlayerId, std::shared_ptr<model::Player>> players_;
};

} // namespace application

