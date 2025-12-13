#include <algorithm>
#include <sstream>
#include <vector>

#define CATCH_CONFIG_MAIN

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "../src/collision_detector.h"

using Catch::Approx;
using namespace collision_detector;

constexpr double EPS = 1e-10;

namespace Catch {
template <>
struct StringMaker<collision_detector::GatheringEvent> {
	static std::string convert(collision_detector::GatheringEvent const& value) {
		std::ostringstream tmp;
		tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << ","
			 << value.time << ")";
		return tmp.str();
	}
};
} // namespace Catch

SCENARIO("FindGatherEvents detects single collision with correct data") {
	GIVEN("one moving gatherer and one item exactly on its path") {
		collision_detector::ItemGathererProvider provider;

		provider.AddGatherer(Gatherer{geom::Point2D{0.0, 0.0}, geom::Point2D{10.0, 0.0}, 0.0});
		provider.AddItem(Item{geom::Point2D{5.0, 0.0}, 0.0});

		WHEN("gather events are searched") {
			auto events = FindGatherEvents(provider);

			THEN("exactly one event is found and it has correct fields") {
				REQUIRE(events.size() == 1);

				const auto& ev = events[0];
				CHECK(ev.gatherer_id == 0);
				CHECK(ev.item_id == 0);
				CHECK(ev.time == Approx(0.5).margin(EPS));
				CHECK(ev.sq_distance == Approx(0.0).margin(EPS));
			}
		}
	}
}

SCENARIO("FindGatherEvents does not report collisions when item is far from path") {
	GIVEN("one moving gatherer and a distant item") {
		collision_detector::ItemGathererProvider provider;

		provider.AddGatherer(Gatherer{geom::Point2D{0.0, 0.0}, geom::Point2D{10.0, 0.0}, 0.1});
		provider.AddItem(Item{geom::Point2D{5.0, 10.0}, 0.1});

		WHEN("gather events are searched") {
			auto events = FindGatherEvents(provider);

			THEN("no events are produced") { CHECK(events.empty()); }
		}
	}
}

SCENARIO("FindGatherEvents returns all collisions in chronological order") {
	GIVEN("one gatherer passing through several items along its path") {
		collision_detector::ItemGathererProvider provider;

		provider.AddGatherer(Gatherer{geom::Point2D{0.0, 0.0}, geom::Point2D{4.0, 0.0}, 0.0});

		provider.AddItem(Item{geom::Point2D{-1.0, 0.0}, 0.0});
		provider.AddItem(Item{geom::Point2D{1.0, 0.0}, 0.0});
		provider.AddItem(Item{geom::Point2D{3.0, 0.0}, 0.0});

		WHEN("gather events are searched") {
			auto events = FindGatherEvents(provider);

			THEN("only items on the segment are collected and events are ordered by time") {
				REQUIRE(events.size() == 2);

				REQUIRE(std::is_sorted(events.begin(), events.end(),
											  [](const GatheringEvent& lhs, const GatheringEvent& rhs) {
												  return lhs.time < rhs.time ||
															std::abs(lhs.time - rhs.time) <= EPS;
											  }));

				CHECK(events[0].gatherer_id == 0);
				CHECK(events[0].item_id == 1);
				CHECK(events[0].time == Approx(0.25).margin(EPS));
				CHECK(events[0].sq_distance == Approx(0.0).margin(EPS));

				CHECK(events[1].gatherer_id == 0);
				CHECK(events[1].item_id == 2);
				CHECK(events[1].time == Approx(0.75).margin(EPS));
				CHECK(events[1].sq_distance == Approx(0.0).margin(EPS));
			}
		}
	}
}

SCENARIO("Stationary gatherer does not produce collision events") {
	GIVEN("a gatherer that did not move and an item at the same point") {
		collision_detector::ItemGathererProvider provider;

		provider.AddGatherer(Gatherer{geom::Point2D{1.0, 1.0}, geom::Point2D{1.0, 1.0}, 1.0});
		provider.AddItem(Item{geom::Point2D{1.0, 1.0}, 1.0});

		WHEN("gather events are searched") {
			auto events = FindGatherEvents(provider);

			THEN("no events are detected for zero-length movement") { CHECK(events.empty()); }
		}
	}
}
