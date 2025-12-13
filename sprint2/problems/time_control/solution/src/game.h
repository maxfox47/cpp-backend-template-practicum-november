#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <optional>
#include <random>
#include <boost/json.hpp>
#include "model/map.h"
#include "model/game_session.h"
#include "model/dog.h"
#include "model/player.h"
#include "model/position.h"
#include "application/players.h"
#include "util/token.h"

namespace application {

class Game {
public:
    explicit Game(boost::json::value config_data) {
        LoadDefaultDogSpeed(config_data);
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
        
        // Выбрать случайную позицию на дороге
        std::random_device rd;
        std::mt19937 gen(rd());
        model::Position position = found_map->GetRandomRoadPosition(gen);
        
        // Создать собаку с нулевой скоростью и направлением на север
        model::Speed speed(0.0, 0.0);
        model::Direction direction = model::Direction::NORTH;
        auto dog = std::make_shared<model::Dog>(next_dog_id_++, user_name, position, speed, direction);
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
    
    std::shared_ptr<model::GameSession> FindSessionByToken(const util::Token& token, const Players& players) const {
        auto player = players.FindByToken(token);
        if (!player) {
            return nullptr;
        }
        return FindSessionByPlayer(player->GetId());
    }
    
    double GetDefaultDogSpeed() const { return default_dog_speed_; }

private:
    void LoadDefaultDogSpeed(const boost::json::value& config_data) {
        if (config_data.as_object().contains("defaultDogSpeed") && 
            config_data.as_object().at("defaultDogSpeed").is_number()) {
            default_dog_speed_ = config_data.as_object().at("defaultDogSpeed").as_double();
        } else {
            default_dog_speed_ = 1.0;
        }
    }
    
    void LoadMaps(const boost::json::value& config_data) {
        if (config_data.as_object().contains("maps") && config_data.as_object().at("maps").is_array()) {
            for (const auto& map_val : config_data.as_object().at("maps").as_array()) {
                maps_.emplace_back(map_val, default_dog_speed_);
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
    
    double default_dog_speed_ = 1.0;
    std::vector<model::Map> maps_;
    std::unordered_map<std::string, std::shared_ptr<model::GameSession>> sessions_;
    model::DogId next_dog_id_ = 0;
};

} // namespace application

