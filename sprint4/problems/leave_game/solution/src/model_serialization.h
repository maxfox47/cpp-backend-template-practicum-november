#include <boost/serialization/deque.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include "model.h"

#include "geom.h"

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
	ar & point.x;
	ar & point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
	ar & vec.x;
	ar & vec.y;
}

} // namespace geom

namespace serialization {

class DogRepr {
 public:
	DogRepr() = default;

	explicit DogRepr(const model::Dog& dog)
		 : id_(dog.GetId()), name_(dog.GetName()), pos_(dog.GetPosition()),
			bag_capacity_(dog.GetBagCapacity()), speed_(dog.GetSpeed()),
			direction_(dog.GetDirection()), score_(dog.GetScore()), bag_content_(dog.GetBag()),
			idle_time_(dog.GetIdleTime()), play_time_(dog.GetPlayTime()) {}

	[[nodiscard]] model::Dog Restore() const {
		model::Dog dog{name_, id_};
		dog.SetPosition(pos_);
		dog.SetBagCapacity(bag_capacity_);
		dog.SetSpeed(speed_);
		dog.SetDirection(direction_);
		dog.AddScore(score_);
		for (const auto& item : bag_content_) {
			if (!dog.AddItem(item)) {
				throw std::runtime_error("Failed to put bag content");
			}
		}
		dog.SetIdleTime(idle_time_);
		dog.SetPlayTime(play_time_);
		return dog;
	}

	template <typename Archive>
	void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
		ar & id_;
		ar & name_;
		ar & pos_;
		ar & bag_capacity_;
		ar & speed_;
		ar & direction_;
		ar & score_;
		ar & bag_content_;
		ar & idle_time_;
		ar & play_time_;
	}

 private:
	uint64_t id_ = 0;
	std::string name_;
	geom::Point2D pos_;
	size_t bag_capacity_ = 0;
	geom::Vec2D speed_;
	model::Direction direction_ = model::Direction::NORTH;
	int score_ = 0;
	model::Dog::BagContent bag_content_;
	double idle_time_ = 0;
	double play_time_ = 0;
};

} // namespace serialization

namespace app::serialization {

struct PlayerRepr {
	uint64_t id = 0;
	uint64_t dog_id = 0;
	std::string map_id;

	template <typename Archive>
	void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
		ar & id;
		ar & dog_id;
		ar & map_id;
	}
};

struct TokenRepr {
	std::string token;
	uint64_t player_id = 0;

	template <typename Archive>
	void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
		ar & token;
		ar & player_id;
	}
};

struct PlayersRepr {
	std::vector<PlayerRepr> players;
	std::vector<TokenRepr> tokens;
	uint64_t last_player_id = 0;

	template <typename Archive>
	void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
		ar & players;
		ar & tokens;
		ar & last_player_id;
	}
};

} // namespace app::serialization

namespace model {

template <typename Archive>
void serialize(Archive& ar, model::TakenItem& obj, [[maybe_unused]] const unsigned version) {
	ar&(obj.type);
	ar&(obj.id);
}

struct LostObjectRepr {
	int type = 0;
	geom::Point2D pos{};

	template <typename Archive>
	void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
		ar & type;
		ar & pos;
	}
};

struct GameSessionRepr {
	std::string map_id;
	std::deque<serialization::DogRepr> dogs;
	std::deque<LostObjectRepr> lost_objects;
	uint64_t last_dog_id = 0;

	template <typename Archive>
	void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
		ar & map_id;
		ar & dogs;
		ar & lost_objects;
		ar & last_dog_id;
	}
};

struct GameStateRepr {
	std::vector<GameSessionRepr> sessions;

	template <typename Archive>
	void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
		ar & sessions;
	}
};

} // namespace model

namespace serialization {

struct StateRepr {
	model::GameStateRepr game_state;
	app::serialization::PlayersRepr players_state;

	template <typename Archive>
	void serialize(Archive& ar, const unsigned version) {
		ar & game_state;
		ar & players_state;
	}
};

} // namespace serialization
