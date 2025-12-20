#pragma once

#include "conn_pull.h"

#include <string>
#include <vector>

namespace database {

struct RetiredPlayerRecord {
	std::string name;
	int score;
	double play_time;
};

class Database {
 public:
	explicit Database(ConnectionPool& pool) : pool_{pool} {}

	void InitializeSchema();
	void SaveRetiredPlayer(const std::string& name, int score, double play_time);
	std::vector<RetiredPlayerRecord> GetRecords(int start_index = 0, int max_items_count = 100);

 private:
	ConnectionPool& pool_;
};

}  // namespace database
