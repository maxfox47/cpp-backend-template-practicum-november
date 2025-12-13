#pragma once
#include "http_server.h"
#include "model.h"
#include <boost/json.hpp>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        if (req.method() != http::verb::get) {
            const auto bad_request = [&req](beast::string_view why) {
                http::response<http::string_body> response{http::status::bad_request, req.version()};
                response.set(http::field::content_type, "application/json");
                response.keep_alive(req.keep_alive());
                response.body() = json::serialize(json::object{
                    {"code", "badRequest"},
                    {"message", why}
                });
                response.prepare_payload();
                return response;
            };
            return send(bad_request("Unsupported HTTP method"));
        }

        try {
            const std::string target_str = std::string(req.target());
            
            if (req.target() == "/api/v1/maps") {
                json::array maps_array;
                for (const auto& map : game_.GetMaps()) {
                    maps_array.push_back(json::object{
                        {"id", *map.GetId()},
                        {"name", map.GetName()}
                    });
                }

                http::response<http::string_body> response{http::status::ok, req.version()};
                response.set(http::field::content_type, "application/json");
                response.keep_alive(req.keep_alive());
                response.body() = json::serialize(maps_array);
                response.prepare_payload();
                return send(std::move(response));
            }
            
            if (target_str.starts_with("/api/v1/maps/")) {
                std::string map_id = target_str.substr(13);

                if (!map_id.empty() && map_id.back() == '/') {
                    map_id.pop_back();
                }

                const model::Map* const map = game_.FindMap(model::Map::Id(map_id));
                if (!map) {
                    const auto not_found = [&req](beast::string_view what) {
                        http::response<http::string_body> response{http::status::not_found, req.version()};
                        response.set(http::field::content_type, "application/json");
                        response.keep_alive(req.keep_alive());
                        response.body() = json::serialize(json::object{
                            {"code", "mapNotFound"},
                            {"message", what}
                        });
                        response.prepare_payload();
                        return response;
                    };
                    return send(not_found("Map not found"));
                }

                json::object map_json;
                map_json["name"] = map->GetName();
                map_json["id"] = *map->GetId();

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

                http::response<http::string_body> response{http::status::ok, req.version()};
                response.set(http::field::content_type, "application/json");
                response.keep_alive(req.keep_alive());
                response.body() = json::serialize(map_json);
                response.prepare_payload();
                return send(std::move(response));
            }

            if (target_str.starts_with("/api/")) {
                const auto bad_request = [&req](beast::string_view why) {
                    http::response<http::string_body> response{http::status::bad_request, req.version()};
                    response.set(http::field::content_type, "application/json");
                    response.keep_alive(req.keep_alive());
                    response.body() = json::serialize(json::object{
                        {"code", "badRequest"},
                        {"message", why}
                    });
                    response.prepare_payload();
                    return response;
                };
                return send(bad_request("Bad request"));
            }

            const auto not_found = [&req](beast::string_view what) {
                http::response<http::string_body> response{http::status::not_found, req.version()};
                response.set(http::field::content_type, "application/json");
                response.keep_alive(req.keep_alive());
                response.body() = json::serialize(json::object{
                    {"code", "mapNotFound"},
                    {"message", what}
                });
                response.prepare_payload();
                return response;
            };
            return send(not_found("Endpoint not found"));

        } catch (const std::exception& e) {
            const auto server_error = [&req](beast::string_view what) {
                http::response<http::string_body> response{http::status::internal_server_error, req.version()};
                response.set(http::field::content_type, "application/json");
                response.keep_alive(req.keep_alive());
                response.body() = json::serialize(json::object{
                    {"code", "internalError"},
                    {"message", what}
                });
                response.prepare_payload();
                return response;
            };
            return send(server_error(e.what()));
        }
    }

private:
    model::Game& game_;
};

}  // namespace http_handler