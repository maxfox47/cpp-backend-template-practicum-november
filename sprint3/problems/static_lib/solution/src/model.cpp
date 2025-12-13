#include "model.h"

#include <algorithm>
#include <random>
#include <stdexcept>

namespace model {
using namespace std::literals;

/*
 * Проверяет, находится ли позиция на дороге.
 * Дорога имеет ширину 0.8 (половина ширины = 0.4 с каждой стороны от оси дороги).
 */
bool Road::IsOnRoad(Position pos) const {
	constexpr double ROAD_HALF_WIDTH = 0.4;
	
	if (IsHorizontal()) {
		// Горизонтальная дорога: проверяем по оси X
		double min_x = std::min(start_.x, end_.x);
		double max_x = std::max(start_.x, end_.x);
		if (pos.x >= min_x - ROAD_HALF_WIDTH && pos.x <= max_x + ROAD_HALF_WIDTH &&
			 std::abs(pos.y - start_.y) <= ROAD_HALF_WIDTH) {
			return true;
		}
	} else {
		// Вертикальная дорога: проверяем по оси Y
		double min_y = std::min(start_.y, end_.y);
		double max_y = std::max(start_.y, end_.y);
		if (pos.y >= min_y - ROAD_HALF_WIDTH && pos.y <= max_y + ROAD_HALF_WIDTH &&
			 std::abs(pos.x - start_.x) <= ROAD_HALF_WIDTH) {
			return true;
		}
	}

	return false;
}

/*
 * Добавляет офис на карту.
 * Проверяет уникальность ID офиса.
 */
void Map::AddOffice(Office office) {
	if (warehouse_id_to_index_.contains(office.GetId())) {
		throw std::invalid_argument("Duplicate warehouse");
	}

	const size_t office_index = offices_.size();
	Office& new_office = offices_.emplace_back(std::move(office));
	try {
		warehouse_id_to_index_.emplace(new_office.GetId(), office_index);
	} catch (...) {
		offices_.pop_back();
		throw;
	}
}

/*
 * Возвращает случайную позицию на случайной дороге карты.
 * Используется для размещения потерянных предметов.
 */
Position Map::GetRandomRoadPosition() const {
	std::random_device random_device;
	std::mt19937 random_generator(random_device());
	
	// Выбираем случайную дорогу
	std::uniform_int_distribution<> road_distribution(0, roads_.size() - 1);
	int selected_road_index = road_distribution(random_generator);
	const Road& selected_road = roads_[selected_road_index];
	
	Position random_position;

	if (selected_road.IsHorizontal()) {
		// Горизонтальная дорога: случайная позиция по оси X
		int min_x = std::min(selected_road.GetStart().x, selected_road.GetEnd().x);
		int max_x = std::max(selected_road.GetStart().x, selected_road.GetEnd().x);
		std::uniform_real_distribution<> position_distribution(min_x, max_x);
		random_position.x = position_distribution(random_generator);
		random_position.y = static_cast<double>(selected_road.GetStart().y);
	}

	if (selected_road.IsVertical()) {
		// Вертикальная дорога: случайная позиция по оси Y
		int min_y = std::min(selected_road.GetStart().y, selected_road.GetEnd().y);
		int max_y = std::max(selected_road.GetStart().y, selected_road.GetEnd().y);
		std::uniform_real_distribution<> position_distribution(selected_road.GetStart().y, selected_road.GetEnd().y);
		random_position.y = position_distribution(random_generator);
		random_position.x = static_cast<double>(selected_road.GetStart().x);
	}

	return random_position;
}

/*
 * Возвращает список дорог, на которых находится указанная позиция.
 * Позиция может находиться на нескольких дорогах одновременно (пересечения).
 */
std::vector<const Road*> Map::IsOnRoad(Position pos) const {
	constexpr double ROAD_HALF_WIDTH = 0.4;
	std::vector<const Road*> roads_at_position;

	for (const auto& road : roads_) {
		if (road.IsOnRoad(pos)) {
			roads_at_position.push_back(&road);
		}
	}

	return roads_at_position;
}

/*
 * Проверяет, находится ли линия между двумя точками на одной дороге.
 * Используется для проверки движения собаки по дороге.
 */
bool Map::IsLineOnRoad(Position start_pos, Position end_pos) const {
	std::vector<const Road*> roads_at_start = IsOnRoad(start_pos);
	std::vector<const Road*> roads_at_end = IsOnRoad(end_pos);

	// Если хотя бы одна из точек не на дороге, линия не на дороге
	if (roads_at_start.empty() || roads_at_end.empty()) {
		return false;
	}

	// Проверяем, есть ли общая дорога для обеих точек
	for (const Road* road_at_start : roads_at_start) {
		for (const Road* road_at_end : roads_at_end) {
			if (road_at_start == road_at_end) {
				return true;
			}
		}
	}

	return false;
}

/*
 * Перемещает собаку по карте с учетом границ дорог.
 * Возвращает новую позицию и флаг остановки (true, если собака уперлась в границу).
 */
std::pair<Position, bool> Map::MoveDog(Position current_pos, Speed speed, double time_ms) const {
	// Если скорость нулевая, собака не движется
	if (speed.x == 0 && speed.y == 0) {
		return {current_pos, false};
	}

	constexpr double ROAD_HALF_WIDTH = 0.4;
	const double time_in_seconds = time_ms / 1000.0;
	
	// Вычисляем целевую позицию после движения
	Position target_position = {
		current_pos.x + speed.x * time_in_seconds,
		current_pos.y + speed.y * time_in_seconds
	};

	// Если путь от текущей до целевой позиции полностью на дороге, движение разрешено
	if (IsLineOnRoad(current_pos, target_position)) {
		return {target_position, false};
	}

	// Иначе находим максимальную позицию, до которой можно дойти
	std::vector<const Road*> roads_at_current_pos = IsOnRoad(current_pos);
	bool is_moving_horizontally = (speed.x != 0);
	Position max_reachable_position;

	for (const Road* road : roads_at_current_pos) {
		if (road->IsHorizontal()) {
			// Горизонтальная дорога
			if (is_moving_horizontally) {
				// Движение по горизонтали: находим границу дороги в направлении движения
				double boundary_x = speed.x > 0 
					? (std::max(road->GetStart().x, road->GetEnd().x) + ROAD_HALF_WIDTH)
					: (std::min(road->GetStart().x, road->GetEnd().x) - ROAD_HALF_WIDTH);
				Position candidate_position = {boundary_x, current_pos.y};
				
				// Обновляем максимальную достижимую позицию
				max_reachable_position = (speed.x > 0 && candidate_position.x > max_reachable_position.x) ||
												(speed.x < 0 && candidate_position.x < max_reachable_position.x)
										  ? candidate_position
										  : max_reachable_position;
			} else {
				// Движение по вертикали на горизонтальной дороге: граница по Y
				double boundary_y = speed.y > 0 
					? (road->GetStart().y + ROAD_HALF_WIDTH) 
					: (road->GetStart().y - ROAD_HALF_WIDTH);
				Position candidate_position = {current_pos.x, boundary_y};
				
				max_reachable_position = (speed.y > 0 && candidate_position.y > max_reachable_position.y) ||
												(speed.y < 0 && candidate_position.y < max_reachable_position.y)
										  ? candidate_position
										  : max_reachable_position;
			}
		} else {
			// Вертикальная дорога
			if (is_moving_horizontally) {
				// Движение по горизонтали на вертикальной дороге: граница по X
				double boundary_x = speed.x > 0 
					? (road->GetStart().x + ROAD_HALF_WIDTH) 
					: (road->GetStart().x - ROAD_HALF_WIDTH);
				Position candidate_position = {boundary_x, current_pos.y};
				
				max_reachable_position = (speed.x > 0 && candidate_position.x > max_reachable_position.x) ||
												(speed.x < 0 && candidate_position.x < max_reachable_position.x)
										  ? candidate_position
										  : max_reachable_position;
			} else {
				// Движение по вертикали: находим границу дороги в направлении движения
				double boundary_y = speed.y > 0 
					? (std::max(road->GetStart().y, road->GetEnd().y) + ROAD_HALF_WIDTH)
					: (std::min(road->GetStart().y, road->GetEnd().y) - ROAD_HALF_WIDTH);
				Position candidate_position = {current_pos.x, boundary_y};
				
				max_reachable_position = (speed.y > 0 && candidate_position.y > max_reachable_position.y) ||
												(speed.y < 0 && candidate_position.y < max_reachable_position.y)
										  ? candidate_position
										  : max_reachable_position;
			}
		}
	}

	// Возвращаем максимальную достижимую позицию и флаг остановки
	return {max_reachable_position, true};
}

bool GameSession::operator==(const GameSession& other) const { 
	return map_ == other.map_; 
}

/*
 * Добавляет новую собаку в игровую сессию.
 * Возвращает указатель на добавленную собаку.
 */
Dog* GameSession::AddDog(std::string name) {
	dogs_.push_back(Dog(name, last_id_++));
	return &dogs_.back();
}

double GameSession::GetDefaultSpeed() const { 
	return map_->GetDefaultSpeed(); 
}

/*
 * Обновляет состояние игровой сессии за указанный промежуток времени.
 * Обновляет позиции собак и генерирует новые потерянные предметы.
 */
void GameSession::Tick(double time_delta_ms) {
	// Обновляем позиции всех собак на карте
	for (Dog& dog : dogs_) {
		auto [new_position, should_stop] = map_->MoveDog(dog.GetPosition(), dog.GetSpeed(), time_delta_ms);
		dog.SetPosition(new_position);
		if (should_stop) {
			// Если собака уперлась в границу, останавливаем её
			dog.SetSpeed({0, 0});
		}
	}

	// Генерируем новые потерянные предметы
	// Конвертируем миллисекунды в секунды для генератора
	const double time_delta_seconds = time_delta_ms / 1000.0;
	const unsigned current_lost_objects_count = lost_objects_.size();
	const unsigned dogs_count = dogs_.size();
	
	unsigned new_loot_count = loot_gen_.Generate(
		SecondsToTimeInterval(time_delta_seconds), 
		current_lost_objects_count, 
		dogs_count
	);
	
	// Количество типов трофеев на карте
	const unsigned max_loot_type = map_->GetLootTypes().size();
	
	// Лямбда-функция для генерации случайного типа трофея
	auto generate_random_loot_type = [max_loot_type]() {
		std::random_device random_device;
		std::mt19937 random_generator(random_device());
		std::uniform_int_distribution<int> type_distribution(0, max_loot_type - 1);
		return type_distribution(random_generator);
	};

	// Создаем новые потерянные предметы
	for (unsigned i = 0; i < new_loot_count; ++i) {
		int loot_type = generate_random_loot_type();
		Position loot_position = map_->GetRandomRoadPosition();
		lost_objects_.push_back(LostObject{loot_type, loot_position});
	}
}

void Game::AddMap(Map map) {
	const size_t map_index = maps_.size();
	if (auto [iterator, was_inserted] = map_id_to_index_.emplace(map.GetId(), map_index); !was_inserted) {
		throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
	} else {
		try {
			maps_.emplace_back(std::move(map));
		} catch (...) {
			map_id_to_index_.erase(iterator);
			throw;
		}
	}
}

const Game::Maps& Game::GetMaps() const noexcept { 
	return maps_; 
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
	if (auto iterator = map_id_to_index_.find(id); iterator != map_id_to_index_.end()) {
		return &maps_.at(iterator->second);
	}

	return nullptr;
}

GameSession* Game::AddGameSession(GameSession session) {
	// Если сессия для этой карты уже существует, возвращаем её
	if (auto existing_session = FindGameSession(session)) {
		return existing_session;
	}

	// Иначе создаем новую сессию
	sessions_.push_back(std::move(session));
	return &sessions_.back();
}

GameSession* Game::FindGameSession(const GameSession& session) {
	for (auto iterator = sessions_.begin(); iterator != sessions_.end(); ++iterator) {
		if (*iterator == session) {
			return &(*iterator);
		}
	}

	return nullptr;
}

const Map* GameSession::GetMap() const { 
	return map_; 
}

/*
 * Обновляет состояние всех игровых сессий.
 */
void Game::Tick(double time_delta_ms) {
	for (GameSession& session : sessions_) {
		session.Tick(time_delta_ms);
	}
}

} // namespace model

