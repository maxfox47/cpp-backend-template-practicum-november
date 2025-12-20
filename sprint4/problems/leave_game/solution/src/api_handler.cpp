#include "api_handler.h"
#include "boost/json/serialize.hpp"
#include "model.h"

#include <iostream>

namespace http_handler {

json::array ApiHandler::AddRoads(const model::Map* map) const {
	json::array roads_arr;
	for (const auto& road : map->GetRoads()) {
		json::object road_obj;
		road_obj["x0"] = road.GetStart().x;
		road_obj["y0"] = road.GetStart().y;
		if (road.IsHorizontal())
			road_obj["x1"] = road.GetEnd().x;
		else
			road_obj["y1"] = road.GetEnd().y;
		roads_arr.push_back(road_obj);
	}

	return roads_arr;
}

json::array ApiHandler::AddBuildings(const model::Map* map) const {
	json::array buildings_arr;
	for (const auto& b : map->GetBuildings()) {
		const auto& bounds = b.GetBounds();
		json::object bounds_obj;
		bounds_obj["x"] = bounds.position.x;
		bounds_obj["y"] = bounds.position.y;
		bounds_obj["w"] = bounds.size.width;
		bounds_obj["h"] = bounds.size.height;
		buildings_arr.push_back(std::move(bounds_obj));
	}

	return buildings_arr;
}

json::array ApiHandler::AddOffices(const model::Map* map) const {
	json::array offices_arr;
	for (const auto& office : map->GetOffices()) {
		json::object office_obj;
		office_obj["id"] = *office.GetId();
		office_obj["x"] = office.GetPosition().x;
		office_obj["y"] = office.GetPosition().y;
		office_obj["offsetX"] = office.GetOffset().dx;
		office_obj["offsetY"] = office.GetOffset().dy;
		offices_arr.push_back(std::move(office_obj));
	}

	return offices_arr;
}

json::array ApiHandler::AddLootTypes(const model::Map* map) const {
	json::array loot_types;
	for (const auto& loot_type : map->GetLootTypes()) {
		json::object obj;
		obj["name"] = loot_type.name;
		obj["file"] = loot_type.file;
		obj["type"] = loot_type.type;
		obj["value"] = loot_type.value;

		if (loot_type.rotation) {
			obj["rotation"] = *loot_type.rotation;
		}

		if (loot_type.color) {
			obj["color"] = *loot_type.color;
		}

		obj["scale"] = loot_type.scale;

		loot_types.push_back(std::move(obj));
	}

	return loot_types;
}

ApiHandler::StringResponse ApiHandler::ErrorRequest(std::string code, std::string message,
																	 http::status status, unsigned int ver,
																	 std::string allowed_method) const {
	StringResponse response{status, ver};
	if (status == http::status::method_not_allowed) {
		if (allowed_method == "GET") {
			response.set(http::field::allow, "GET");
			response.insert(http::field::allow, "HEAD");
		} else {
			response.set(http::field::allow, allowed_method);
		}
	}

	boost::json::object response_body;
	response_body["code"] = code;
	response_body["message"] = message;
	std::string json_str = boost::json::serialize(response_body);
	response.set(http::field::content_type, "application/json");
	response.set(http::field::content_length, std::to_string(json_str.size()));
	response.set(http::field::cache_control, "no-cache");
	response.body() = json_str;

	return response;
}

ApiHandler::StringResponse ApiHandler::GoodJoinRequest(const model::Map* map, std::string username,
																		 unsigned int ver) {
	auto game_session =
		 game_.AddGameSession(model::GameSession{map, game_.GetPeriod(), game_.GetProbability()});
	auto dog = game_session->AddDog(username);
	if (randomize_) {
		dog->SetPosition(map->GetRandomRoadPosition());
	} else {
		dog->SetPosition({static_cast<double>(map->GetRoads().front().GetStart().x),
								static_cast<double>(map->GetRoads().front().GetStart().y)});
	}
	auto player = players_.Add(game_session, dog);
	app::Token token = players_tokens_.AddPlayer(player);

	boost::json::object response_body;
	response_body["authToken"] = *token;
	response_body["playerId"] = player->GetId();
	std::string json_str = boost::json::serialize(response_body);

	StringResponse response{http::status::ok, ver};
	response.set(http::field::content_type, "application/json");
	response.set(http::field::content_length, std::to_string(json_str.size()));
	response.set(http::field::cache_control, "no-cache");
	response.body() = json_str;

	return response;
}

std::optional<json::object> ApiHandler::ParseJoinRequest(const StringRequest& request) {
	auto it = request.find(http::field::content_type);
	if (it == request.end() || it->value() != "application/json") {
		return std::nullopt;
	}
	try {
		boost::json::value json_data = boost::json::parse(request.body());
		if (!json_data.is_object()) {
			return std::nullopt;
		}
		boost::json::object obj = json_data.as_object();
		if (!obj.contains("userName") || !obj.contains("mapId")) {
			return std::nullopt;
		}
		if (!obj.at("userName").is_string() || !obj.at("mapId").is_string()) {
			return std::nullopt;
		}
		return obj;

	} catch (const std::exception&) {
		return std::nullopt;
	}
}

std::string ApiHandler::ExtractBearerToken(const beast::string_view auth_header) {
	const std::string bearer_prefix = "Bearer ";
	if (auth_header.size() > bearer_prefix.size() &&
		 auth_header.compare(0, bearer_prefix.size(), bearer_prefix) == 0) {
		std::string str = std::string(auth_header);
		std::string token_str = str.substr(bearer_prefix.size());
		if (token_str.size() != 32) {
			return "";
		}
		return token_str;
	}
	return "";
}

std::optional<std::string> ApiHandler::GetAuthToken(const StringRequest& request) {
	auto auth_header = request.find(http::field::authorization);
	if (auth_header == request.end()) {
		return std::nullopt;
	}
	std::string auth_token = ExtractBearerToken(auth_header->value());
	if (auth_token.empty()) {
		return std::nullopt;
	}
	return auth_token;
}

ApiHandler::StringResponse ApiHandler::GoodPlayersRequest(const StringRequest& req) {
	boost::json::object response_body;
	std::vector<std::string> names = players_.GetNames();

	for (size_t i = 0; i < names.size(); i++) {
		boost::json::object player_info;
		player_info["name"] = names[i];
		response_body[std::to_string(i)] = player_info;
	}

	std::string json_str = boost::json::serialize(response_body);
	StringResponse response{http::status::ok, req.version()};
	response.set(http::field::content_type, "application/json");
	response.insert(http::field::content_length, std::to_string(json_str.size()));
	response.set(http::field::cache_control, "no-cache");
	response.keep_alive(req.keep_alive());
	response.body() = json_str;

	response.prepare_payload();
	return response;
}

ApiHandler::StringResponse ApiHandler::GoodStateRequest(const StringRequest& req) {
	json::object obj;
	obj["players"] = players_.GetPlayersInfo();
	json::object lost_objects;

	int count = 0;
	for (const model::GameSession& session : game_.GetGameSessions()) {
		for (const model::LostObject& loot : session.GetLostObjects()) {
			json::object lost_obj;
			lost_obj["type"] = loot.type;
			lost_obj["pos"] = {loot.pos.x, loot.pos.y};
			std::string key = std::to_string(count++);
			lost_objects[key] = std::move(lost_obj);
		}
	}

	obj["lostObjects"] = std::move(lost_objects);

	std::string json_str = json::serialize(obj);

	StringResponse response{http::status::ok, req.version()};
	response.set(http::field::content_type, "application/json");
	response.insert(http::field::content_length, std::to_string(json_str.size()));
	response.set(http::field::cache_control, "no-cache");
	response.keep_alive(req.keep_alive());
	response.body() = json_str;

	response.prepare_payload();
	return response;
}

std::optional<json::object> ApiHandler::ParseMoveRequest(const StringRequest& request) {
	auto it = request.find(http::field::content_type);
	if (it == request.end() || it->value() != "application/json") {
		return std::nullopt;
	}
	try {
		boost::json::value json_data = json::parse(request.body());
		if (!json_data.is_object()) {
			return std::nullopt;
		}

		json::object obj = json_data.as_object();
		if (!obj.contains("move")) {
			return std::nullopt;
		}

		if (!obj.at("move").is_string()) {
			return std::nullopt;
		}

		return obj;
	} catch (const std::exception&) {
		return std::nullopt;
	}
}

ApiHandler::StringResponse ApiHandler::GoogMoveRequest(const StringRequest& req) {
	json::object obj;
	std::string json_str = json::serialize(obj);

	StringResponse response{http::status::ok, req.version()};
	response.set(http::field::content_type, "application/json");
	response.insert(http::field::content_length, std::to_string(json_str.size()));
	response.set(http::field::cache_control, "no-cache");
	response.keep_alive(req.keep_alive());
	response.body() = json_str;

	response.prepare_payload();
	return response;
}

std::optional<json::object> ApiHandler::ParseTickRequest(const StringRequest& request) {
	auto it = request.find(http::field::content_type);
	if (it == request.end() || it->value() != "application/json") {
		return std::nullopt;
	}
	try {
		boost::json::value json_data = json::parse(request.body());
		if (!json_data.is_object()) {
			return std::nullopt;
		}

		json::object obj = json_data.as_object();
		if (!obj.contains("timeDelta")) {
			return std::nullopt;
		}

		if (!obj.at("timeDelta").is_int64()) {
			return std::nullopt;
		}

		return obj;
	} catch (const std::exception&) {
		return std::nullopt;
	}
}

ApiHandler::StringResponse ApiHandler::GoodTickRequest(const StringRequest& req) {
	json::object obj;
	std::string json_str = json::serialize(obj);

	StringResponse response{http::status::ok, req.version()};
	response.set(http::field::content_type, "application/json");
	response.insert(http::field::content_length, std::to_string(json_str.size()));
	response.set(http::field::cache_control, "no-cache");
	response.keep_alive(req.keep_alive());
	response.body() = json_str;

	response.prepare_payload();
	return response;
}

ApiHandler::StringResponse ApiHandler::GoodRecordsRequest(const StringRequest& req, int start,
																			 int max_items) {
	try {
		auto records = db_.GetRecords(start, max_items);

		json::array arr;
		for (const auto& record : records) {
			json::object obj;
			obj["name"] = record.name;
			obj["score"] = record.score;
			obj["playTime"] = record.play_time;
			arr.push_back(obj);
		}

		std::string json_str = json::serialize(arr);

		StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, "application/json");
		response.set(http::field::content_length, std::to_string(json_str.size()));
		response.set(http::field::cache_control, "no-cache");
		response.keep_alive(req.keep_alive());
		response.body() = json_str;
		response.prepare_payload();

		return response;
	} catch (const std::exception& ex) {
		return ErrorRequest("databaseError", ex.what(), http::status::internal_server_error,
								  req.version());
	}
}

} // namespace http_handler
