#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "../src/loot_generator.h"

using namespace std::literals;

/*
 * Тесты для генератора трофеев (LootGenerator).
 * Проверяют корректность работы алгоритма генерации потерянных предметов на карте.
 */
SCENARIO("Loot generation") {
    using loot_gen::LootGenerator;
    using TimeInterval = LootGenerator::TimeInterval;

	// Тест 1: Генератор с вероятностью 100% (всегда генерирует при нехватке)
	GIVEN("a loot generator with 100% probability") {
		// Создаем генератор: период 1 секунда, вероятность 1.0 (100%)
		LootGenerator generator{1s, 1.0};

        constexpr TimeInterval TIME_INTERVAL = 1s;

		// Тест 1.1: Когда трофеев достаточно для всех мародёров
        WHEN("loot count is enough for every looter") {
            THEN("no loot is generated") {
				// Проверяем различные комбинации количества трофеев и мародёров
				// Если трофеев >= мародёров, новых трофеев не должно генерироваться
				for (unsigned looter_count = 0; looter_count < 10; ++looter_count) {
					for (unsigned current_loot_count = looter_count; current_loot_count < looter_count + 10; ++current_loot_count) {
						INFO("loot count: " << current_loot_count << ", looters: " << looter_count);
						unsigned generated_count = generator.Generate(TIME_INTERVAL, current_loot_count, looter_count);
						REQUIRE(generated_count == 0);
                    }
                }
            }
        }

		// Тест 1.2: Когда мародёров больше, чем трофеев
        WHEN("number of looters exceeds loot count") {
            THEN("number of loot is proportional to loot difference") {
				// Генератор должен создать ровно столько трофеев, сколько не хватает
				// (количество мародёров - количество трофеев)
				for (unsigned current_loot_count = 0; current_loot_count < 10; ++current_loot_count) {
					for (unsigned looter_count = current_loot_count; looter_count < current_loot_count + 10; ++looter_count) {
						INFO("loot count: " << current_loot_count << ", looters: " << looter_count);
						unsigned expected_generated = looter_count - current_loot_count;
						unsigned generated_count = generator.Generate(TIME_INTERVAL, current_loot_count, looter_count);
						REQUIRE(generated_count == expected_generated);
                    }
                }
            }
        }
    }

	// Тест 2: Генератор с вероятностью 50%
	GIVEN("a loot generator with 50% probability") {
        constexpr TimeInterval BASE_INTERVAL = 1s;
		constexpr double PROBABILITY = 0.5; // 50% вероятность
		LootGenerator generator{BASE_INTERVAL, PROBABILITY};

		// Тест 2.1: Время больше базового интервала
        WHEN("time is greater than base interval") {
            THEN("number of generated loot is increased") {
				// При времени = 2 * базовый_интервал вероятность увеличивается
				// Ожидаем больше трофеев, чем при базовом интервале
				unsigned generated_count = generator.Generate(BASE_INTERVAL * 2, 0, 4);
				CHECK(generated_count == 3);
            }
        }

		// Тест 2.2: Время меньше базового интервала
        WHEN("time is less than base interval") {
            THEN("number of generated loot is decreased") {
				// Вычисляем интервал времени, при котором вероятность будет 25%
				// Формула: time = 1.0 / (log(1 - 0.5) / log(1.0 - 0.25))
				const auto reduced_time_interval = std::chrono::duration_cast<TimeInterval>(
					 std::chrono::duration<double>{1.0 / (std::log(1 - PROBABILITY) / std::log(1.0 - 0.25))});
				unsigned generated_count = generator.Generate(reduced_time_interval, 0, 4);
				CHECK(generated_count == 1);
            }
        }
    }

	// Тест 3: Генератор с кастомным генератором случайных чисел
    GIVEN("a loot generator with custom random generator") {
		constexpr TimeInterval BASE_INTERVAL = 1s;
		constexpr double PROBABILITY = 0.5;
		// Используем фиксированный генератор, возвращающий всегда 0.5
		LootGenerator generator{BASE_INTERVAL, PROBABILITY, [] { return 0.5; }};
		
        WHEN("loot is generated") {
            THEN("number of loot is proportional to random generated values") {
				// Вычисляем интервал времени для вероятности 25%
				const auto reduced_time_interval = std::chrono::duration_cast<TimeInterval>(
					 std::chrono::duration<double>{1.0 / (std::log(1 - PROBABILITY) / std::log(1.0 - 0.25))});
                
				// Первый вызов: с учетом random_generator() = 0.5, должно быть 0 трофеев
				unsigned first_generation = generator.Generate(reduced_time_interval, 0, 4);
				CHECK(first_generation == 0);
                
				// Второй вызов: накопленное время увеличивается, должно быть 1 трофей
				unsigned second_generation = generator.Generate(reduced_time_interval, 0, 4);
				CHECK(second_generation == 1);
            }
        }
    }
}
