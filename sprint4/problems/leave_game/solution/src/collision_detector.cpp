#include "collision_detector.h"
#include <cassert>
#include <algorithm>

namespace collision_detector {

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
	const double u_x = c.x - a.x;
	const double u_y = c.y - a.y;
	const double v_x = b.x - a.x;
	const double v_y = b.y - a.y;
	const double u_dot_v = u_x * v_x + u_y * v_y;
	const double u_len2 = u_x * u_x + u_y * u_y;
	const double v_len2 = v_x * v_x + v_y * v_y;
	const double proj_ratio = u_dot_v / v_len2;
	const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

	return CollectionResult(sq_distance, proj_ratio);
}

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
	std::vector<GatheringEvent> result;

	const size_t gatherers_count = provider.GatherersCount();
	const size_t items_count = provider.ItemsCount();

	for (size_t g_id = 0; g_id < gatherers_count; ++g_id) {
		const auto gatherer = provider.GetGatherer(g_id);

		if (gatherer.start_pos.x == gatherer.end_pos.x &&
			 gatherer.start_pos.y == gatherer.end_pos.y) {
			continue;
		}

		for (size_t i_id = 0; i_id < items_count; ++i_id) {
			const auto item = provider.GetItem(i_id);

			const double collect_radius = item.width + gatherer.width;

			const CollectionResult res =
				 TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);

			if (res.IsCollected(collect_radius)) {
				result.push_back(GatheringEvent{/*item_id*/ i_id,
														  /*gatherer_id*/ g_id,
														  /*sq_distance*/ res.sq_distance,
														  /*time*/ res.proj_ratio});
			}
		}
	}

	std::sort(result.begin(), result.end(),
				 [](const GatheringEvent& lhs, const GatheringEvent& rhs) {
					 if (lhs.time != rhs.time) {
						 return lhs.time < rhs.time;
					 }
					 if (lhs.sq_distance != rhs.sq_distance) {
						 return lhs.sq_distance < rhs.sq_distance;
					 }
					 if (lhs.gatherer_id != rhs.gatherer_id) {
						 return lhs.gatherer_id < rhs.gatherer_id;
					 }
					 return lhs.item_id < rhs.item_id;
				 });

	return result;
}

} // namespace collision_detector
