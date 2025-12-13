#include "json_loader.h"

#include <fstream>
#include <sstream>
#include <string>

namespace json = boost::json;

namespace json_loader {
std::string LoadJsonFile(const std::filesystem::path& json_path) {
	std::ifstream file(json_path, std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Cannot open file");
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

model::Map LoadMap(const json::value& map_json_not) {
	const auto& map_json = map_json_not.as_object();
	std::string id(map_json.at("id").as_string());
	std::string name(map_json.at("name").as_string());
	util::Tagged<std::string, model::Map> id_t(id);
	model::Map map(id_t, name);
	return map;
}

model::Road LoadRoad(const json::value& road_json_not) {
	const auto& road_json = road_json_not.as_object();
	model::Point point_start(road_json.at("x0").as_int64(), road_json.at("y0").as_int64());
	auto last_point = road_json.find("x1");
	if (last_point != road_json.end()) {
		model::Road road(model::Road::HORIZONTAL, point_start, last_point->value().as_int64());
		return road;
	} else {
		last_point = road_json.find("y1");
		model::Road road(model::Road::VERTICAL, point_start, last_point->value().as_int64());
		return road;
	}
}

model::Building LoadBuilding(const json::value& building_json_not) {
	const auto& building_json = building_json_not.as_object();
	model::Rectangle rect({static_cast<int>(building_json.at("x").as_int64()),
								  static_cast<int>(building_json.at("y").as_int64())},
								 {static_cast<int>(building_json.at("w").as_int64()),
								  static_cast<int>(building_json.at("h").as_int64())});
	model::Building building(rect);
	return building;
}

model::Office LoadOffice(const json::value& office_json_not) {
	const auto& office_json = office_json_not.as_object();
	model::Point position(office_json.at("x").as_int64(), office_json.at("y").as_int64());
	model::Offset offset(office_json.at("offsetX").as_int64(), office_json.at("offsetY").as_int64());
	std::string id(office_json.at("id").as_string());
	util::Tagged<std::string, model::Office> id_t(id);
	model::Office office(id_t, position, offset);
	return office;
}

model::Game LoadGame(const std::filesystem::path& json_path) {
	std::string json_str = LoadJsonFile(json_path);
	auto value = json::parse(json_str);
	double def_speed = 1;
	int def_capacity = 3;
	if (value.as_object().contains("defaultDogSpeed")) {
		def_speed = value.as_object().at("defaultDogSpeed").as_double();
	}

	if (value.as_object().contains("defaultBagCapacity")) {
		def_speed = value.as_object().at("defaultBagCapacity").as_int64();
	}

	const auto maps = value.at("maps").as_array();
	model::Game game;
	for (const auto& map_json : maps) {
		model::Map map = LoadMap(map_json);

		const auto roads = map_json.at("roads").as_array();
		for (const auto& road_json : roads) {
			model::Road road = LoadRoad(road_json);
			map.AddRoad(road);
		}

		const auto buildings = map_json.at("buildings").as_array();
		for (const auto& building_json : buildings) {
			model::Building building = LoadBuilding(building_json);
			map.AddBuilding(building);
		}

		const auto offices = map_json.at("offices").as_array();
		for (const auto& office_json : offices) {
			model::Office office = LoadOffice(office_json);
			map.AddOffice(office);
		}

		if (map_json.as_object().contains("dogSpeed")) {
			map.SetDefaultSpeed(map_json.as_object().at("dogSpeed").as_double());
		} else {
			map.SetDefaultSpeed(def_speed);
		}

		if (map_json.as_object().contains("bagCapacity")) {
			map.SetBagCapacity(map_json.as_object().at("bagCapacity").as_double());
		} else {
			map.SetBagCapacity(def_capacity);
		}

		const auto loot_types = map_json.at("lootTypes").as_array();
		for (const auto& loot_type : loot_types) {
			json::object obj = loot_type.as_object();
			model::Loot loot;
			loot.name = obj.at("name").as_string();
			loot.file = obj.at("file").as_string();
			loot.type = obj.at("type").as_string();
			loot.value = obj.at("value").as_int64();

			if (obj.contains("rotation")) {
				loot.rotation = obj.at("rotation").as_int64();
			}

			if (obj.contains("color")) {
				loot.color = obj.at("color").as_string();
			}

			loot.scale = obj.at("scale").as_double();

			map.AddLootType(loot);
		}

		const auto loot_gen_cfg = value.at("lootGeneratorConfig").as_object();
		double period = loot_gen_cfg.at("period").as_double();
		double probability = loot_gen_cfg.at("probability").as_double();
		game.SetPeriod(period);
		game.SetProbability(probability);

		game.AddMap(map);
	}

	return game;
}

} // namespace json_loader
