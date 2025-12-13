#pragma once

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

#include "logging.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
using namespace std::literals;
namespace fs = std::filesystem;

class RequestHandler {
public:
    explicit RequestHandler(std::string config_path, std::string base_path)
        : config_path_{std::move(config_path)}
        , base_path_{std::move(base_path)} {
        LoadConfig();
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto const bad_request = [&req](beast::string_view why) {
            http::response<http::string_body> res{http::status::bad_request, req.version()};
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
    void LoadConfig();
    bool IsSubPath(fs::path path);
    static std::string UrlDecode(std::string_view str);
    static std::string GetMimeType(const std::string& extension);

    template <typename Body, typename Allocator, typename Send>
    void HandleApiMaps(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
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

        if (!map_id.empty() && map_id.back() == '/') {
            map_id.pop_back();
        }

        json::value* found_map = nullptr;
        if (config_data_.as_object().contains("maps") && config_data_.as_object().at("maps").is_array()) {
            for (auto& map_val : config_data_.as_object().at("maps").as_array()) {
                auto& map_obj = map_val.as_object();
                if (map_obj.contains("id") && map_obj.at("id").is_string()) {
                    std::string id_str = std::string(map_obj.at("id").as_string());
                    if (id_str == map_id) {
                        found_map = &map_val;
                        break;
                    }
                }
            }
        }

        if (!found_map) {
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

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = json::serialize(*found_map);
        res.prepare_payload();
        return send(std::move(res));
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleStaticFile(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, const std::string& target_str) {
        std::string decoded_path = UrlDecode(target_str);

        if (decoded_path.empty() || decoded_path.back() == '/') {
            decoded_path += "index.html";
        }

        if (decoded_path.front() == '/') {
            decoded_path = decoded_path.substr(1);
        }

        fs::path file_path = fs::path(base_path_) / decoded_path;

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

        std::string content;
        file.seekg(0, std::ios::end);
        content.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(&content[0], content.size());

        std::string content_type = GetMimeType(file_path.extension().string());

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, content_type);
        res.set(http::field::content_length, std::to_string(content.size()));
        res.keep_alive(req.keep_alive());

        if (req.method() == http::verb::get) {
            res.body() = std::move(content);
        }

        res.prepare_payload();
        return send(std::move(res));
    }

private:
    std::string config_path_;
    std::string base_path_;
    json::value config_data_;
};

} // namespace http_handler
