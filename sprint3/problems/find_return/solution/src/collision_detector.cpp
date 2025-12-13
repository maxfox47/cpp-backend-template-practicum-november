#include "collision_detector.h"
#include <cassert>
#include <cmath>

namespace collision_detector {

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
    // Проверим, что перемещение ненулевое.
    // Тут приходится использовать строгое равенство, а не приближённое,
    // пскольку при сборе заказов придётся учитывать перемещение даже на небольшое
    // расстояние.
    assert(b.x != a.x || b.y != a.y);
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
    std::vector<GatheringEvent> events;
    
    const size_t gatherers_count = provider.GatherersCount();
    const size_t items_count = provider.ItemsCount();
    
    for (size_t gatherer_idx = 0; gatherer_idx < gatherers_count; ++gatherer_idx) {
        const Gatherer gatherer = provider.GetGatherer(gatherer_idx);
        
        // Проверяем, что собиратель переместился
        if (gatherer.start_pos.x == gatherer.end_pos.x && 
            gatherer.start_pos.y == gatherer.end_pos.y) {
            continue;
        }
        
        const double collect_radius = gatherer.width;
        
        for (size_t item_idx = 0; item_idx < items_count; ++item_idx) {
            const Item item = provider.GetItem(item_idx);
            
            // Вычисляем результат попытки сбора
            const CollectionResult result = TryCollectPoint(
                gatherer.start_pos,
                gatherer.end_pos,
                item.position
            );
            
            // Проверяем, попадает ли предмет в радиус сбора
            const double total_radius = collect_radius + item.width;
            if (result.IsCollected(total_radius)) {
                GatheringEvent event;
                event.gatherer_id = gatherer_idx;
                event.item_id = item_idx;
                event.sq_distance = result.sq_distance;
                event.time = result.proj_ratio;
                events.push_back(event);
            }
        }
    }
    
    // Сортируем события по времени (хронологический порядок)
    // Для событий с одинаковым временем (с учетом погрешности) используем дополнительные критерии
    // для обеспечения однозначного порядка
    const double epsilon = 1e-10;
    std::sort(events.begin(), events.end(), 
        [epsilon](const GatheringEvent& a, const GatheringEvent& b) {
            if (std::abs(a.time - b.time) > epsilon) {
                return a.time < b.time;
            }
            if (a.gatherer_id != b.gatherer_id) {
                return a.gatherer_id < b.gatherer_id;
            }
            return a.item_id < b.item_id;
        });
    return events;
}

}  // namespace collision_detector