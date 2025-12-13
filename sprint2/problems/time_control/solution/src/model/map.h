#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <random>
#include <cmath>
#include <limits>
#include <utility>
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
    
    Position GetFirstRoadStartPosition() const {
        if (roads_.empty()) {
            return Position(0.0, 0.0);
        }
        return roads_[0].start;
    }
    
    // Проверяет, находится ли точка на дороге (в пределах 0.4 от оси дороги)
    bool IsOnRoad(const Position& pos) const {
        const double road_width = 0.8;
        const double half_width = road_width / 2.0;
        
        for (const auto& road : roads_) {
            // Проверяем горизонтальную дорогу
            if (road.start.y == road.end.y) {
                double min_x = std::min(road.start.x, road.end.x);
                double max_x = std::max(road.start.x, road.end.x);
                if (pos.x >= min_x - half_width && pos.x <= max_x + half_width &&
                    std::abs(pos.y - road.start.y) <= half_width) {
                    return true;
                }
            }
            // Проверяем вертикальную дорогу
            if (road.start.x == road.end.x) {
                double min_y = std::min(road.start.y, road.end.y);
                double max_y = std::max(road.start.y, road.end.y);
                if (pos.y >= min_y - half_width && pos.y <= max_y + half_width &&
                    std::abs(pos.x - road.start.x) <= half_width) {
                    return true;
                }
            }
        }
        return false;
    }
    
    // Перемещает собаку с учетом границ дорог
    // Возвращает новую позицию и флаг остановки (true если уперлась в границу)
    std::pair<Position, bool> MoveDog(const Position& current_pos, const Speed& speed, double delta_time_ms) const {
        if (speed.vx == 0.0 && speed.vy == 0.0) {
            return {current_pos, false};
        }
        
        const double road_width = 0.8;
        const double half_width = road_width / 2.0;
        const double delta_time_s = delta_time_ms / 1000.0;
        
        // Вычисляем целевую позицию
        Position target_pos(
            current_pos.x + speed.vx * delta_time_s,
            current_pos.y + speed.vy * delta_time_s
        );
        
        // Проверяем, находится ли целевая позиция на дороге
        if (IsOnRoad(target_pos)) {
            return {target_pos, false};
        }
        
        // Если целевая позиция вне дороги, находим точку пересечения с границей
        // Используем бинарный поиск для нахождения ближайшей точки на границе
        Position pos = current_pos;
        double t_min = 0.0;
        double t_max = 1.0;
        const double epsilon = 1e-9;
        
        // Бинарный поиск точки пересечения с границей дороги
        for (int i = 0; i < 100; ++i) {
            double t = (t_min + t_max) / 2.0;
            Position test_pos(
                current_pos.x + speed.vx * delta_time_s * t,
                current_pos.y + speed.vy * delta_time_s * t
            );
            
            if (IsOnRoad(test_pos)) {
                pos = test_pos;
                t_min = t;
            } else {
                t_max = t;
            }
            
            if (t_max - t_min < epsilon) {
                break;
            }
        }
        
        // Проверяем, что найденная позиция действительно на дороге
        if (IsOnRoad(pos)) {
            // Находим ближайшую границу дороги в направлении движения и используем точное значение
            const double road_width = 0.8;
            const double half_width = road_width / 2.0;
            Position exact_pos = pos;
            double min_dist = std::numeric_limits<double>::max();
            const double threshold = 0.1;  // Порог для определения близости к границе (увеличен для учета погрешностей)
            
            // Определяем направление движения для фильтрации границ
            bool moving_right = speed.vx > 0.0;
            bool moving_left = speed.vx < 0.0;
            bool moving_down = speed.vy > 0.0;
            bool moving_up = speed.vy < 0.0;
            
            for (const auto& road : roads_) {
                // Горизонтальная дорога
                if (road.start.y == road.end.y) {
                    double min_x = std::min(road.start.x, road.end.x) - half_width;
                    double max_x = std::max(road.start.x, road.end.x) + half_width;
                    double road_y = road.start.y;
                    double top_boundary = road_y - half_width;
                    double bottom_boundary = road_y + half_width;
                    
                    // Проверяем, находится ли позиция на этой дороге
                    if (pos.x >= min_x && pos.x <= max_x && 
                        pos.y >= top_boundary && pos.y <= bottom_boundary) {
                        
                        // Проецируем позицию на линию границы
                        double proj_x = std::max(min_x, std::min(max_x, pos.x));
                        
                        // Проверяем верхнюю границу только если движемся вверх и граница выше текущей позиции
                        if (moving_up && top_boundary < pos.y) {
                            double dist_top = std::abs(pos.y - top_boundary);
                            if (dist_top < min_dist && dist_top < threshold) {
                                exact_pos = Position(proj_x, top_boundary);
                                min_dist = dist_top;
                            }
                        }
                        
                        // Проверяем нижнюю границу только если движемся вниз и граница ниже текущей позиции
                        if (moving_down && bottom_boundary > pos.y) {
                            double dist_bottom = std::abs(pos.y - bottom_boundary);
                            if (dist_bottom < min_dist && dist_bottom < threshold) {
                                exact_pos = Position(proj_x, bottom_boundary);
                                min_dist = dist_bottom;
                            }
                        }
                        
                        // Проецируем позицию на линию границы
                        double proj_y = std::max(top_boundary, std::min(bottom_boundary, pos.y));
                        
                        // Проверяем левую границу только если движемся влево и граница левее текущей позиции
                        if (moving_left && min_x < pos.x) {
                            double dist_left = std::abs(pos.x - min_x);
                            if (dist_left < min_dist && dist_left < threshold) {
                                exact_pos = Position(min_x, proj_y);
                                min_dist = dist_left;
                            }
                        }
                        
                        // Проверяем правую границу только если движемся вправо и граница правее текущей позиции
                        if (moving_right && max_x > pos.x) {
                            double dist_right = std::abs(pos.x - max_x);
                            if (dist_right < min_dist && dist_right < threshold) {
                                exact_pos = Position(max_x, proj_y);
                                min_dist = dist_right;
                            }
                        }
                    }
                }
                
                // Вертикальная дорога
                if (road.start.x == road.end.x) {
                    double min_y = std::min(road.start.y, road.end.y) - half_width;
                    double max_y = std::max(road.start.y, road.end.y) + half_width;
                    double road_x = road.start.x;
                    double left_boundary = road_x - half_width;
                    double right_boundary = road_x + half_width;
                    
                    // Проверяем, находится ли позиция на этой дороге
                    if (pos.y >= min_y && pos.y <= max_y && 
                        pos.x >= left_boundary && pos.x <= right_boundary) {
                        
                        // Проецируем позицию на линию границы
                        double proj_y = std::max(min_y, std::min(max_y, pos.y));
                        
                        // Проверяем левую границу только если движемся влево и граница левее текущей позиции
                        if (moving_left && left_boundary < pos.x) {
                            double dist_left = std::abs(pos.x - left_boundary);
                            if (dist_left < min_dist && dist_left < threshold) {
                                exact_pos = Position(left_boundary, proj_y);
                                min_dist = dist_left;
                            }
                        }
                        
                        // Проверяем правую границу только если движемся вправо и граница правее текущей позиции
                        if (moving_right && right_boundary > pos.x) {
                            double dist_right = std::abs(pos.x - right_boundary);
                            if (dist_right < min_dist && dist_right < threshold) {
                                exact_pos = Position(right_boundary, proj_y);
                                min_dist = dist_right;
                            }
                        }
                        
                        // Проецируем позицию на линию границы
                        double proj_x = std::max(left_boundary, std::min(right_boundary, pos.x));
                        
                        // Проверяем верхнюю границу только если движемся вверх и граница выше текущей позиции
                        if (moving_up && min_y < pos.y) {
                            double dist_top = std::abs(pos.y - min_y);
                            if (dist_top < min_dist && dist_top < threshold) {
                                exact_pos = Position(proj_x, min_y);
                                min_dist = dist_top;
                            }
                        }
                        
                        // Проверяем нижнюю границу только если движемся вниз и граница ниже текущей позиции
                        if (moving_down && max_y > pos.y) {
                            double dist_bottom = std::abs(pos.y - max_y);
                            if (dist_bottom < min_dist && dist_bottom < threshold) {
                                exact_pos = Position(proj_x, max_y);
                                min_dist = dist_bottom;
                            }
                        }
                    }
                }
            }
            
            // Если нашли ближайшую границу в направлении движения, используем точное значение
            if (min_dist < std::numeric_limits<double>::max()) {
                return {exact_pos, true};
            }
            
            // Если не нашли ближайшую границу, возвращаем найденную позицию
            return {pos, true};
        }
        
        // Если не нашли точку на дороге, остаемся на месте
        return {current_pos, true};
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

