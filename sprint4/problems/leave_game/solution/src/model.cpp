#include "model.h"
#include "collision_detector.h"

#include <algorithm>
#include <random>
#include <set>
#include <stdexcept>

namespace model {
using namespace std::literals;

bool Road::IsOnRoad(geom::Point2D pos) const {
	if (IsHorizontal()) {
		double min_x = std::min(start_.x, end_.x);
		double max_x = std::max(start_.x, end_.x);
		if (pos.x >= min_x - HALF_WIDTH && pos.x <= max_x + HALF_WIDTH &&
			 std::abs(pos.y - start_.y) <= HALF_WIDTH) {
			return true;
		}
	} else {
		double min_y = std::min(start_.y, end_.y);
		double max_y = std::max(start_.y, end_.y);
		if (pos.y >= min_y - HALF_WIDTH && pos.y <= max_y + HALF_WIDTH &&
			 std::abs(pos.x - start_.x) <= HALF_WIDTH) {
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

geom::Point2D Map::GetRandomRoadPosition() const {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(0, roads_.size() - 1);
	int road_num = distrib(gen);
	const Road& road = roads_[road_num];
	geom::Point2D pos;

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

std::vector<const Road*> Map::IsOnRoad(geom::Point2D pos) const {
	std::vector<const Road*> result;

	for (const auto& road : roads_) {
		if (road.IsOnRoad(pos)) {
			result.push_back(&road);
		}
	}

	return result;
}

bool Map::IsLineOnRoad(geom::Point2D p1, geom::Point2D p2) const {
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

std::pair<geom::Point2D, bool> Map::MoveDog(geom::Point2D pos, geom::Vec2D speed,
														  double time_ms) const {
	if (speed.x == 0 && speed.y == 0) {
		return {pos, false};
	}

	double time_s = time_ms / 1000;
	geom::Point2D target_pos = {pos.x + speed.x * time_s, pos.y + speed.y * time_s};

	if (IsLineOnRoad(pos, target_pos)) {
		return {target_pos, false};
	}

	std::vector<const Road*> roads_at_pos = IsOnRoad(pos);
	bool moving_horizontally = (speed.x != 0);
	geom::Point2D max_pos;

	for (const Road* road : roads_at_pos) {
		if (road->IsHorizontal()) {
			if (moving_horizontally) {
				double x = speed.x > 0 ? (std::max(road->GetStart().x, road->GetEnd().x) + 0.4)
											  : (std::min(road->GetStart().x, road->GetEnd().x) - 0.4);
				geom::Point2D new_max_pos = {x, pos.y};
				max_pos = (speed.x > 0 && new_max_pos.x > max_pos.x) ||
										(speed.x < 0 && new_max_pos.x < max_pos.x)
								  ? new_max_pos
								  : max_pos;
			} else {
				double y = speed.y > 0 ? (road->GetStart().y + 0.4) : (road->GetStart().y - 0.4);
				geom::Point2D new_max_pos = {pos.x, y};
				max_pos = (speed.y > 0 && new_max_pos.y > max_pos.y) ||
										(speed.y < 0 && new_max_pos.y < max_pos.y)
								  ? new_max_pos
								  : max_pos;
			}
		} else {
			if (moving_horizontally) {
				double x = speed.x > 0 ? (road->GetStart().x + 0.4) : (road->GetStart().x - 0.4);
				geom::Point2D new_max_pos = {x, pos.y};
				max_pos = (speed.x > 0 && new_max_pos.x > max_pos.x) ||
										(speed.x < 0 && new_max_pos.x < max_pos.x)
								  ? new_max_pos
								  : max_pos;
			} else {
				double y = speed.y > 0 ? (std::max(road->GetStart().y, road->GetEnd().y) + 0.4)
											  : (std::min(road->GetStart().y, road->GetEnd().y) - 0.4);
				geom::Point2D new_max_pos = {pos.x, y};
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
	dogs_.back().SetBagCapacity(map_->GetBagCapacity());
	return &dogs_.back();
}

double GameSession::GetDefaultSpeed() const { return map_->GetDefaultSpeed(); }

void GameSession::Tick(double ms) {
	collision_detector::ItemGathererProvider provider;
	for (Dog& dog : dogs_) {
		geom::Point2D old_pos = dog.GetPosition();
		auto [new_pos, should_stop] = map_->MoveDog(dog.GetPosition(), dog.GetSpeed(), ms);
		dog.SetPosition(new_pos);
		if (should_stop) {
			dog.SetSpeed({0, 0});
		}

		dog.AddPlayTime(ms / 1000.0);
		dog.UpdateIdleTime(ms);

		provider.AddGatherer({old_pos, new_pos, 0.3});
	}

	for (LostObject obj : lost_objects_) {
		provider.AddItem({{obj.pos.x, obj.pos.y}, 0});
	}

	for (const Office& office : map_->GetOffices()) {
		provider.AddItem({{static_cast<double>(office.GetPosition().x),
								 static_cast<double>(office.GetPosition().y)},
								0.25,
								true});
	}

	std::vector<collision_detector::GatheringEvent> events =
		 collision_detector::FindGatherEvents(provider);
	std::set<size_t, std::greater<size_t>> taken_items_;

	for (collision_detector::GatheringEvent event : events) {
		Dog& dog = dogs_[event.gatherer_id];

		if (provider.GetItem(event.item_id).is_office) {
			const std::vector<TakenItem>& bag = dog.GetBag();
			const std::vector<Loot>& loot_types = map_->GetLootTypes();
			for (TakenItem taken_item : bag) {
				dog.AddScore(loot_types[taken_item.type].value);
			}
			dog.ClearBag();
			continue;
		}

		if (taken_items_.contains(event.item_id) || dog.GetBagSize() == dog.GetBagCapacity()) {
			continue;
		}

		if (!dog.AddItem({lost_objects_[event.item_id].type, event.item_id})) {
			continue;
		}

		taken_items_.insert(event.item_id);
	}

	for (size_t item : taken_items_) {
		lost_objects_.erase(lost_objects_.begin() + item);
	}

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
		geom::Point2D pos = map_->GetRandomRoadPosition();
		AddLostObject({type, pos});
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
