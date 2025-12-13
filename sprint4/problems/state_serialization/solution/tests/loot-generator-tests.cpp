#include "../src/loot_generator.h"
#include <catch2/catch_test_macros.hpp>

using namespace loot_gen;
using namespace std::chrono;

SCENARIO("Loot generator") {
	GIVEN("A loot generator with period 1s and probability 0.5") {
		LootGenerator gen{milliseconds{1000}, 0.5};

		WHEN("time delta is 0.5s, loot count is 0, looter count is 1") {
			THEN("generated loot count is 0") {
				CHECK(gen.Generate(milliseconds{500}, 0, 1) == 0);
			}
		}

		WHEN("time delta is 1s, loot count is 0, looter count is 1") {
			THEN("generated loot count is 1") {
				CHECK(gen.Generate(milliseconds{1000}, 0, 1) == 1);
			}
		}

		WHEN("time delta is 2s, loot count is 0, looter count is 1") {
			THEN("generated loot count is 1") {
				CHECK(gen.Generate(milliseconds{2000}, 0, 1) == 1);
			}
		}

		WHEN("time delta is 1s, loot count is 1, looter count is 1") {
			THEN("generated loot count is 0") {
				CHECK(gen.Generate(milliseconds{1000}, 1, 1) == 0);
			}
		}

		WHEN("time delta is 1s, loot count is 0, looter count is 2") {
			THEN("generated loot count is 2") {
				CHECK(gen.Generate(milliseconds{1000}, 0, 2) == 2);
			}
		}

		WHEN("time delta is 1s, loot count is 1, looter count is 2") {
			THEN("generated loot count is 1") {
				CHECK(gen.Generate(milliseconds{1000}, 1, 2) == 1);
			}
		}
	}
}

