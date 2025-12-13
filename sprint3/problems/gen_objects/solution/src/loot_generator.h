#pragma once
#include <chrono>
#include <functional>

namespace loot_gen {

/*
 *  Генератор трофеев
 */
class LootGenerator {
 public:
	using RandomGenerator = std::function<double()>;
	using TimeInterval = std::chrono::milliseconds;

	/*
	 * Создает генератор трофеев.
	 * 
	 * @param base_interval - базовый отрезок времени > 0 (например, 5 секунд)
	 * @param probability - вероятность появления трофея в течение базового интервала (0.0 - 1.0)
	 * @param random_gen - генератор псевдослучайных чисел в диапазоне [0.0, 1.0]
	 *                    По умолчанию всегда возвращает 1.0
	 */
	LootGenerator(TimeInterval base_interval, double probability,
					  RandomGenerator random_gen = DefaultGenerator)
		 : base_interval_{base_interval}, probability_{probability},
			random_generator_{std::move(random_gen)} {}

	/*
	 * Вычисляет количество трофеев, которые должны появиться на карте.
	 * 
	 * Алгоритм гарантирует, что количество трофеев не превышает количество мародёров.
	 * Вероятность генерации увеличивается с накоплением времени без генерации.
	 * 
	 * @param time_delta - отрезок времени, прошедший с момента предыдущего вызова
	 * @param loot_count - текущее количество трофеев на карте
	 * @param looter_count - количество мародёров (собак) на карте
	 * @return количество новых трофеев для генерации
	 */
	unsigned Generate(TimeInterval time_delta, unsigned loot_count, unsigned looter_count);

 private:
	static double DefaultGenerator() noexcept { return 1.0; };
	TimeInterval base_interval_;
	double probability_;
	TimeInterval time_without_loot_{};
	RandomGenerator random_generator_;
};

} // namespace loot_gen
