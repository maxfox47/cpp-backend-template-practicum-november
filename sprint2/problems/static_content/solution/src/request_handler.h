#pragma once
#include "http_server.h"
#include "model.h"
#include <boost/json.hpp>
#include <filesystem>
#include <fstream>
#include <utility>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
using namespace std::literals;
namespace fs = std::filesystem;


class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(model::Game& game, std::string  base_path)
        : game_{game}
        , base_path_{std::move(base_path)} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto const bad_request = [&req](beast::string_view why) {
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            //res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = json::serialize(json::object{
                {"code", "badRequest"},
                {"message", why}
            });
            res.prepare_payload();
            return res;
        };

        auto const not_found = [&req](beast::string_view what) {
            http::response<http::string_body> res{http::status::not_found, req.version()};
            //res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = json::serialize(json::object{
                {"code", "mapNotFound"},
                {"message", what}
            });
            res.prepare_payload();
            return res;
        };

        auto const server_error = [&req](beast::string_view what) {
            http::response<http::string_body> res{http::status::internal_server_error, req.version()};
            //res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = json::serialize(json::object{
                {"code", "internalError"},
                {"message", what}
            });
            res.prepare_payload();
            return res;
        };

        // Обрабатываем только GET и HEAD запросы
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            return send(bad_request("Unsupported HTTP method"));
        }

        try {
            std::string target_str = std::string(req.target());
            // API endpoint для получения списка карт
            if (target_str == "/api/v1/maps") {
                return HandleApiMaps(std::move(req), std::forward<Send>(send));
            }

            // API endpoint для получения конкретной карты по ID
            if (target_str.starts_with("/api/v1/maps/")) {
                return HandleApiMap(std::move(req), std::forward<Send>(send), target_str);
            }

            // Обработка статических файлов
            if (!target_str.starts_with("/api/")) {
                return HandleStaticFile(std::move(req), std::forward<Send>(send), target_str);
            }

            // Если запрос начинается с /api/ но не соответствует известным endpoint'ам
            return send(bad_request("Bad request"));

        } catch (const std::exception& e) {
            return send(server_error(e.what()));
        }
    }
private:
    bool IsSubPath(fs::path path);

    template <typename Body, typename Allocator, typename Send>
    void HandleApiMaps(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        json::array maps_array;
        for (const auto& map : game_.GetMaps()) {
            maps_array.push_back(json::object{
                {"id", *map.GetId()},
                {"name", map.GetName()}
            });
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = json::serialize(maps_array);
        res.prepare_payload();
        return send(std::move(res));
    }


    template <typename Body, typename Allocator, typename Send>
    void HandleApiMap(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, const std::string& target_str) {
        std::string map_id = target_str.substr(13); // Убираем "/api/v1/maps/"

        // Убираем возможные слеши в конце
        if (!map_id.empty() && map_id.back() == '/') {
            map_id.pop_back();
        }

        const model::Map* map = game_.FindMap(model::Map::Id(map_id));
        if (!map) {
            auto const not_found = [&req](beast::string_view what) {
                http::response<http::string_body> res{http::status::not_found, req.version()};
                res.set(http::field::content_type, "application/json");
                res.keep_alive(req.keep_alive());
                res.body() = json::serialize(json::object{
                    {"code", "mapNotFound"},
                    {"message", what}
                });
                res.prepare_payload();
                return res;
            };
            return send(not_found("Map not found"));
        }

        // Формируем JSON для карты
        json::object map_json;
        map_json["id"] = *map->GetId();
        map_json["name"] = map->GetName();

        // Добавляем дороги
        json::array roads_array;
        for (const auto& road : map->GetRoads()) {
            if (road.IsHorizontal()) {
                roads_array.push_back(json::object{
                    {"x0", road.GetStart().x},
                    {"y0", road.GetStart().y},
                    {"x1", road.GetEnd().x}
                });
            } else {
                roads_array.push_back(json::object{
                    {"x0", road.GetStart().x},
                    {"y0", road.GetStart().y},
                    {"y1", road.GetEnd().y}
                });
            }
        }
        map_json["roads"] = std::move(roads_array);

        // Добавляем здания
        json::array buildings_array;
        for (const auto& building : map->GetBuildings()) {
            const auto& bounds = building.GetBounds();
            buildings_array.push_back(json::object{
                {"x", bounds.position.x},
                {"y", bounds.position.y},
                {"w", bounds.size.width},
                {"h", bounds.size.height}
            });
        }
        map_json["buildings"] = std::move(buildings_array);

        // Добавляем офисы
        json::array offices_array;
        for (const auto& office : map->GetOffices()) {
            offices_array.push_back(json::object{
                {"id", *office.GetId()},
                {"x", office.GetPosition().x},
                {"y", office.GetPosition().y},
                {"offsetX", office.GetOffset().dx},
                {"offsetY", office.GetOffset().dy}
            });
        }
        map_json["offices"] = std::move(offices_array);

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = json::serialize(map_json);
        res.prepare_payload();
        return send(std::move(res));
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleStaticFile(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, const std::string& target_str) {
        // URL декодирование
        std::string decoded_path = UrlDecode(target_str);

        // Если путь пустой или заканчивается на /, добавляем index.html
        if (decoded_path.empty() || decoded_path.back() == '/') {
            decoded_path += "index.html";
        }

        // Убираем ведущий слеш
        if (decoded_path.front() == '/') {
            decoded_path = decoded_path.substr(1);
        }

        // Формируем полный путь к файлу
        fs::path file_path = fs::path(base_path_) / decoded_path;

        // Проверяем, что файл находится внутри базового каталога
        if (!IsSubPath(file_path)) {
            auto const bad_request = [&req](beast::string_view why) {
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::content_type, "text/plain");
                res.keep_alive(req.keep_alive());
                res.body() = std::string(why);
                res.prepare_payload();
                return res;
            };
            return send(bad_request("Invalid path"));
        }

        // Проверяем существование файла
        if (!fs::exists(file_path) || !fs::is_regular_file(file_path)) {
            auto const not_found = [&req](beast::string_view what) {
                http::response<http::string_body> res{http::status::not_found, req.version()};
                res.set(http::field::content_type, "text/plain");
                res.keep_alive(req.keep_alive());
                res.body() = std::string(what);
                res.prepare_payload();
                return res;
            };
            return send(not_found("File not found"));
        }

        // Открываем файл
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            auto const not_found = [&req](beast::string_view what) {
                http::response<http::string_body> res{http::status::not_found, req.version()};
                res.set(http::field::content_type, "text/plain");
                res.keep_alive(req.keep_alive());
                res.body() = std::string(what);
                res.prepare_payload();
                return res;
            };
            return send(not_found("File not found"));
        }

        // Читаем содержимое файла
        std::string content;
        file.seekg(0, std::ios::end);
        content.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(&content[0], content.size());

        // Определяем MIME-тип
        std::string content_type = GetMimeType(file_path.extension().string());

        // Формируем ответ
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, content_type);
        res.set(http::field::content_length, std::to_string(content.size()));
        res.keep_alive(req.keep_alive());

        // Для HEAD запроса не добавляем тело
        if (req.method() == http::verb::get) {
            res.body() = std::move(content);
        }

        res.prepare_payload();
        return send(std::move(res));
    }

    static std::string UrlDecode(std::string_view str);

    static std::string GetMimeType(const std::string& extension);

private:
    model::Game& game_;
    std::string base_path_;
};

}  // namespace http_handler

