#pragma once

#include "boost/beast/http/status.hpp"
#include "endpoint.h"
#include "http_server.h"
#include "model.h"
#include "player.h"

#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/json.hpp>
#include <optional>
#include <string>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

template <typename Body, typename Allocator>
inline http::response<http::string_body>
BadRequest(const http::request<Body, http::basic_fields<Allocator>>& req, beast::string_view err,
			  const std::string& content_type, std::string cache_control = "") {
	http::response<http::string_body> res{http::status::bad_request, req.version()};
	res.set(http::field::content_type, content_type);
	res.keep_alive(req.keep_alive());
	res.body() = std::string(err);
	if (!cache_control.empty()) {
		res.set(http::field::cache_control, cache_control);
	}
	res.prepare_payload();
	return res;
}

template <typename Body, typename Allocator>
inline http::response<http::string_body>
NotFound(const http::request<Body, http::basic_fields<Allocator>>& req, beast::string_view err,
			const std::string& content_type, std::string cache_control = "") {
	http::response<http::string_body> res{http::status::not_found, req.version()};
	res.set(http::field::content_type, content_type);
	res.keep_alive(req.keep_alive());
	res.body() = std::string(err);
	if (!cache_control.empty()) {
		res.set(http::field::cache_control, cache_control);
	}
	res.prepare_payload();
	return res;
};

template <typename Body, typename Allocator>
inline http::response<http::string_body>
ServerError(const http::request<Body, http::basic_fields<Allocator>>& req, beast::string_view err,
				const std::string& content_type, std::string cache_control = "") {
	http::response<http::string_body> res{http::status::internal_server_error, req.version()};
	res.set(http::field::content_type, content_type);
	res.keep_alive(req.keep_alive());
	res.body() = std::string(err);
	if (!cache_control.empty()) {
		res.set(http::field::cache_control, cache_control);
	}
	res.prepare_payload();
	return res;
}

template <typename Body, typename Allocator>
inline http::response<http::string_body>
MethodNotAllowed(const http::request<Body, http::basic_fields<Allocator>>& req,
					  beast::string_view err, const std::string& content_type,
					  std::string cache_control = "") {
	http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
	res.set(http::field::content_type, content_type);
	res.keep_alive(req.keep_alive());
	res.body() = std::string(err);
	if (!cache_control.empty()) {
		res.set(http::field::cache_control, cache_control);
	}
	res.prepare_payload();
	return res;
}

class ApiHandler {
 public:
	using StringRequest = http::request<http::string_body>;
	using StringResponse = http::response<http::string_body>;

	explicit ApiHandler(model::Game& game, bool randomize, bool auto_tick)
		 : game_(game), randomize_(randomize), auto_tick_(auto_tick) {}

	template <typename Body, typename Allocator, typename Send>
	void operator()(const EndPoint& endpoint,
						 const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
		if (endpoint.IsMapsReq()) {
			return MapsRequest(req, std::move(send));
		}

		if (endpoint.IsSpecificMapReq()) {
			return SpecificMapRequest(endpoint.GetEndPoint(), req, std::move(send));
		}

		if (endpoint.IsJoinReq()) {
			return JoinRequest(req, std::move(send));
		}

		if (endpoint.IsPlayersReq()) {
			return PlayersRequest(req, std::move(send));
		}

		if (endpoint.IsStateReq()) {
			return StateRequest(req, std::move(send));
		}

		if (endpoint.IsActionReq()) {
			return MoveRequest(req, std::move(send));
		}

		if (endpoint.IsTickReq()) {
			return TickRequest(req, std::move(send));
		}

		return send(BadRequest(
			 std::move(req),
			 json::serialize(json::object{{"code", "badRequest"}, {"message", "Bad request"}}),
			 "application/json"));
	}

 private:
	template <typename Body, typename Allocator, typename Send>
	void CheckMethod(const http::request<Body, http::basic_fields<Allocator>>& req, Send& send,
						  std::string method) const {
		if (method == "GET") {
			if (req.method() != http::verb::get && req.method() != http::verb::head) {
				return send(ErrorRequest("invalidMethod", "Only GET and HEAD method are expected",
												 http::status::method_not_allowed, req.version(), "GET"));
			}
		} else if (method == "POST") {
			if (req.method() != http::verb::post) {
				return send(ErrorRequest("invalidMethod", "Only POST method are expected",
												 http::status::method_not_allowed, req.version(), "POST"));
			}
		}
	}

	template <typename Body, typename Allocator, typename Send>
	app::Player* CheckTokenAndPlayer(const http::request<Body, http::basic_fields<Allocator>>& req,
												Send& send) {
		auto ver = req.version();
		std::optional<std::string> token_str = GetAuthToken(req);
		if (!token_str) {
			send(ErrorRequest("invalidToken", "Authorization header is missing",
									http::status::unauthorized, ver));
			return nullptr;
		}

		app::Token token{*token_str};
		auto player = players_tokens_.FindPlayerByToken(token);
		if (!player) {
			send(ErrorRequest("unknownToken", "Player token has not been found",
									http::status::unauthorized, ver));
			return nullptr;
		}

		return player;
	}

	template <typename Body, typename Allocator, typename Send>
	void MapsRequest(const http::request<Body, http::basic_fields<Allocator>>& req,
						  Send&& send) const {
		json::array arr;
		for (const auto& map : game_.GetMaps()) {
			json::object obj;
			obj["id"] = *map.GetId();
			obj["name"] = map.GetName();
			arr.push_back(std::move(obj));
		}

		http::response<http::string_body> resp{http::status::ok, req.version()};
		resp.set(http::field::content_type, "application/json");
		resp.body() = json::serialize(arr);
		resp.keep_alive(req.keep_alive());
		resp.prepare_payload();

		return send(std::move(resp));
	}

	template <typename Body, typename Allocator, typename Send>
	void SpecificMapRequest(const std::string& target,
									const http::request<Body, http::basic_fields<Allocator>>& req,
									Send&& send) const {
		CheckMethod(req, send, "GET");
		std::string id = target.substr(13);
		if (!id.empty() && id.back() == '/') {
			id.pop_back();
		}

		const auto* map = game_.FindMap(model::Map::Id(id));
		if (!map) {
			return send(
				 ErrorRequest("mapNotFound", "Map not found", http::status::not_found, req.version()));
		}

		json::object obj;
		obj["id"] = *map->GetId();
		obj["name"] = map->GetName();
		obj["roads"] = AddRoads(map);
		obj["buildings"] = AddBuildings(map);
		obj["offices"] = AddOffices(map);
		obj["lootTypes"] = AddLootTypes(map);

		http::response<http::string_body> resp{http::status::ok, req.version()};
		resp.set(http::field::content_type, "application/json");
		resp.body() = json::serialize(obj);
		resp.keep_alive(req.keep_alive());
		resp.prepare_payload();

		return send(std::move(resp));
	}

	template <typename Body, typename Allocator, typename Send>
	void JoinRequest(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
		auto ver = req.version();
		CheckMethod(req, send, "POST");

		std::optional<boost::json::object> obj = ParseJoinRequest(req);

		if (!obj) {
			return send(ErrorRequest("invalidArgument", "Join game request parse error",
											 http::status::bad_request, ver));
		}
		std::string name = std::string(obj.value().at("userName").as_string());
		std::string map_id = std::string(obj.value().at("mapId").as_string());

		if (name.empty()) {
			return send(
				 ErrorRequest("invalidArgument", "Invalid name", http::status::bad_request, ver));
		}

		auto map = game_.FindMap(model::Map::Id{std::string(obj.value().at("mapId").as_string())});

		if (!map) {
			return send(ErrorRequest("mapNotFound", "Map not found", http::status::not_found, ver));
		}

		return send(GoodJoinRequest(map, name, ver));
	}

	template <typename Body, typename Allocator, typename Send>
	void PlayersRequest(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
		auto ver = req.version();
		CheckMethod(req, send, "GET");

		if (!CheckTokenAndPlayer(req, send)) {
			return;
		}

		return send(GoodPlayersRequest(req));
	}

	template <typename Body, typename Allocator, typename Send>
	void StateRequest(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
		auto ver = req.version();
		CheckMethod(req, send, "GET");

		if (!CheckTokenAndPlayer(req, send)) {
			return;
		}

		return send(GoodStateRequest(req));
	}

	template <typename Body, typename Allocator, typename Send>
	void MoveRequest(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
		auto ver = req.version();
		CheckMethod(req, send, "POST");
		app::Player* player = CheckTokenAndPlayer(req, send);
		if (!player) {
			return;
		}

		std::optional<json::object> obj = ParseMoveRequest(req);
		if (!obj) {
			return send(ErrorRequest("invalidArgument", "Failed to parse action",
											 http::status::bad_request, ver));
		}

		std::string dir((*obj).at("move").as_string());

		if (dir == "U") {
			double def_speed = player->GetDefaultSpeed();
			player->SetDirection(model::Direction::NORTH);
			player->SetSpeed({0, -def_speed});
		} else if (dir == "D") {
			double def_speed = player->GetDefaultSpeed();
			player->SetDirection(model::Direction::SOUTH);
			player->SetSpeed({0, def_speed});
		} else if (dir == "L") {
			double def_speed = player->GetDefaultSpeed();
			player->SetDirection(model::Direction::WEST);
			player->SetSpeed({-def_speed, 0});
		} else if (dir == "R") {
			double def_speed = player->GetDefaultSpeed();
			player->SetDirection(model::Direction::EAST);
			player->SetSpeed({def_speed, 0});
		} else if (dir.empty()) {
			player->SetSpeed({0, 0});
		} else {
			return send(ErrorRequest("invalidArgument", "Failed to parse action",
											 http::status::bad_request, ver));
		}

		return send(GoogMoveRequest(req));
	}

	template <typename Body, typename Allocator, typename Send>
	void TickRequest(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
		auto ver = req.version();
		if (auto_tick_) {
			return send(
				 ErrorRequest("badRequest", "Invalid endpoint", http::status::bad_request, ver));
		}
		CheckMethod(req, send, "POST");

		std::optional<json::object> obj = ParseTickRequest(req);
		if (!obj) {
			return send(ErrorRequest("invalidArgument", "Failed to parse JSON",
											 http::status::bad_request, ver));
		}

		int time_delta = static_cast<int>((*obj).at("timeDelta").as_int64());

		game_.Tick(time_delta);

		return send(GoodTickRequest(req));
	}

	json::array AddRoads(const model::Map* map) const;
	json::array AddBuildings(const model::Map* map) const;
	json::array AddOffices(const model::Map* map) const;
	json::array AddLootTypes(const model::Map* map) const;
	StringResponse ErrorRequest(std::string code, std::string message, http::status status,
										 unsigned int ver, std::string allowed_method = "") const;
	StringResponse GoodJoinRequest(const model::Map* map, std::string username, unsigned int ver);
	std::optional<json::object> ParseJoinRequest(const StringRequest& request);
	std::string ExtractBearerToken(const beast::string_view auth_header);
	std::optional<std::string> GetAuthToken(const StringRequest& request);
	StringResponse GoodPlayersRequest(const StringRequest& req);
	StringResponse GoodStateRequest(const StringRequest& req);
	std::optional<json::object> ParseMoveRequest(const StringRequest& request);
	StringResponse GoogMoveRequest(const StringRequest& req);
	std::optional<json::object> ParseTickRequest(const StringRequest& request);
	StringResponse GoodTickRequest(const StringRequest& req);

	model::Game& game_;
	app::Players players_;
	app::PlayerTokens players_tokens_;
	bool randomize_;
	bool auto_tick_;
};

} // namespace http_handler
