#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <optional>
#include <boost/json.hpp>
#include "model/map.h"
#include "model/game_session.h"
#include "model/dog.h"
#include "model/player.h"
#include "application/players.h"
#include "util/token.h"

namespace application {

class Game {
public:
    explicit Game(boost::json::value config_data) {
        LoadMaps(config_data);
    }
    
    struct JoinResult {
        model::PlayerId player_id;
        util::Token token;
        model::DogId dog_id;
    };
    
    std::optional<JoinResult> JoinGame(const std::string& user_name, const std::string& map_id, Players& players, util::PlayerTokens& token_generator) {
        // Найти карту
        model::Map* found_map = nullptr;
        for (auto& map : maps_) {
            if (map.HasId(map_id)) {
                found_map = &map;
                break;
            }
        }
        
        if (!found_map) {
            return std::nullopt;
        }
        
        // Найти или создать игровую сессию
        auto& session = GetOrCreateSession(*found_map);
        
        // Создать собаку
        auto dog = std::make_shared<model::Dog>(next_dog_id_++, user_name);
        session.AddDog(dog);
        
        // Создать игрока
        util::Token token = token_generator.GenerateToken();
        auto player = players.AddPlayer(user_name, token, dog->GetId());
        session.AddPlayer(player);
        
        return JoinResult{player->GetId(), token, dog->GetId()};
    }
    
    std::shared_ptr<model::GameSession> FindSessionByPlayer(model::PlayerId player_id) const {
        for (const auto& [map_id, session] : sessions_) {
            if (session->GetPlayer(player_id)) {
                return session;
            }
        }
        return nullptr;
    }

private:
    void LoadMaps(const boost::json::value& config_data) {
        if (config_data.as_object().contains("maps") && config_data.as_object().at("maps").is_array()) {
            for (const auto& map_val : config_data.as_object().at("maps").as_array()) {
                maps_.emplace_back(map_val);
            }
        }
    }
    
    model::GameSession& GetOrCreateSession(const model::Map& map) {
        std::string map_id = map.GetId();
        auto it = sessions_.find(map_id);
        if (it == sessions_.end()) {
            sessions_[map_id] = std::make_shared<model::GameSession>(map);
        }
        return *sessions_[map_id];
    }
    
    std::vector<model::Map> maps_;
    std::unordered_map<std::string, std::shared_ptr<model::GameSession>> sessions_;
    model::DogId next_dog_id_ = 0;
};

} // namespace application
