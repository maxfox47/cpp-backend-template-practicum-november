#include "loot_generator.h"

#include <algorithm>
#include <cmath>

namespace loot_gen {

unsigned LootGenerator::Generate(TimeInterval time_delta, unsigned loot_count,
                                 unsigned looter_count) {
	// Накопляем время без генерации трофеев
    time_without_loot_ += time_delta;
	
	// Вычисляем нехватку трофеев: сколько мародёров не имеют трофеев
	// Если трофеев больше или равно мародёрам, нехватки нет
    const unsigned loot_shortage = loot_count > looter_count ? 0u : looter_count - loot_count;
	
	// Вычисляем отношение накопленного времени к базовому интервалу
	const double time_ratio = std::chrono::duration<double>{time_without_loot_} / base_interval_;
	
	// Вычисляем вероятность генерации с учетом накопленного времени
	// Формула: P = (1 - (1 - p)^ratio) * random_value
	// где p - базовая вероятность, ratio - отношение времени
	const double generation_probability =
		 std::clamp((1.0 - std::pow(1.0 - probability_, time_ratio)) * random_generator_(), 0.0, 1.0);
	
	// Вычисляем количество генерируемых трофеев
	// Округляем произведение нехватки на вероятность
	const unsigned generated_loot = static_cast<unsigned>(std::round(loot_shortage * generation_probability));

	// Если сгенерировали хотя бы один трофей, сбрасываем накопленное время
    if (generated_loot > 0) {
        time_without_loot_ = {};
    }
	
    return generated_loot;
}

} // namespace loot_gen
