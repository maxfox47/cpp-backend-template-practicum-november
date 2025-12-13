#pragma once

#include "model.h"

#include <boost/json.hpp>
#include <deque>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace json = boost::json;

namespace app {

struct PlayerInfo {
	model::Position pos;
	model::Speed speed;
	model::Direction dir;
};

class Player {
 public:
	explicit Player(model::GameSession* session, model::Dog* dog, uint64_t id)
		 : dog_(dog), session_(session), id_(id) {
		if (!session)
			throw std::invalid_argument("Session cannot be null");
		if (!dog)
			throw std::invalid_argument("Dog cannot be null");
	}

	std::string GetName() const {
		if (!dog_) {
			return "unknown";
		}
		return dog_->GetName();
	}

	uint64_t GetId() const { return id_; }

	PlayerInfo GetInfo() const {
		return {dog_->GetPosition(), dog_->GetSpeed(), dog_->GetDirection()};
	}

	void SetSpeed(model::Speed speed) {
		dog_->SetSpeed(speed);
	}

	void SetDirection(model::Direction dir) {
		dog_->SetDirection(dir);
	}

	double GetDefaultSpeed() const {
		return session_->GetDefaultSpeed();
	}

 private:
	model::Dog* dog_;
	model::GameSession* session_;
	uint64_t id_;
};

class Players {
 public:
	Player* Add(model::GameSession* session, model::Dog* dog);
	Player* FindByDogIdAndMapId(int dog_id, const std::string& map_id);
	std::vector<std::string> GetNames() const;
	json::object GetPlayersInfo() const;

 private:
	std::deque<Player> players_;
	uint64_t last_player_id_ = 0;
};

namespace detail {
struct TokenTag {};
} // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

class PlayerTokens {
 public:
	PlayerTokens() = default;

	Player* FindPlayerByToken(Token token) {
		auto it = token_to_player_.find(token);
		if (it != token_to_player_.end()) {
			return it->second;
		}

		return nullptr;
	}

	Token AddPlayer(Player* player) {
		Token token = MakeToken();
		token_to_player_[token] = player;

		return token;
	}

	Token MakeToken() {
		std::stringstream ss;
		uint64_t num1 = generator1_();
		uint64_t num2 = generator2_();

		ss << std::hex << std::setw(16) << std::setfill('0') << num1;
		ss << std::hex << std::setw(16) << std::setfill('0') << num2;

		return Token{ss.str()};
	}

 private:
	std::random_device random_device_;
	std::mt19937_64 generator1_{[this] {
		std::uniform_int_distribution<std::mt19937_64::result_type> dist;
		return dist(random_device_);
	}()};
	std::mt19937_64 generator2_{[this] {
		std::uniform_int_distribution<std::mt19937_64::result_type> dist;
		return dist(random_device_);
	}()};

	std::unordered_map<Token, Player*, util::TaggedHasher<Token>> token_to_player_;
};

} // namespace app


