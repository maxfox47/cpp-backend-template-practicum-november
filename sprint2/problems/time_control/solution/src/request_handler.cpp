#include "request_handler.h"
#include "model/position.h"
#include "model/dog.h"
#include <sstream>
#include <cmath>

namespace http_handler {

void RequestHandler::LoadConfig() {
    std::ifstream file(config_path_);
    if (!file) {
        throw std::runtime_error("Cannot open config file: " + config_path_);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    config_data_ = json::parse(content);
}

bool RequestHandler::IsSubPath(fs::path path) const {
    path = fs::weakly_canonical(path);
    fs::path base = fs::weakly_canonical(base_path_);

    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

std::string RequestHandler::GetMimeType(const std::string& extension) {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    static const std::unordered_map<std::string, std::string> mime_types = {
        {".htm", "text/html"},
        {".html", "text/html"},
        {".css", "text/css"},
        {".txt", "text/plain"},
        {".js", "text/javascript"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpe", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".ico", "image/vnd.microsoft.icon"},
        {".tiff", "image/tiff"},
        {".tif", "image/tiff"},
        {".svg", "image/svg+xml"},
        {".svgz", "image/svg+xml"},
        {".mp3", "audio/mpeg"}
    };

    auto it = mime_types.find(ext);
    if (it != mime_types.end()) {
        return it->second;
    }

    return "application/octet-stream";
}

std::string RequestHandler::UrlDecode(std::string_view str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int hex_value;
            if (sscanf(str.substr(i + 1, 2).data(), "%2x", &hex_value) == 1) {
                result += static_cast<char>(hex_value);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }

    return result;
}

std::optional<util::Token> RequestHandler::TryExtractToken(const StringRequest& request) const {
    auto auth_it = request.find(http::field::authorization);
    if (auth_it == request.end()) {
        return std::nullopt;
    }
    
    std::string auth_header = std::string(auth_it->value());
    if (!auth_header.starts_with("Bearer ")) {
        return std::nullopt;
    }
    
    std::string token_str = auth_header.substr(7); // Убираем "Bearer "
    
    // Проверяем длину токена (должен быть 32 hex-символа)
    if (token_str.length() != 32) {
        return std::nullopt;
    }
    
    return util::Token(token_str);
}

StringResponse RequestHandler::MakeUnauthorizedError(unsigned version, bool keep_alive, const std::string& code, const std::string& message) const {
    StringResponse res{http::status::unauthorized, version};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::cache_control, "no-cache");
    res.keep_alive(keep_alive);
    res.body() = json::serialize(json::object{
        {"code", code},
        {"message", message}
    });
    res.prepare_payload();
    return res;
}

StringResponse RequestHandler::HandleApiRequest(const StringRequest& request) const {
    std::string target_str = std::string(request.target());
    auto version = request.version();
    auto keep_alive = request.keep_alive();
    
    // POST /api/v1/game/join
    if (target_str == "/api/v1/game/join" && request.method() == http::verb::post) {
        // Проверяем Content-Type
        auto content_type_it = request.find(http::field::content_type);
        if (content_type_it == request.end() || 
            std::string(content_type_it->value()) != "application/json") {
            StringResponse res{http::status::bad_request, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{
                {"code", "invalidArgument"},
                {"message", "Content-Type must be application/json"}
            });
            res.prepare_payload();
            return res;
        }
        
        // Парсим JSON
        json::value json_body;
        try {
            json_body = json::parse(request.body());
        } catch (const std::exception&) {
            StringResponse res{http::status::bad_request, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{
                {"code", "invalidArgument"},
                {"message", "Join game request parse error"}
            });
            res.prepare_payload();
            return res;
        }
        
        // Проверяем наличие полей
        if (!json_body.is_object() || 
            !json_body.as_object().contains("userName") ||
            !json_body.as_object().contains("mapId")) {
            StringResponse res{http::status::bad_request, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{
                {"code", "invalidArgument"},
                {"message", "Join game request parse error"}
            });
            res.prepare_payload();
            return res;
        }
        
        std::string user_name = std::string(json_body.as_object().at("userName").as_string());
        std::string map_id = std::string(json_body.as_object().at("mapId").as_string());
        
        // Проверяем имя
        if (user_name.empty()) {
            StringResponse res{http::status::bad_request, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{
                {"code", "invalidArgument"},
                {"message", "Invalid name"}
            });
            res.prepare_payload();
            return res;
        }
        
        // Пытаемся присоединиться к игре
        auto join_result = game_.JoinGame(user_name, map_id, players_, token_generator_);
        
        if (!join_result) {
            StringResponse res{http::status::not_found, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{
                {"code", "mapNotFound"},
                {"message", "Map not found"}
            });
            res.prepare_payload();
            return res;
        }
        
        // Успешный ответ
        StringResponse res{http::status::ok, version};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.keep_alive(keep_alive);
        res.body() = json::serialize(json::object{
            {"authToken", join_result->token.GetValue()},
            {"playerId", join_result->player_id}
        });
        res.prepare_payload();
        return res;
    }
    
    // POST /api/v1/game/player/action
    if (target_str == "/api/v1/game/player/action" && request.method() == http::verb::post) {
        // Проверяем Content-Type
        auto content_type_it = request.find(http::field::content_type);
        if (content_type_it == request.end() || 
            std::string(content_type_it->value()) != "application/json") {
            StringResponse res{http::status::bad_request, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{
                {"code", "invalidArgument"},
                {"message", "Invalid content type"}
            });
            res.prepare_payload();
            return res;
        }
        
        std::string request_body = request.body();
        return ExecuteAuthorized(request, [this, version, keep_alive, request_body](const util::Token& token, const model::Player& player) {
            // Парсим JSON
            json::value json_body;
            try {
                json_body = json::parse(request_body);
            } catch (const std::exception&) {
                StringResponse res{http::status::bad_request, version};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.keep_alive(keep_alive);
                res.body() = json::serialize(json::object{
                    {"code", "invalidArgument"},
                    {"message", "Failed to parse action"}
                });
                res.prepare_payload();
                return res;
            }
            
            // Проверяем наличие поля move
            if (!json_body.is_object() || !json_body.as_object().contains("move")) {
                StringResponse res{http::status::bad_request, version};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.keep_alive(keep_alive);
                res.body() = json::serialize(json::object{
                    {"code", "invalidArgument"},
                    {"message", "Failed to parse action"}
                });
                res.prepare_payload();
                return res;
            }
            
            std::string move_str = std::string(json_body.as_object().at("move").as_string());
            
            // Находим игровую сессию
            auto session = game_.FindSessionByPlayer(player.GetId());
            if (!session) {
                StringResponse res{http::status::internal_server_error, version};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.keep_alive(keep_alive);
                res.body() = json::serialize(json::object{
                    {"code", "internalError"},
                    {"message", "Game session not found"}
                });
                res.prepare_payload();
                return res;
            }
            
            // Находим собаку игрока
            auto dog = session->GetDog(player.GetDogId());
            if (!dog) {
                StringResponse res{http::status::internal_server_error, version};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.keep_alive(keep_alive);
                res.body() = json::serialize(json::object{
                    {"code", "internalError"},
                    {"message", "Dog not found"}
                });
                res.prepare_payload();
                return res;
            }
            
            // Получаем скорость персонажа на карте
            double dog_speed = session->GetMap().GetDogSpeed();
            
            // Определяем новую скорость и направление
            model::Speed new_speed(0.0, 0.0);
            model::Direction new_direction = dog->GetDirection();
            
            if (move_str == "L") {
                new_speed = model::Speed(-dog_speed, 0.0);
                new_direction = model::Direction::WEST;
            } else if (move_str == "R") {
                new_speed = model::Speed(dog_speed, 0.0);
                new_direction = model::Direction::EAST;
            } else if (move_str == "U") {
                new_speed = model::Speed(0.0, -dog_speed);
                new_direction = model::Direction::NORTH;
            } else if (move_str == "D") {
                new_speed = model::Speed(0.0, dog_speed);
                new_direction = model::Direction::SOUTH;
            } else if (move_str == "") {
                new_speed = model::Speed(0.0, 0.0);
                // Направление не меняем при остановке
                new_direction = dog->GetDirection();
            } else {
                StringResponse res{http::status::bad_request, version};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.keep_alive(keep_alive);
                res.body() = json::serialize(json::object{
                    {"code", "invalidArgument"},
                    {"message", "Failed to parse action"}
                });
                res.prepare_payload();
                return res;
            }
            
            // Обновляем скорость и направление собаки
            dog->SetSpeed(new_speed);
            dog->SetDirection(new_direction);
            
            // Успешный ответ
            StringResponse res{http::status::ok, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{});
            res.prepare_payload();
            return res;
        });
    }
    
    // POST /api/v1/game/tick
    if (target_str == "/api/v1/game/tick" && request.method() == http::verb::post) {
        // Проверяем Content-Type
        auto content_type_it = request.find(http::field::content_type);
        if (content_type_it == request.end() || 
            std::string(content_type_it->value()) != "application/json") {
            StringResponse res{http::status::bad_request, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{
                {"code", "invalidArgument"},
                {"message", "Invalid content type"}
            });
            res.prepare_payload();
            return res;
        }
        
        // Парсим JSON
        json::value json_body;
        try {
            json_body = json::parse(request.body());
        } catch (const std::exception&) {
            StringResponse res{http::status::bad_request, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{
                {"code", "invalidArgument"},
                {"message", "Failed to parse tick request JSON"}
            });
            res.prepare_payload();
            return res;
        }
        
        // Проверяем наличие поля timeDelta
        if (!json_body.is_object() || !json_body.as_object().contains("timeDelta")) {
            StringResponse res{http::status::bad_request, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{
                {"code", "invalidArgument"},
                {"message", "Failed to parse tick request JSON"}
            });
            res.prepare_payload();
            return res;
        }
        
        // Получаем timeDelta
        const auto& time_delta_val = json_body.as_object().at("timeDelta");
        if (!time_delta_val.is_number()) {
            StringResponse res{http::status::bad_request, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{
                {"code", "invalidArgument"},
                {"message", "Failed to parse tick request JSON"}
            });
            res.prepare_payload();
            return res;
        }
        
        // Проверяем, что это целое число (не float)
        // В JSON числа могут быть представлены как int или double
        // Проверяем, является ли число целым, сравнивая его с округленным значением
        double time_delta_double = time_delta_val.to_number<double>();
        int64_t time_delta_ms_int = static_cast<int64_t>(std::round(time_delta_double));
        // Если разница между исходным значением и округленным больше погрешности, это не целое число
        // Также отклоняем 0.0 (float с нулевой дробной частью)
        double diff = std::abs(time_delta_double - static_cast<double>(time_delta_ms_int));
        if (diff > 1e-9 || (time_delta_double == 0.0 && !time_delta_val.is_int64())) {
            // Это не целое число или 0.0 как float
            StringResponse res{http::status::bad_request, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{
                {"code", "invalidArgument"},
                {"message", "Failed to parse tick request JSON"}
            });
            res.prepare_payload();
            return res;
        }
        
        if (time_delta_ms_int < 0) {
            StringResponse res{http::status::bad_request, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{
                {"code", "invalidArgument"},
                {"message", "Failed to parse tick request JSON"}
            });
            res.prepare_payload();
            return res;
        }
        
        double time_delta_ms = static_cast<double>(time_delta_ms_int);
        
        // Обновляем игровое состояние
        game_.Tick(time_delta_ms);
        
        // Успешный ответ
        StringResponse res{http::status::ok, version};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.keep_alive(keep_alive);
        res.body() = json::serialize(json::object{});
        res.prepare_payload();
        return res;
    }
    
    // GET /api/v1/game/state
    if (target_str == "/api/v1/game/state" && 
        (request.method() == http::verb::get || request.method() == http::verb::head)) {
        return ExecuteAuthorized(request, [this, version, keep_alive](const util::Token& token, const model::Player& player) {
            // Находим игровую сессию
            auto session = game_.FindSessionByPlayer(player.GetId());
            if (!session) {
                StringResponse res{http::status::internal_server_error, version};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.keep_alive(keep_alive);
                res.body() = json::serialize(json::object{
                    {"code", "internalError"},
                    {"message", "Game session not found"}
                });
                res.prepare_payload();
                return res;
            }
            
            // Формируем состояние игроков
            json::object players_json;
            const auto& dogs = session->GetDogs();
            
            for (const auto& [player_id, session_player] : session->GetPlayers()) {
                auto dog_it = dogs.find(session_player->GetDogId());
                if (dog_it == dogs.end()) {
                    continue;
                }
                
                const auto& dog = dog_it->second;
                const auto& pos = dog->GetPosition();
                const auto& speed = dog->GetSpeed();
                
                // Конвертируем направление в строку
                std::string dir_str;
                switch (dog->GetDirection()) {
                    case model::Direction::NORTH:
                        dir_str = "U";
                        break;
                    case model::Direction::SOUTH:
                        dir_str = "D";
                        break;
                    case model::Direction::WEST:
                        dir_str = "L";
                        break;
                    case model::Direction::EAST:
                        dir_str = "R";
                        break;
                }
                
                players_json[std::to_string(player_id)] = json::object{
                    {"pos", json::array{pos.x, pos.y}},
                    {"speed", json::array{speed.vx, speed.vy}},
                    {"dir", dir_str}
                };
            }
            
            StringResponse res{http::status::ok, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{
                {"players", players_json}
            });
            res.prepare_payload();
            return res;
        });
    }
    
    // GET /api/v1/game/players
    if (target_str == "/api/v1/game/players" && 
        (request.method() == http::verb::get || request.method() == http::verb::head)) {
        return ExecuteAuthorized(request, [this, version, keep_alive](const util::Token& token, const model::Player& player) {
            // Находим игровую сессию
            auto session = game_.FindSessionByPlayer(player.GetId());
            if (!session) {
                StringResponse res{http::status::internal_server_error, version};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.keep_alive(keep_alive);
                res.body() = json::serialize(json::object{
                    {"code", "internalError"},
                    {"message", "Game session not found"}
                });
                res.prepare_payload();
                return res;
            }
            
            // Формируем список игроков
            json::object players_json;
            for (const auto& [id, session_player] : session->GetPlayers()) {
                players_json[std::to_string(id)] = json::object{
                    {"name", session_player->GetName()}
                };
            }
            
            StringResponse res{http::status::ok, version};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(players_json);
            res.prepare_payload();
            return res;
        });
    }
    
    // GET /api/v1/maps
    if (target_str == "/api/v1/maps" && request.method() == http::verb::get) {
        json::array maps_array;
        
        if (config_data_.as_object().contains("maps") && config_data_.as_object().at("maps").is_array()) {
            for (const auto& map_val : config_data_.as_object().at("maps").as_array()) {
                const auto& map_obj = map_val.as_object();
                maps_array.push_back(json::object{
                    {"id", map_obj.at("id")},
                    {"name", map_obj.at("name")}
                });
            }
        }

        StringResponse res{http::status::ok, version};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.keep_alive(keep_alive);
        res.body() = json::serialize(maps_array);
        res.prepare_payload();
        return res;
    }

    // GET /api/v1/maps/{id}
    if (target_str.starts_with("/api/v1/maps/") && request.method() == http::verb::get) {
        std::string map_id = target_str.substr(13); // Убираем "/api/v1/maps/"

        if (!map_id.empty() && map_id.back() == '/') {
            map_id.pop_back();
        }

        const json::value* found_map = nullptr;
        if (config_data_.as_object().contains("maps") && config_data_.as_object().at("maps").is_array()) {
            for (const auto& map_val : config_data_.as_object().at("maps").as_array()) {
                const auto& map_obj = map_val.as_object();
                if (map_obj.contains("id") && std::string(map_obj.at("id").as_string()) == map_id) {
                    found_map = &map_val;
                    break;
                }
            }
        }

        if (!found_map) {
            StringResponse res{http::status::not_found, version};
            res.set(http::field::content_type, "application/json");
            res.keep_alive(keep_alive);
            res.body() = json::serialize(json::object{
                {"code", "mapNotFound"},
                {"message", "Map not found"}
            });
            res.prepare_payload();
            return res;
        }

        // Создаем копию объекта карты без поля dogSpeed
        json::object map_obj = found_map->as_object();
        json::object filtered_map;
        for (const auto& [key, value] : map_obj) {
            if (key != "dogSpeed") {
                filtered_map[key] = value;
            }
        }

        StringResponse res{http::status::ok, version};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.keep_alive(keep_alive);
        res.body() = json::serialize(filtered_map);
        res.prepare_payload();
        return res;
    }
    
    // Method not allowed для известных API endpoints
    if (target_str == "/api/v1/game/join" || 
        target_str == "/api/v1/game/player/action" ||
        target_str == "/api/v1/game/tick" ||
        target_str == "/api/v1/game/players" ||
        target_str == "/api/v1/game/state" ||
        target_str == "/api/v1/maps" ||
        target_str.starts_with("/api/v1/maps/")) {
        StringResponse res{http::status::method_not_allowed, version};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.keep_alive(keep_alive);
        
        if (target_str == "/api/v1/game/join") {
            res.set(http::field::allow, "POST");
            res.body() = json::serialize(json::object{
                {"code", "invalidMethod"},
                {"message", "Only POST method is expected"}
            });
        } else if (target_str == "/api/v1/game/player/action") {
            res.set(http::field::allow, "POST");
            res.body() = json::serialize(json::object{
                {"code", "invalidMethod"},
                {"message", "Invalid method"}
            });
        } else if (target_str == "/api/v1/game/tick") {
            res.set(http::field::allow, "POST");
            res.body() = json::serialize(json::object{
                {"code", "invalidMethod"},
                {"message", "Invalid method"}
            });
        } else if (target_str == "/api/v1/game/players") {
            res.set(http::field::allow, "GET, HEAD");
            res.body() = json::serialize(json::object{
                {"code", "invalidMethod"},
                {"message", "Invalid method"}
            });
        } else if (target_str == "/api/v1/game/state") {
            res.set(http::field::allow, "GET, HEAD");
            res.body() = json::serialize(json::object{
                {"code", "invalidMethod"},
                {"message", "Invalid method"}
            });
        } else if (target_str == "/api/v1/maps") {
            res.set(http::field::allow, "GET");
            res.body() = json::serialize(json::object{
                {"code", "invalidMethod"},
                {"message", "Invalid method"}
            });
        } else if (target_str.starts_with("/api/v1/maps/")) {
            res.set(http::field::allow, "GET");
            res.body() = json::serialize(json::object{
                {"code", "invalidMethod"},
                {"message", "Invalid method"}
            });
        }
        res.prepare_payload();
        return res;
    }
    
    // Bad request для неизвестных API путей
    if (target_str.starts_with("/api/")) {
        StringResponse res{http::status::bad_request, version};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.keep_alive(keep_alive);
        res.body() = json::serialize(json::object{
            {"code", "badRequest"},
            {"message", "Bad request"}
        });
        res.prepare_payload();
        return res;
    }
    
    // Bad request для всех остальных запросов
    StringResponse res{http::status::bad_request, version};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::cache_control, "no-cache");
    res.keep_alive(keep_alive);
    res.body() = json::serialize(json::object{
        {"code", "badRequest"},
        {"message", "Bad request"}
    });
    res.prepare_payload();
    return res;
}

StringResponse RequestHandler::HandleFileRequest(const StringRequest& req) const {
    std::string target_str = std::string(req.target());
    std::string decoded_path = UrlDecode(target_str);

    if (decoded_path.empty() || decoded_path.back() == '/') {
        decoded_path += "index.html";
    }

    if (decoded_path.front() == '/') {
        decoded_path = decoded_path.substr(1);
    }

    fs::path file_path = fs::path(base_path_) / decoded_path;

    if (!IsSubPath(file_path)) {
        StringResponse res{http::status::bad_request, req.version()};
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = "Invalid path";
        res.prepare_payload();
        return res;
    }

    if (!fs::exists(file_path) || !fs::is_regular_file(file_path)) {
        StringResponse res{http::status::not_found, req.version()};
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = "File not found";
        res.prepare_payload();
        return res;
    }

    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        StringResponse res{http::status::not_found, req.version()};
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = "File not found";
        res.prepare_payload();
        return res;
    }

    std::string content;
    file.seekg(0, std::ios::end);
    content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&content[0], content.size());

    std::string content_type = GetMimeType(file_path.extension().string());

    StringResponse res{http::status::ok, req.version()};
    res.set(http::field::content_type, content_type);
    res.set(http::field::content_length, std::to_string(content.size()));
    res.keep_alive(req.keep_alive());

    if (req.method() == http::verb::get) {
        res.body() = std::move(content);
    }

    res.prepare_payload();
    return res;
}

StringResponse RequestHandler::ReportServerError(unsigned version, bool keep_alive) const {
    StringResponse res{http::status::internal_server_error, version};
    res.set(http::field::content_type, "application/json");
    res.keep_alive(keep_alive);
    res.body() = json::serialize(json::object{
        {"code", "internalError"},
        {"message", "Internal server error"}
    });
    res.prepare_payload();
    return res;
}

} // namespace http_handler

