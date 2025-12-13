#include "json_loader.h"
#include <fstream>
#include <sstream>
#include <boost/json.hpp>

namespace json_loader {

namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Читаем весь файл
    std::ifstream file(json_path);
    if (!file) {
        throw std::runtime_error("Cannot open config file: " + json_path.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Парсим JSON
    json::value config = json::parse(content);
    json::object const& obj = config.as_object();

    model::Game game;

    // Обрабатываем карты
    if (obj.contains("maps") && obj.at("maps").is_array()) {
        for (auto const& map_val : obj.at("maps").as_array()) {
            auto const& map_obj = map_val.as_object();

            // Получаем id и name карты
            std::string id = json::value_to<std::string>(map_obj.at("id"));
            std::string name = json::value_to<std::string>(map_obj.at("name"));

            model::Map map(model::Map::Id(id), name);

            // Добавляем дороги
            if (map_obj.contains("roads") && map_obj.at("roads").is_array()) {
                for (auto const& road_val : map_obj.at("roads").as_array()) {
                    auto const& road_obj = road_val.as_object();

                    if (road_obj.contains("x1")) {
                        // Горизонтальная дорога
                        model::Coord x0 = road_obj.at("x0").as_int64();
                        model::Coord y0 = road_obj.at("y0").as_int64();
                        model::Coord x1 = road_obj.at("x1").as_int64();
                        map.AddRoad(model::Road(model::Road::HORIZONTAL,
                                               model::Point{x0, y0}, x1));
                    } else {
                        // Вертикальная дорога
                        model::Coord x0 = road_obj.at("x0").as_int64();
                        model::Coord y0 = road_obj.at("y0").as_int64();
                        model::Coord y1 = road_obj.at("y1").as_int64();
                        map.AddRoad(model::Road(model::Road::VERTICAL,
                                               model::Point{x0, y0}, y1));
                    }
                }
            }

            // Добавляем здания
            if (map_obj.contains("buildings") && map_obj.at("buildings").is_array()) {
                for (auto const& building_val : map_obj.at("buildings").as_array()) {
                    auto const& building_obj = building_val.as_object();

                    model::Coord x = building_obj.at("x").as_int64();
                    model::Coord y = building_obj.at("y").as_int64();
                    model::Dimension w = building_obj.at("w").as_int64();
                    model::Dimension h = building_obj.at("h").as_int64();

                    map.AddBuilding(model::Building(model::Rectangle{
                        model::Point{x, y},
                        model::Size{w, h}
                    }));
                }
            }

            // Добавляем офисы
            if (map_obj.contains("offices") && map_obj.at("offices").is_array()) {
                for (auto const& office_val : map_obj.at("offices").as_array()) {
                    auto const& office_obj = office_val.as_object();

                    std::string office_id = json::value_to<std::string>(office_obj.at("id"));
                    model::Coord x = office_obj.at("x").as_int64();
                    model::Coord y = office_obj.at("y").as_int64();
                    model::Dimension offsetX = office_obj.at("offsetX").as_int64();
                    model::Dimension offsetY = office_obj.at("offsetY").as_int64();

                    map.AddOffice(model::Office(
                        model::Office::Id(office_id),
                        model::Point{x, y},
                        model::Offset{offsetX, offsetY}
                    ));
                }
            }

            game.AddMap(std::move(map));
        }
    }

    return game;
}

}  // namespace json_loader