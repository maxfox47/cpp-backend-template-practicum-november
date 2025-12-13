#include "../src/collision_detector.h"
#include <catch2/catch_test_macros.hpp>

using namespace collision_detector;

SCENARIO("Collision detector") {
	GIVEN("A gatherer moving from (0,0) to (10,0) with width 0.5") {
		Gatherer gatherer{{0, 0}, {10, 0}, 0.5};

		WHEN("item is at (5, 0) with width 0.1") {
			Item item{{5, 0}, 0.1};

			THEN("item is collected") {
				CollectionResult res = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);
				CHECK(res.IsCollected(gatherer.width + item.width));
			}
		}

		WHEN("item is at (5, 1) with width 0.1") {
			Item item{{5, 1}, 0.1};

			THEN("item is not collected") {
				CollectionResult res = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);
				CHECK_FALSE(res.IsCollected(gatherer.width + item.width));
			}
		}

		WHEN("item is at (15, 0) with width 0.1") {
			Item item{{15, 0}, 0.1};

			THEN("item is not collected") {
				CollectionResult res = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);
				CHECK_FALSE(res.IsCollected(gatherer.width + item.width));
			}
		}
	}
}

