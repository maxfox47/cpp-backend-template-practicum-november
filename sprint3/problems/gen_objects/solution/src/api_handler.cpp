#include "api_handler.h"
#include "boost/json/serialize.hpp"
#include "model.h"

#include <iostream>

namespace http_handler {

/*
 * Преобразует дороги карты в JSON-массив для API-ответа.
 */
json::array ApiHandler::AddRoads(const model::Map* map) const {
	json::array roads_array;
	for (const auto& road : map->GetRoads()) {
		json::object road_object;
		road_object["x0"] = road.GetStart().x;
		road_object["y0"] = road.GetStart().y;
		
		// Горизонтальная дорога имеет x1, вертикальная - y1
		if (road.IsHorizontal()) {
			road_object["x1"] = road.GetEnd().x;
		} else {
			road_object["y1"] = road.GetEnd().y;
		}
		roads_array.push_back(road_object);
	}

	return roads_array;
}

/*
 * Преобразует здания карты в JSON-массив для API-ответа.
 */
json::array ApiHandler::AddBuildings(const model::Map* map) const {
	json::array buildings_array;
	for (const auto& building : map->GetBuildings()) {
		const auto& bounds = building.GetBounds();
		json::object building_object;
		building_object["x"] = bounds.position.x;
		building_object["y"] = bounds.position.y;
		building_object["w"] = bounds.size.width;
		building_object["h"] = bounds.size.height;
		buildings_array.push_back(std::move(building_object));
	}

	return buildings_array;
}

/*
 * Преобразует офисы карты в JSON-массив для API-ответа.
 */
json::array ApiHandler::AddOffices(const model::Map* map) const {
	json::array offices_array;
	for (const auto& office : map->GetOffices()) {
		json::object office_object;
		office_object["id"] = *office.GetId();
		office_object["x"] = office.GetPosition().x;
		office_object["y"] = office.GetPosition().y;
		office_object["offsetX"] = office.GetOffset().dx;
		office_object["offsetY"] = office.GetOffset().dy;
		offices_array.push_back(std::move(office_object));
	}

	return offices_array;
}

/*
 * Преобразует типы трофеев карты в JSON-массив для API-ответа.
 * Возвращает полную информацию о типах трофеев для фронтенда.
 */
json::array ApiHandler::AddLootTypes(const model::Map* map) const {
	json::array loot_types_array;
	for (const auto& loot_type : map->GetLootTypes()) {
		json::object loot_type_object;
		loot_type_object["name"] = loot_type.name;
		loot_type_object["file"] = loot_type.file;
		loot_type_object["type"] = loot_type.type;

		// Опциональные поля
		if (loot_type.rotation) {
			loot_type_object["rotation"] = *loot_type.rotation;
		}

		if (loot_type.color) {
			loot_type_object["color"] = *loot_type.color;
		}

		loot_type_object["scale"] = loot_type.scale;

		loot_types_array.push_back(std::move(loot_type_object));
	}

	return loot_types_array;
}

/*
 * Создает HTTP-ответ с ошибкой.
 */
ApiHandler::StringResponse ApiHandler::ErrorRequest(std::string error_code, std::string error_message,
																	 http::status status_code, unsigned int http_version,
																	 std::string allowed_method) const {
	StringResponse response{status_code, http_version};
	
	// Для метода не разрешенного добавляем заголовок Allow
	if (status_code == http::status::method_not_allowed) {
		if (allowed_method == "GET") {
			response.set(http::field::allow, "GET");
			response.insert(http::field::allow, "HEAD");
		} else {
			response.set(http::field::allow, allowed_method);
		}
	}

	boost::json::object response_body;
	response_body["code"] = error_code;
	response_body["message"] = error_message;
	std::string json_string = boost::json::serialize(response_body);
	
	response.set(http::field::content_type, "application/json");
	response.set(http::field::content_length, std::to_string(json_string.size()));
	response.set(http::field::cache_control, "no-cache");
	response.body() = json_string;

	return response;
}

/*
 * Обрабатывает успешный запрос на присоединение к игре.
 * Создает игровую сессию, добавляет собаку и игрока, возвращает токен.
 */
ApiHandler::StringResponse ApiHandler::GoodJoinRequest(const model::Map* map, std::string username,
																		 unsigned int http_version) {
	// Создаем игровую сессию с параметрами генератора трофеев
	auto game_session = game_.AddGameSession(
		model::GameSession{map, game_.GetPeriod(), game_.GetProbability()}
	);
	
	// Добавляем собаку игрока
	auto dog = game_session->AddDog(username);
	if (randomize_) {
		// Случайная позиция на дороге
		dog->SetPosition(map->GetRandomRoadPosition());
	} else {
		// Позиция на первой дороге
		dog->SetPosition({
			static_cast<double>(map->GetRoads().front().GetStart().x),
			static_cast<double>(map->GetRoads().front().GetStart().y)
		});
	}
	
	// Создаем игрока и генерируем токен
	auto player = players_.Add(game_session, dog);
	app::Token auth_token = players_tokens_.AddPlayer(player);

	boost::json::object response_body;
	response_body["authToken"] = *auth_token;
	response_body["playerId"] = player->GetId();
	std::string json_string = boost::json::serialize(response_body);

	StringResponse response{http::status::ok, http_version};
	response.set(http::field::content_type, "application/json");
	response.set(http::field::content_length, std::to_string(json_string.size()));
	response.set(http::field::cache_control, "no-cache");
	response.body() = json_string;

	return response;
}

/*
 * Парсит JSON-запрос на присоединение к игре.
 * Проверяет наличие обязательных полей userName и mapId.
 */
std::optional<json::object> ApiHandler::ParseJoinRequest(const StringRequest& request) {
	auto content_type_iterator = request.find(http::field::content_type);
	if (content_type_iterator == request.end() || content_type_iterator->value() != "application/json") {
		return std::nullopt;
	}
	
	try {
		boost::json::value json_data = boost::json::parse(request.body());
		if (!json_data.is_object()) {
			return std::nullopt;
		}
		
		boost::json::object request_object = json_data.as_object();
		if (!request_object.contains("userName") || !request_object.contains("mapId")) {
			return std::nullopt;
		}
		
		if (!request_object.at("userName").is_string() || !request_object.at("mapId").is_string()) {
			return std::nullopt;
		}
		
		return request_object;

	} catch (const std::exception&) {
		return std::nullopt;
	}
}

/*
 * Извлекает Bearer токен из заголовка Authorization.
 * Ожидает формат: "Bearer <32-символьный hex-токен>"
 */
std::string ApiHandler::ExtractBearerToken(const beast::string_view auth_header) {
	const std::string BEARER_PREFIX = "Bearer ";
	if (auth_header.size() > BEARER_PREFIX.size() &&
		 auth_header.compare(0, BEARER_PREFIX.size(), BEARER_PREFIX) == 0) {
		std::string auth_header_string = std::string(auth_header);
		std::string token_string = auth_header_string.substr(BEARER_PREFIX.size());
		
		// Токен должен быть 32 символа (16 байт в hex)
		if (token_string.size() != 32) {
			return "";
		}
		return token_string;
	}
	return "";
}

/*
 * Извлекает токен авторизации из HTTP-запроса.
 */
std::optional<std::string> ApiHandler::GetAuthToken(const StringRequest& request) {
	auto auth_header_iterator = request.find(http::field::authorization);
	if (auth_header_iterator == request.end()) {
		return std::nullopt;
	}
	
	std::string extracted_token = ExtractBearerToken(auth_header_iterator->value());
	if (extracted_token.empty()) {
		return std::nullopt;
	}
	return extracted_token;
}

/*
 * Формирует успешный ответ на запрос списка игроков.
 */
ApiHandler::StringResponse ApiHandler::GoodPlayersRequest(const StringRequest& req) {
	boost::json::object response_body;
	std::vector<std::string> player_names = players_.GetNames();

	// Преобразуем список имен игроков в JSON-объект с числовыми ключами
	for (size_t player_index = 0; player_index < player_names.size(); player_index++) {
		boost::json::object player_info;
		player_info["name"] = player_names[player_index];
		response_body[std::to_string(player_index)] = player_info;
	}

	std::string json_string = boost::json::serialize(response_body);
	StringResponse response{http::status::ok, req.version()};
	response.set(http::field::content_type, "application/json");
	response.insert(http::field::content_length, std::to_string(json_string.size()));
	response.set(http::field::cache_control, "no-cache");
	response.keep_alive(req.keep_alive());
	response.body() = json_string;

	response.prepare_payload();
	return response;
}

/*
 * Формирует успешный ответ на запрос состояния игры.
 * Включает информацию об игроках и потерянных предметах.
 */
ApiHandler::StringResponse ApiHandler::GoodStateRequest(const StringRequest& req) {
	json::object response_object;
	response_object["players"] = players_.GetPlayersInfo();
	
	// Собираем все потерянные предметы из всех игровых сессий
	json::object lost_objects_object;
	unsigned lost_object_id = 0;
	
	for (const model::GameSession& session : game_.GetGameSessions()) {
		for (const model::LostObject& lost_object : session.GetLostObjects()) {
			json::object lost_object_json;
			lost_object_json["type"] = lost_object.type;
			lost_object_json["pos"] = {lost_object.pos.x, lost_object.pos.y};
			
			std::string object_id_string = std::to_string(lost_object_id++);
			lost_objects_object[object_id_string] = std::move(lost_object_json);
		}
	}

	response_object["lostObjects"] = std::move(lost_objects_object);

	std::string json_string = json::serialize(response_object);

	StringResponse response{http::status::ok, req.version()};
	response.set(http::field::content_type, "application/json");
	response.insert(http::field::content_length, std::to_string(json_string.size()));
	response.set(http::field::cache_control, "no-cache");
	response.keep_alive(req.keep_alive());
	response.body() = json_string;

	response.prepare_payload();
	return response;
}

/*
 * Парсит JSON-запрос на движение игрока.
 * Ожидает поле "move" со значением "U", "D", "L", "R" или пустой строкой.
 */
std::optional<json::object> ApiHandler::ParseMoveRequest(const StringRequest& request) {
	auto content_type_iterator = request.find(http::field::content_type);
	if (content_type_iterator == request.end() || content_type_iterator->value() != "application/json") {
		return std::nullopt;
	}
	
	try {
		boost::json::value json_data = json::parse(request.body());
		if (!json_data.is_object()) {
			return std::nullopt;
		}

		json::object request_object = json_data.as_object();
		if (!request_object.contains("move")) {
			return std::nullopt;
		}

		if (!request_object.at("move").is_string()) {
			return std::nullopt;
		}

		return request_object;
	} catch (const std::exception&) {
		return std::nullopt;
	}
}

/*
 * Формирует успешный ответ на запрос движения игрока.
 */
ApiHandler::StringResponse ApiHandler::GoogMoveRequest(const StringRequest& req) {
	json::object empty_object;
	std::string json_string = json::serialize(empty_object);

	StringResponse response{http::status::ok, req.version()};
	response.set(http::field::content_type, "application/json");
	response.insert(http::field::content_length, std::to_string(json_string.size()));
	response.set(http::field::cache_control, "no-cache");
	response.keep_alive(req.keep_alive());
	response.body() = json_string;

	response.prepare_payload();
	return response;
}

/*
 * Парсит JSON-запрос на обновление игрового времени.
 * Ожидает поле "timeDelta" с целым числом (миллисекунды).
 */
std::optional<json::object> ApiHandler::ParseTickRequest(const StringRequest& request) {
	auto content_type_iterator = request.find(http::field::content_type);
	if (content_type_iterator == request.end() || content_type_iterator->value() != "application/json") {
		return std::nullopt;
	}
	
	try {
		boost::json::value json_data = json::parse(request.body());
		if (!json_data.is_object()) {
			return std::nullopt;
		}

		json::object request_object = json_data.as_object();
		if (!request_object.contains("timeDelta")) {
			return std::nullopt;
		}

		// timeDelta должен быть целым числом
		if (!request_object.at("timeDelta").is_int64()) {
			return std::nullopt;
		}

		return request_object;
	} catch (const std::exception&) {
		return std::nullopt;
	}
}

/*
 * Формирует успешный ответ на запрос обновления игрового времени.
 */
ApiHandler::StringResponse ApiHandler::GoodTickRequest(const StringRequest& req) {
	json::object empty_object;
	std::string json_string = json::serialize(empty_object);

	StringResponse response{http::status::ok, req.version()};
	response.set(http::field::content_type, "application/json");
	response.insert(http::field::content_length, std::to_string(json_string.size()));
	response.set(http::field::cache_control, "no-cache");
	response.keep_alive(req.keep_alive());
	response.body() = json_string;

	response.prepare_payload();
	return response;
}

} // namespace http_handler
