#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "geom.h"
#include "loot_generator.h"
#include "tagged.h"

class GameSession;

namespace model {

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

enum class Direction { NORTH, SOUTH, WEST, EAST };

struct Loot {
	std::string name;
	std::string file;
	std::string type;
	std::optional<int> rotation;
	std::optional<std::string> color;
	double scale;
	int value;
};

struct LostObject {
	int type;
	geom::Point2D pos;
};

struct TakenItem {
	int type;
	size_t id;
};

inline bool operator==(const TakenItem& v1, const TakenItem& v2) {
	return v1.type == v2.type && v1.id == v2.id;
}

inline double GenerateRandomNumber() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<> dis(0.0, 1.0);

	return dis(gen);
}

inline loot_gen::LootGenerator::TimeInterval SecondsToTimeInterval(double time) {
	return duration_cast<loot_gen::LootGenerator::TimeInterval>(std::chrono::duration<double>(time));
}

class Road {
	const double HALF_WIDTH = 0.4;

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

	bool IsOnRoad(geom::Point2D pos) const;

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

	void AddLootType(const Loot& loot) { loot_types_.push_back(loot); }

	geom::Point2D GetRandomRoadPosition() const;

	void SetDefaultSpeed(double speed) { def_speed_ = speed; }

	double GetDefaultSpeed() const { return def_speed_; }

	std::pair<geom::Point2D, bool> MoveDog(geom::Point2D pos, geom::Vec2D speed,
														double time_ms) const;

	std::vector<const Road*> IsOnRoad(geom::Point2D pos) const;

	const std::vector<Loot>& GetLootTypes() const { return loot_types_; }

	void SetBagCapacity(int capacity) { bag_capacity_ = capacity; }

	int GetBagCapacity() const { return bag_capacity_; }

 private:
	bool IsLineOnRoad(geom::Point2D p1, geom::Point2D p2) const;

	using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

	Id id_;
	std::string name_;
	Roads roads_;
	Buildings buildings_;

	OfficeIdToIndex warehouse_id_to_index_;
	Offices offices_;
	double def_speed_ = 1;
	int bag_capacity_ = 3;
	std::vector<Loot> loot_types_;
};

class Dog {
 public:
	using BagContent = std::vector<TakenItem>;

	explicit Dog(std::string name, uint64_t id) : name_(name), id_(id) {}

	const std::string& GetName() const { return name_; }

	uint64_t GetId() const { return id_; }

	geom::Vec2D GetSpeed() const { return speed_; }

	geom::Point2D GetPosition() const { return pos_; }

	Direction GetDirection() const { return dir_; }

	void SetPosition(geom::Point2D pos) { pos_ = pos; }

	void SetSpeed(geom::Vec2D speed) { speed_ = speed; }

	void SetDirection(Direction dir) { dir_ = dir; }

	bool AddItem(TakenItem item) {
		if (GetBagSize() == GetBagCapacity()) {
			return false;
		}

		bag_.push_back(item);
		return true;
	}

	const BagContent& GetBag() const { return bag_; }

	void ClearBag() { bag_.clear(); }

	size_t GetBagSize() const { return bag_.size(); }

	void AddScore(int score) { score_ += score; }

	int GetScore() const { return score_; }

	void SetBagCapacity(int capacity) { bag_capacity_ = capacity; }

	int GetBagCapacity() const { return bag_capacity_; }

	double GetIdleTime() const { return idle_time_; }

	void SetIdleTime(double time) { idle_time_ = time; }

	void AddIdleTime(double ms) { idle_time_ += ms; }

	double GetPlayTime() const { return play_time_; }

	void SetPlayTime(double time) { play_time_ = time; }

	void AddPlayTime(double ms) { play_time_ += ms; }

 private:
	std::string name_;
	uint64_t id_;
	geom::Point2D pos_;
	geom::Vec2D speed_;
	Direction dir_ = Direction::NORTH;
	BagContent bag_;
	int score_ = 0;
	int bag_capacity_ = 3;
	double idle_time_ = 0;
	double play_time_ = 0;
};

class GameSession {
 public:
	using Dogs = std::deque<Dog>;
	explicit GameSession(const Map* map, double period, double probability)
		 : map_(map), loot_gen_(SecondsToTimeInterval(period), probability, GenerateRandomNumber) {}

	bool operator==(const GameSession& other) const;
	Dog* AddDog(std::string name);
	double GetDefaultSpeed() const;
	const Map* GetMap() const;
	void Tick(double ms);
	const Dogs& GetDogs() const { return dogs_; }
	uint64_t GetLastDogId() const { return last_id_; }
	void SetLastDogId(uint64_t id) { last_id_ = id; }
	void AddExistingDog(Dog&& dog) { dogs_.push_back(std::move(dog)); }
	void AddLostObject(LostObject obj) { lost_objects_.push_back(obj); }
	Dog* FindDogById(uint64_t id) {
		for (Dog& dog : dogs_) {
			if (dog.GetId() == id) {
				return &dog;
			}
		}

		return nullptr;
	}

	const std::deque<LostObject>& GetLostObjects() const { return lost_objects_; }

 private:
	uint64_t last_id_ = 0;
	Dogs dogs_;
	const Map* map_;
	std::deque<LostObject> lost_objects_;
	loot_gen::LootGenerator loot_gen_;
};

class Game {
 public:
	using Maps = std::vector<Map>;

	void AddMap(Map map);
	const Maps& GetMaps() const noexcept;
	const Map* FindMap(const Map::Id& id) const noexcept;
	GameSession* AddGameSession(GameSession session);
	GameSession* FindGameSession(const GameSession& session);
	void Tick(double ms);
	void SetPeriod(double period) { loot_period_ = period; }
	void SetProbability(double probability) { loot_probability_ = probability; }
	double GetPeriod() const { return loot_period_; }
	double GetProbability() const { return loot_probability_; }
	const std::deque<GameSession>& GetGameSessions() const { return sessions_; }
	GameSession* FindSessionByMap(const model::Map* map) {
		for (GameSession& session : sessions_) {
			if (session.GetMap() == map) {
				return &session;
			}
		}

		return nullptr;
	}
	void SetDogRetirementTime(double time) { dog_retirement_time_ = time; }
	double GetDogRetirementTime() const { return dog_retirement_time_; }

 private:
	using MapIdHasher = util::TaggedHasher<Map::Id>;
	using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

	Maps maps_;
	MapIdToIndex map_id_to_index_;
	std::deque<GameSession> sessions_;
	double loot_period_;
	double loot_probability_;
	double dog_retirement_time_ = 60000.0;
};

} // namespace model
