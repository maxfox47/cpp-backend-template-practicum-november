#include "endpoint.h"

bool EndPoint::IsApiReq() const {
	return endpoint_.starts_with("/api");
}

bool EndPoint::IsMapsReq() const {
	return (endpoint_ == "/api/v1/maps" || endpoint_ == "/api/v1/maps/");
}

bool EndPoint::IsSpecificMapReq() const {
	return (endpoint_ != "/api/v1/maps/" && endpoint_.starts_with("/api/v1/maps/"));
}

bool EndPoint::IsJoinReq() const {
	return endpoint_ == "/api/v1/game/join";
}

bool EndPoint::IsPlayersReq() const {
	return endpoint_ == "/api/v1/game/players";
}

bool EndPoint::IsStateReq() const {
	return endpoint_ == "/api/v1/game/state";
}

bool EndPoint::IsActionReq() const {
	return endpoint_ == "/api/v1/game/player/action";
}

bool EndPoint::IsTickReq() const {
	return endpoint_ == "/api/v1/game/tick";
}

const std::string& EndPoint::GetEndPoint() const {
	return endpoint_;
}

