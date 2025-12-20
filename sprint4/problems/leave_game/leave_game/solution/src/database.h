#pragma once

#include "conn_pull.h"

#include <string>

namespace database {

struct RetiredPlayer {
	std::string name;
	int score;
	double play_time;
};

class Database {
 public:
	Database(ConnectionPool& pool) : pool_(pool) {}

	void InitDb();
	void SaveRetiredPlayer(const std::string& name, int score, double play_time);
	std::vector<RetiredPlayer> GetRecords(int start = 0, int max_items = 100);

 private:
	ConnectionPool& pool_;
};

} // namespace database
