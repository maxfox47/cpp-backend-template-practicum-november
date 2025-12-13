#include "model.h"
#include "collision_detector.h"

#include <algorithm>
#include <random>
#include <set>
#include <stdexcept>
#include <cmath>

namespace model {
using namespace std::literals;

bool Road::IsOnRoad(Position pos) const {
	double half_width = 0.4;
	if (IsHorizontal()) {
		double min_x = std::min(start_.x, end_.x);
		double max_x = std::max(start_.x, end_.x);
		if (pos.x >= min_x - half_width && pos.x <= max_x + half_width &&
			 std::abs(pos.y - start_.y) <= half_width) {
			return true;
		}
	} else {
		double min_y = std::min(start_.y, end_.y);
		double max_y = std::max(start_.y, end_.y);
		if (pos.y >= min_y - half_width && pos.y <= max_y + half_width &&
			 std::abs(pos.x - start_.x) <= half_width) {
			return true;
		}
	}
	return false;
}

void Map::AddOffice(Office office) {
	if (warehouse_id_to_index_.contains(office.GetId())) {
		throw std::invalid_argument("Duplicate warehouse");
	}

	const size_t index = offices_.size();
	Office& o = offices_.emplace_back(std::move(office));
	try {
		warehouse_id_to_index_.emplace(o.GetId(), index);
	} catch (...) {
		offices_.pop_back();
		throw;
	}
}

Position Map::GetRandomRoadPosition() const {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(0, roads_.size() - 1);
	int road_num = distrib(gen);
	const Road& road = roads_[road_num];
	Position pos;

	if (road.IsHorizontal()) {
		int min_x = std::min(road.GetStart().x, road.GetEnd().x);
		int max_x = std::max(road.GetStart().x, road.GetEnd().x);
		std::uniform_real_distribution<> distrib_pos(min_x, max_x);
		pos.x = distrib_pos(gen);
		pos.y = static_cast<double>(road.GetStart().y);
	}

	if (road.IsVertical()) {
		int min_y = std::min(road.GetStart().y, road.GetEnd().y);
		int max_y = std::max(road.GetStart().y, road.GetEnd().y);
		std::uniform_real_distribution<> distrib_pos(road.GetStart().y, road.GetEnd().y);
		pos.y = distrib_pos(gen);
		pos.x = static_cast<double>(road.GetStart().x);
	}

	return pos;
}

std::vector<const Road*> Map::IsOnRoad(Position pos) const {
	const double half_width = 0.4;
	std::vector<const Road*> result;

	for (const auto& road : roads_) {
		if (road.IsOnRoad(pos)) {
			result.push_back(&road);
		}
	}

	return result;
}

bool Map::IsLineOnRoad(Position p1, Position p2) const {
	std::vector<const Road*> v1 = IsOnRoad(p1);
	std::vector<const Road*> v2 = IsOnRoad(p2);

	if (v1.empty() || v2.empty()) {
		return false;
	}

	for (const Road* r1 : v1) {
		for (const Road* r2 : v2) {
			if (r1 == r2) {
				return true;
			}
		}
	}

	return false;
}

std::pair<Position, bool> Map::MoveDog(Position pos, Speed speed, double time_ms) const {
	if (speed.x == 0 && speed.y == 0) {
		return {pos, false};
	}

	double half_width = 0.4;
	double time_s = time_ms / 1000;
	Position target_pos = {pos.x + speed.x * time_s, pos.y + speed.y * time_s};

	if (IsLineOnRoad(pos, target_pos)) {
		return {target_pos, false};
	}

	std::vector<const Road*> roads_at_pos = IsOnRoad(pos);
	bool moving_horizontally = (speed.x != 0);
	Position max_pos;

	for (const Road* road : roads_at_pos) {
		if (road->IsHorizontal()) {
			if (moving_horizontally) {
				double x = speed.x > 0 ? (std::max(road->GetStart().x, road->GetEnd().x) + 0.4)
											  : (std::min(road->GetStart().x, road->GetEnd().x) - 0.4);
				Position new_max_pos = {x, pos.y};
				max_pos = (speed.x > 0 && new_max_pos.x > max_pos.x) ||
										(speed.x < 0 && new_max_pos.x < max_pos.x)
								  ? new_max_pos
								  : max_pos;
			} else {
				double y = speed.y > 0 ? (road->GetStart().y + 0.4) : (road->GetStart().y - 0.4);
				Position new_max_pos = {pos.x, y};
				max_pos = (speed.y > 0 && new_max_pos.y > max_pos.y) ||
										(speed.y < 0 && new_max_pos.y < max_pos.y)
								  ? new_max_pos
								  : max_pos;
			}
		} else {
			if (moving_horizontally) {
				double x = speed.x > 0 ? (road->GetStart().x + 0.4) : (road->GetStart().x - 0.4);
				Position new_max_pos = {x, pos.y};
				max_pos = (speed.x > 0 && new_max_pos.x > max_pos.x) ||
										(speed.x < 0 && new_max_pos.x < max_pos.x)
								  ? new_max_pos
								  : max_pos;
			} else {
				double y = speed.y > 0 ? (std::max(road->GetStart().y, road->GetEnd().y) + 0.4)
											  : (std::min(road->GetStart().y, road->GetEnd().y) - 0.4);
				Position new_max_pos = {pos.x, y};
				max_pos = (speed.y > 0 && new_max_pos.y > max_pos.y) ||
										(speed.y < 0 && new_max_pos.y < max_pos.y)
								  ? new_max_pos
								  : max_pos;
			}
		}
	}

	return {max_pos, true};
}

bool GameSession::operator==(const GameSession& other) const { return map_ == other.map_; }

Dog* GameSession::AddDog(std::string name) {
	dogs_.push_back(Dog(name, last_id_++));
	return &dogs_.back();
}

double GameSession::GetDefaultSpeed() const { return map_->GetDefaultSpeed(); }

void GameSession::Tick(double ms) {
	// Сохраняем начальные позиции собак
	std::vector<Position> start_positions;
	for (const Dog& dog : dogs_) {
		start_positions.push_back(dog.GetPosition());
	}

	// Перемещаем собак
	for (Dog& dog : dogs_) {
		auto [new_pos, should_stop] = map_->MoveDog(dog.GetPosition(), dog.GetSpeed(), ms);
		dog.SetPosition(new_pos);
		if (should_stop) {
			dog.SetSpeed({0, 0});
		}
	}

	// Генерируем новые предметы
	int loot_count =
		 loot_gen_.Generate(SecondsToTimeInterval(ms / 1000), lost_objects_.size(), dogs_.size());
	int max_type = map_->GetLootTypes().size();
	auto random_type = [max_type]() {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dist(0, max_type - 1);
		return dist(gen);
	};

	for (int i = 0; i < loot_count; ++i) {
		int type = random_type();
		Position pos = map_->GetRandomRoadPosition();
		lost_objects_.push_back(LostObject{last_lost_object_id_++, type, pos});
	}

	// Обрабатываем сбор предметов и возврат на базу
	if (!dogs_.empty() && (!lost_objects_.empty() || !map_->GetOffices().empty())) {
		double time_s = ms / 1000.0;

		// Создаем временный провайдер с правильными конечными позициями
		class TempProvider : public collision_detector::ItemGathererProvider {
		public:
			TempProvider(const std::deque<model::Dog>& dogs,
							  const std::vector<model::Position>& start_positions,
							  const std::deque<model::LostObject>& lost_objects,
							  const std::vector<model::Office>& offices,
							  double time_s)
				 : dogs_(dogs),
					start_positions_(start_positions),
					lost_objects_(lost_objects),
					offices_(offices),
					time_s_(time_s) {}

			size_t ItemsCount() const override {
				return lost_objects_.size() + offices_.size();
			}

			collision_detector::Item GetItem(size_t idx) const override {
				if (idx < lost_objects_.size()) {
					const auto& obj = lost_objects_[idx];
					return collision_detector::Item{
						 {obj.pos.x, obj.pos.y},
						 0.0  // ширина предмета = 0
					};
				} else {
					const auto& office = offices_[idx - lost_objects_.size()];
					model::Point pos = office.GetPosition();
					return collision_detector::Item{
						 {static_cast<double>(pos.x), static_cast<double>(pos.y)},
						 0.5  // ширина базы = 0.5
					};
				}
			}

			size_t GatherersCount() const override { return dogs_.size(); }

			collision_detector::Gatherer GetGatherer(size_t idx) const override {
				const Dog& dog = dogs_[idx];
				Position start_pos = start_positions_[idx];
				Speed speed = dog.GetSpeed();
				Position end_pos = {start_pos.x + speed.x * time_s_,
											 start_pos.y + speed.y * time_s_};
				return collision_detector::Gatherer{
					 {start_pos.x, start_pos.y},
					 {end_pos.x, end_pos.y},
					 0.6  // ширина игрока = 0.6
				};
			}

		private:
			const std::deque<model::Dog>& dogs_;
			const std::vector<model::Position>& start_positions_;
			const std::deque<model::LostObject>& lost_objects_;
			const std::vector<model::Office>& offices_;
			double time_s_;
		};

		TempProvider temp_provider(dogs_, start_positions, lost_objects_, map_->GetOffices(),
											time_s);
		auto events = collision_detector::FindGatherEvents(temp_provider);

		// Обрабатываем события в хронологическом порядке
		std::set<size_t> collected_items;  // Индексы собранных предметов
		for (const auto& event : events) {
			if (event.gatherer_id >= dogs_.size()) {
				continue;
			}

			Dog& dog = dogs_[event.gatherer_id];

			// Проверяем, это предмет или база
			if (event.item_id < lost_objects_.size()) {
				// Это предмет
				if (collected_items.count(event.item_id) == 0 &&
					 dog.GetBagSize() < bag_capacity_) {
					// Собираем предмет
					const auto& obj = lost_objects_[event.item_id];
					dog.AddToBag(BagItem{static_cast<int>(obj.id), obj.type});
					collected_items.insert(event.item_id);
				}
			} else {
				// Это база - возвращаем все предметы и начисляем очки
				const auto& bag = dog.GetBag();
				if (!bag.empty()) {
					// Начисляем очки за каждый предмет
					for (const auto& item : bag) {
						// Находим тип предмета в loot_types
						const auto& loot_types = map_->GetLootTypes();
						if (item.type >= 0 && static_cast<size_t>(item.type) < loot_types.size()) {
							const auto& loot = loot_types[item.type];
							dog.AddScore(loot.value);
						}
					}
					dog.ClearBag();
				}
			}
		}

		// Удаляем собранные предметы из списка
		std::deque<LostObject> remaining_objects;
		for (size_t i = 0; i < lost_objects_.size(); ++i) {
			if (collected_items.count(i) == 0) {
				remaining_objects.push_back(lost_objects_[i]);
			}
		}
		lost_objects_ = std::move(remaining_objects);
	}
}

void Game::AddMap(Map map) {
	const size_t index = maps_.size();
	if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
		throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
	} else {
		try {
			maps_.emplace_back(std::move(map));
		} catch (...) {
			map_id_to_index_.erase(it);
			throw;
		}
	}
}

const Game::Maps& Game::GetMaps() const noexcept { return maps_; }

const Map* Game::FindMap(const Map::Id& id) const noexcept {
	if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
		return &maps_.at(it->second);
	}

	return nullptr;
}

GameSession* Game::AddGameSession(GameSession session) {
	if (auto existing_session = FindGameSession(session)) {
		return existing_session;
	}

	sessions_.push_back(std::move(session));
	return &sessions_.back();
}

GameSession* Game::FindGameSession(const GameSession& session) {
	for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
		if (*it == session) {
			return &(*it);
		}
	}

	return nullptr;
}

const Map* GameSession::GetMap() const { return map_; }

void Game::Tick(double ms) {
	for (GameSession& session : sessions_) {
		session.Tick(ms);
	}
}

} // namespace model
