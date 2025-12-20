#pragma once

#include <string>

class EndPoint {
 public:
	explicit EndPoint(const std::string& endpoint) : endpoint_(endpoint) {}

	bool IsApiReq() const { return endpoint_.starts_with("/api"); }

	bool IsMapsReq() const { return (endpoint_ == "/api/v1/maps" || endpoint_ == "/api/v1/maps/"); }

	bool IsSpecificMapReq() const {
		return (endpoint_ != "/api/v1/maps/" && endpoint_.starts_with("/api/v1/maps/"));
	}

	bool IsJoinReq() const { return endpoint_ == "/api/v1/game/join"; }

	bool IsPlayersReq() const { return endpoint_ == "/api/v1/game/players"; }

	bool IsStateReq() const { return endpoint_ == "/api/v1/game/state"; }

	bool IsActionReq() const { return endpoint_ == "/api/v1/game/player/action"; }

	bool IsTickReq() const { return endpoint_ == "/api/v1/game/tick"; }

	bool IsRecordsReq() const {
		return endpoint_ == "/api/v1/game/records" || endpoint_.starts_with("/api/v1/game/records");
	}

	const std::string& GetEndPoint() const { return endpoint_; }

 private:
	std::string endpoint_;
};
