#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <random>
#include <cmath>
#include <boost/json.hpp>
#include "model/dog.h"
#include "model/player.h"
#include "model/position.h"

namespace model {

struct Road {
    Position start;
    Position end;
    
    Road(Position start, Position end) : start(start), end(end) {}
    
    double GetLength() const {
        double dx = end.x - start.x;
        double dy = end.y - start.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    Position GetRandomPoint(std::mt19937& gen) const {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        double t = dist(gen);
        return Position(
            start.x + t * (end.x - start.x),
            start.y + t * (end.y - start.y)
        );
    }
};

class Map {
public:
    Map(boost::json::value json_data, double default_dog_speed) 
        : json_data_(std::move(json_data)), default_dog_speed_(default_dog_speed) {
        LoadRoads();
        LoadDogSpeed();
    }
    
    std::string GetId() const {
        return std::string(json_data_.as_object().at("id").as_string());
    }
    
    bool HasId(const std::string& id) const {
        if (!json_data_.as_object().contains("id")) return false;
        return std::string(json_data_.as_object().at("id").as_string()) == id;
    }
    
    const boost::json::value& GetJsonData() const { return json_data_; }
    
    const std::vector<Road>& GetRoads() const { return roads_; }
    
    double GetDogSpeed() const { return dog_speed_; }
    
    Position GetRandomRoadPosition(std::mt19937& gen) const {
        if (roads_.empty()) {
            return Position(0.0, 0.0);
        }
        
        std::uniform_int_distribution<size_t> road_dist(0, roads_.size() - 1);
        size_t road_index = road_dist(gen);
        return roads_[road_index].GetRandomPoint(gen);
    }

private:
    void LoadRoads() {
        if (!json_data_.as_object().contains("roads")) {
            return;
        }
        
        const auto& roads_array = json_data_.as_object().at("roads").as_array();
        for (const auto& road_val : roads_array) {
            const auto& road_obj = road_val.as_object();
            
            int x0 = road_obj.at("x0").as_int64();
            int y0 = road_obj.at("y0").as_int64();
            
            Position start(static_cast<double>(x0), static_cast<double>(y0));
            Position end;
            
            if (road_obj.contains("x1")) {
                // Горизонтальная дорога
                int x1 = road_obj.at("x1").as_int64();
                end = Position(static_cast<double>(x1), static_cast<double>(y0));
            } else if (road_obj.contains("y1")) {
                // Вертикальная дорога
                int y1 = road_obj.at("y1").as_int64();
                end = Position(static_cast<double>(x0), static_cast<double>(y1));
            } else {
                continue; // Невалидная дорога
            }
            
            roads_.emplace_back(start, end);
        }
    }
    
    void LoadDogSpeed() {
        const auto& map_obj = json_data_.as_object();
        if (map_obj.contains("dogSpeed") && map_obj.at("dogSpeed").is_number()) {
            dog_speed_ = map_obj.at("dogSpeed").as_double();
        } else {
            dog_speed_ = default_dog_speed_;
        }
    }
    
    boost::json::value json_data_;
    std::vector<Road> roads_;
    double default_dog_speed_;
    double dog_speed_;
};

} // namespace model

