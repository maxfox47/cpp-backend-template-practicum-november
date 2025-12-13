#pragma once

#include "geom.h"

#include <algorithm>
#include <vector>

namespace collision_detector {

struct CollectionResult {
	bool IsCollected(double collect_radius) const {
		return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collect_radius * collect_radius;
	}

	double sq_distance;

	double proj_ratio;
};

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c);

struct Item {
	geom::Point2D position;
	double width;
	bool is_office = false;
};

struct Gatherer {
	geom::Point2D start_pos;
	geom::Point2D end_pos;
	double width;
};

class ItemGathererProvider {
 public:
	size_t ItemsCount() const { return items_.size(); }

	Item GetItem(size_t idx) const { return items_.at(idx); }

	size_t GatherersCount() const { return gatherers_.size(); }

	Gatherer GetGatherer(size_t idx) const { return gatherers_.at(idx); }

	void AddItem(Item item) { items_.push_back(item); }

	void AddGatherer(Gatherer gatherer) { gatherers_.push_back(gatherer); }

 private:
	std::vector<Item> items_;
	std::vector<Gatherer> gatherers_;
};

struct GatheringEvent {
	size_t item_id;
	size_t gatherer_id;
	double sq_distance;
	double time;
};

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider);

} // namespace collision_detector
