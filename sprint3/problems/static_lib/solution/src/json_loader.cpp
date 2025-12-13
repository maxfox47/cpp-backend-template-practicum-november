#include "json_loader.h"

#include <fstream>
#include <sstream>
#include <string>

namespace json = boost::json;

namespace json_loader {

/*
 * Загружает содержимое JSON-файла в строку.
 */
std::string LoadJsonFile(const std::filesystem::path& json_path) {
	std::ifstream file(json_path, std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Cannot open file");
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

/*
 * Загружает карту из JSON-объекта.
 * Извлекает id и name карты.
 */
model::Map LoadMap(const json::value& map_json_value) {
	const auto& map_json_object = map_json_value.as_object();
	std::string map_id_string(map_json_object.at("id").as_string());
	std::string map_name(map_json_object.at("name").as_string());
	
	util::Tagged<std::string, model::Map> map_id_tagged(map_id_string);
	model::Map map(map_id_tagged, map_name);
	return map;
}

/*
 * Загружает дорогу из JSON-объекта.
 * Определяет направление дороги по наличию поля x1 (горизонтальная) или y1 (вертикальная).
 */
model::Road LoadRoad(const json::value& road_json_value) {
	const auto& road_json_object = road_json_value.as_object();
	model::Point road_start_point(
		road_json_object.at("x0").as_int64(), 
		road_json_object.at("y0").as_int64()
	);
	
	// Проверяем наличие поля x1 для горизонтальной дороги
	auto x1_iterator = road_json_object.find("x1");
	if (x1_iterator != road_json_object.end()) {
		model::Road horizontal_road(
			model::Road::HORIZONTAL, 
			road_start_point, 
			x1_iterator->value().as_int64()
		);
		return horizontal_road;
	} else {
		// Иначе это вертикальная дорога с полем y1
		auto y1_iterator = road_json_object.find("y1");
		model::Road vertical_road(
			model::Road::VERTICAL, 
			road_start_point, 
			y1_iterator->value().as_int64()
		);
		return vertical_road;
	}
}

/*
 * Загружает здание из JSON-объекта.
 */
model::Building LoadBuilding(const json::value& building_json_value) {
	const auto& building_json_object = building_json_value.as_object();
	model::Rectangle building_bounds(
		{
			static_cast<int>(building_json_object.at("x").as_int64()),
			static_cast<int>(building_json_object.at("y").as_int64())
		},
		{
			static_cast<int>(building_json_object.at("w").as_int64()),
			static_cast<int>(building_json_object.at("h").as_int64())
		}
	);
	model::Building building(building_bounds);
	return building;
}

/*
 * Загружает офис из JSON-объекта.
 */
model::Office LoadOffice(const json::value& office_json_value) {
	const auto& office_json_object = office_json_value.as_object();
	model::Point office_position(
		office_json_object.at("x").as_int64(), 
		office_json_object.at("y").as_int64()
	);
	model::Offset office_offset(
		office_json_object.at("offsetX").as_int64(), 
		office_json_object.at("offsetY").as_int64()
	);
	std::string office_id_string(office_json_object.at("id").as_string());
	
	util::Tagged<std::string, model::Office> office_id_tagged(office_id_string);
	model::Office office(office_id_tagged, office_position, office_offset);
	return office;
}

/*
 * Загружает игровую конфигурацию из JSON-файла.
 * Создает объект Game со всеми картами, их элементами и настройками генератора трофеев.
 */
model::Game LoadGame(const std::filesystem::path& json_path) {
	std::string json_file_content = LoadJsonFile(json_path);
	auto parsed_json = json::parse(json_file_content);
	
	// Загружаем скорость по умолчанию для собак
	double default_dog_speed = 1.0;
	if (parsed_json.as_object().contains("defaultDogSpeed")) {
		default_dog_speed = parsed_json.as_object().at("defaultDogSpeed").as_double();
	}

	// Загружаем настройки генератора трофеев
	const auto& loot_generator_config = parsed_json.at("lootGeneratorConfig").as_object();
	double loot_generation_period = loot_generator_config.at("period").as_double();
	double loot_generation_probability = loot_generator_config.at("probability").as_double();

	const auto& maps_array = parsed_json.at("maps").as_array();
	model::Game game;
	
	// Устанавливаем параметры генератора трофеев для игры
	game.SetPeriod(loot_generation_period);
	game.SetProbability(loot_generation_probability);

	// Загружаем каждую карту
	for (const auto& map_json_value : maps_array) {
		model::Map map = LoadMap(map_json_value);

		// Загружаем дороги карты
		const auto& roads_array = map_json_value.at("roads").as_array();
		for (const auto& road_json_value : roads_array) {
			model::Road road = LoadRoad(road_json_value);
			map.AddRoad(road);
		}

		// Загружаем здания карты
		const auto& buildings_array = map_json_value.at("buildings").as_array();
		for (const auto& building_json_value : buildings_array) {
			model::Building building = LoadBuilding(building_json_value);
			map.AddBuilding(building);
		}

		// Загружаем офисы карты
		const auto& offices_array = map_json_value.at("offices").as_array();
		for (const auto& office_json_value : offices_array) {
			model::Office office = LoadOffice(office_json_value);
			map.AddOffice(office);
		}

		// Устанавливаем скорость собак для карты
		if (map_json_value.as_object().contains("dogSpeed")) {
			map.SetDefaultSpeed(map_json_value.as_object().at("dogSpeed").as_double());
		} else {
			map.SetDefaultSpeed(default_dog_speed);
		}

		// Загружаем типы трофеев для карты
		const auto& loot_types_array = map_json_value.at("lootTypes").as_array();
		for (const auto& loot_type_json_value : loot_types_array) {
			json::object loot_type_object = loot_type_json_value.as_object();
			model::Loot loot_type;
			
			loot_type.name = loot_type_object.at("name").as_string();
			loot_type.file = loot_type_object.at("file").as_string();
			loot_type.type = loot_type_object.at("type").as_string();

			// Опциональные поля
			if (loot_type_object.contains("rotation")) {
				loot_type.rotation = loot_type_object.at("rotation").as_int64();
			}

			if (loot_type_object.contains("color")) {
				loot_type.color = loot_type_object.at("color").as_string();
			}

			loot_type.scale = loot_type_object.at("scale").as_double();

			map.AddLootType(loot_type);
		}

		game.AddMap(map);
	}

	return game;
}

} // namespace json_loader

