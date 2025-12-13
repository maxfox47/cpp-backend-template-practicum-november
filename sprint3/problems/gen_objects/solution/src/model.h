#pragma once

#include <cmath>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "boost/beast/http/field.hpp"
#include "boost/core/pointer_traits.hpp"
#include "loot_generator.h"
#include "tagged.h"

class GameSession;

namespace model {

namespace fs = std::filesystem;
using Dimension = int;
using Coord = Dimension;

struct Point {
	Coord x, y;
};

struct Size {
	Dimension width, height;
};

struct Rectangle {
	Point position;
	Size size;
};

struct Offset {
	Dimension dx, dy;
};

struct Position {
	double x = 0;
	double y = 0;
};

struct Speed {
	double x = 0;
	double y = 0;
};

enum class Direction { NORTH, SOUTH, WEST, EAST };

/*
 * Структура, описывающая тип трофея на карте.
 * Используется для хранения информации о типах потерянных предметов.
 */
struct Loot {
	std::string name;                    // Название типа трофея
	std::string file;                    // Путь к файлу модели
	std::string type;                    // Тип файла (например, "obj")
	std::optional<int> rotation;         // Опциональный угол поворота
	std::optional<std::string> color;    // Опциональный цвет
	double scale;                        // Масштаб модели
};

/*
 * Структура, описывающая потерянный предмет на карте.
 * type - индекс типа трофея в массиве lootTypes карты (от 0 до N-1)
 * pos - позиция предмета на карте
 */
struct LostObject {
	int type;        // Тип трофея (индекс в массиве типов)
	Position pos;    // Позиция на карте
};

/*
 * Генерирует случайное число в диапазоне [0.0, 1.0).
 * Используется для генерации случайных значений в игровой логике.
 */
inline double GenerateRandomNumber() {
	static std::random_device random_device;
	static std::mt19937 random_generator(random_device());
	static std::uniform_real_distribution<> distribution(0.0, 1.0);

	return distribution(random_generator);
}

/*
 * Преобразует время в секундах в TimeInterval для LootGenerator.
 */
inline loot_gen::LootGenerator::TimeInterval SecondsToTimeInterval(double time_in_seconds) {
	return duration_cast<loot_gen::LootGenerator::TimeInterval>(std::chrono::duration<double>(time_in_seconds));
}

class Road {
	struct HorizontalTag {
		explicit HorizontalTag() = default;
	};

	struct VerticalTag {
		explicit VerticalTag() = default;
	};

 public:
	constexpr static HorizontalTag HORIZONTAL{};
	constexpr static VerticalTag VERTICAL{};

	Road(HorizontalTag, Point start, Coord end_x) noexcept : start_{start}, end_{end_x, start.y} {}

	Road(VerticalTag, Point start, Coord end_y) noexcept : start_{start}, end_{start.x, end_y} {}

	bool IsHorizontal() const noexcept { return start_.y == end_.y; }

	bool IsVertical() const noexcept { return start_.x == end_.x; }

	Point GetStart() const noexcept { return start_; }

	Point GetEnd() const noexcept { return end_; }

	bool IsOnRoad(Position pos) const;

 private:
	Point start_;
	Point end_;
};

class Building {
 public:
	explicit Building(Rectangle bounds) noexcept : bounds_{bounds} {}

	const Rectangle& GetBounds() const noexcept { return bounds_; }

 private:
	Rectangle bounds_;
};

class Office {
 public:
	using Id = util::Tagged<std::string, Office>;

	Office(Id id, Point position, Offset offset) noexcept
		 : id_{std::move(id)}, position_{position}, offset_{offset} {}

	const Id& GetId() const noexcept { return id_; }

	Point GetPosition() const noexcept { return position_; }

	Offset GetOffset() const noexcept { return offset_; }

 private:
	Id id_;
	Point position_;
	Offset offset_;
};

class Map {
 public:
	using Id = util::Tagged<std::string, Map>;
	using Roads = std::vector<Road>;
	using Buildings = std::vector<Building>;
	using Offices = std::vector<Office>;

	Map(Id id, std::string name) noexcept : id_(std::move(id)), name_(std::move(name)) {}

	const Id& GetId() const noexcept { return id_; }

	const std::string& GetName() const noexcept { return name_; }

	const Buildings& GetBuildings() const noexcept { return buildings_; }

	const Roads& GetRoads() const noexcept { return roads_; }

	const Offices& GetOffices() const noexcept { return offices_; }

	void AddRoad(const Road& road) { roads_.emplace_back(road); }

	void AddBuilding(const Building& building) { buildings_.emplace_back(building); }

	void AddOffice(Office office);

	/*
	 * Добавляет тип трофея на карту.
	 * Типы трофеев определяют, какие предметы могут появиться на карте.
	 */
	void AddLootType(const Loot& loot) { loot_types_.push_back(loot); }

	/*
	 * Возвращает случайную позицию на случайной дороге карты.
	 * Используется для размещения потерянных предметов.
	 */
	Position GetRandomRoadPosition() const;

	void SetDefaultSpeed(double speed) { def_speed_ = speed; }

	double GetDefaultSpeed() const { return def_speed_; }

	std::pair<Position, bool> MoveDog(Position pos, Speed speed, double time_ms) const;

	std::vector<const Road*> IsOnRoad(Position pos) const;

	const std::vector<Loot>& GetLootTypes() const { return loot_types_; }

 private:
	bool IsLineOnRoad(Position p1, Position p2) const;

	using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

	Id id_;
	std::string name_;
	Roads roads_;
	Buildings buildings_;

	OfficeIdToIndex warehouse_id_to_index_;
	Offices offices_;
	double def_speed_ = 1;
	std::vector<Loot> loot_types_;
};

class Dog {
 public:
	explicit Dog(std::string name, uint64_t id) : name_(name), id_(id) {}

	const std::string& GetName() const { return name_; }

	uint64_t GetId() const { return id_; }

	Speed GetSpeed() const { return speed_; }

	Position GetPosition() const { return pos_; }

	Direction GetDirection() const { return dir_; }

	void SetPosition(Position pos) { pos_ = pos; }

	void SetSpeed(Speed speed) { speed_ = speed; }

	void SetDirection(Direction dir) { dir_ = dir; }

 private:
	std::string name_;
	uint64_t id_;
	Position pos_;
	Speed speed_;
	Direction dir_ = Direction::NORTH;
};

/*
 * Игровая сессия - экземпляр игры на конкретной карте.
 * Управляет собаками, потерянными предметами и генерацией трофеев.
 */
class GameSession {
 public:
	using Dogs = std::deque<Dog>;
	
	/*
	 * Создает игровую сессию на указанной карте.
	 * @param map - указатель на карту
	 * @param period - период генерации трофеев в секундах
	 * @param probability - вероятность появления трофея за период
	 */
	explicit GameSession(const Map* map, double period, double probability)
		 : map_(map),
			loot_gen_(SecondsToTimeInterval(period), probability, GenerateRandomNumber) {}

	bool operator==(const GameSession& other) const;
	Dog* AddDog(std::string name);
	double GetDefaultSpeed() const;
	const Map* GetMap() const;
	
	/*
	 * Обновляет состояние игровой сессии за указанный промежуток времени.
	 * Обновляет позиции собак и генерирует новые потерянные предметы.
	 * @param time_delta_ms - время в миллисекундах
	 */
	void Tick(double time_delta_ms);

	/*
	 * Возвращает список всех потерянных предметов на карте.
	 */
	const std::deque<LostObject>& GetLostObjects() const { return lost_objects_; }

 private:
	uint64_t last_id_ = 0;
	Dogs dogs_;
	const Map* map_;
	std::deque<LostObject> lost_objects_;
	loot_gen::LootGenerator loot_gen_;
};

/*
 * Основной класс игры, управляющий всеми картами и игровыми сессиями.
 * Хранит настройки генератора трофеев для всех сессий.
 */
class Game {
 public:
	using Maps = std::vector<Map>;

	void AddMap(Map map);
	const Maps& GetMaps() const noexcept;
	const Map* FindMap(const Map::Id& id) const noexcept;
	GameSession* AddGameSession(GameSession session);
	GameSession* FindGameSession(const GameSession& session);
	
	/*
	 * Обновляет состояние всех игровых сессий.
	 * @param time_delta_ms - время в миллисекундах
	 */
	void Tick(double time_delta_ms);
	
	// Настройки генератора трофеев
	void SetPeriod(double period) { loot_period_ = period; }
	void SetProbability(double probability) { loot_probability_ = probability; }
	double GetPeriod() const { return loot_period_; }
	double GetProbability() const { return loot_probability_; }
	
	const std::vector<GameSession>& GetGameSessions() const { return sessions_; }

 private:
	using MapIdHasher = util::TaggedHasher<Map::Id>;
	using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

	Maps maps_;
	MapIdToIndex map_id_to_index_;
	std::vector<GameSession> sessions_;
	double loot_period_;
	double loot_probability_;
};

} // namespace model
