#pragma once

#include <string>

class EndPoint {
 public:
	EndPoint(const std::string& endpoint) : endpoint_(endpoint) {}
	
	bool IsApiReq() const;
	bool IsMapsReq() const;
	bool IsSpecificMapReq() const;
	bool IsJoinReq() const;
	bool IsPlayersReq() const;
	bool IsStateReq() const;
	bool IsActionReq() const;
	bool IsTickReq() const;
	const std::string& GetEndPoint() const;

 private:
	std::string endpoint_;
};

