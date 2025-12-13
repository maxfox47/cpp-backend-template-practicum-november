#define _USE_MATH_DEFINES

#include "../src/collision_detector.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <sstream>
#include <vector>

using Catch::Matchers::WithinAbs;

namespace Catch {
template<>
struct StringMaker<collision_detector::GatheringEvent> {
    static std::string convert(collision_detector::GatheringEvent const& value) {
        std::ostringstream tmp;
        tmp << "(" << value.gatherer_id << "," << value.item_id << "," 
            << value.sq_distance << "," << value.time << ")";
        return tmp.str();
    }
};
}  // namespace Catch

namespace collision_detector {

// Тестовый провайдер для проверки функции FindGatherEvents
class TestProvider : public ItemGathererProvider {
public:
    TestProvider(std::vector<Item> items, std::vector<Gatherer> gatherers)
        : items_(std::move(items)), gatherers_(std::move(gatherers)) {}

    size_t ItemsCount() const override {
        return items_.size();
    }

    Item GetItem(size_t idx) const override {
        return items_[idx];
    }

    size_t GatherersCount() const override {
        return gatherers_.size();
    }

    Gatherer GetGatherer(size_t idx) const override {
        return gatherers_[idx];
    }

private:
    std::vector<Item> items_;
    std::vector<Gatherer> gatherers_;
};

// Функция для сравнения событий с учетом погрешности
bool EventsEqual(const GatheringEvent& a, const GatheringEvent& b, double epsilon = 1e-10) {
    return a.item_id == b.item_id &&
           a.gatherer_id == b.gatherer_id &&
           std::abs(a.sq_distance - b.sq_distance) < epsilon &&
           std::abs(a.time - b.time) < epsilon;
}

// Функция для сравнения векторов событий
bool EventsVectorsEqual(const std::vector<GatheringEvent>& a, 
                        const std::vector<GatheringEvent>& b,
                        double epsilon = 1e-10) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (!EventsEqual(a[i], b[i], epsilon)) {
            return false;
        }
    }
    return true;
}

}  // namespace collision_detector

TEST_CASE("Empty provider returns empty events") {
    collision_detector::TestProvider provider({}, {});
    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.empty());
}

TEST_CASE("No items returns empty events") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    collision_detector::TestProvider provider({}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.empty());
}

TEST_CASE("No gatherers returns empty events") {
    collision_detector::Item item{
        {5.0, 0.0},
        0.5
    };
    collision_detector::TestProvider provider({item}, {});
    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.empty());
}

TEST_CASE("Single gatherer collects single item on path") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    collision_detector::Item item{
        {5.0, 0.0},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].gatherer_id == 0);
    REQUIRE(events[0].item_id == 0);
    REQUIRE_THAT(events[0].time, WithinAbs(0.5, 1e-10));
    REQUIRE_THAT(events[0].sq_distance, WithinAbs(0.0, 1e-10));
}

TEST_CASE("Gatherer does not collect item far from path") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    collision_detector::Item item{
        {5.0, 10.0},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.empty());
}

TEST_CASE("Gatherer does not collect item before start") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    collision_detector::Item item{
        {-5.0, 0.0},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.empty());
}

TEST_CASE("Gatherer does not collect item after end") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    collision_detector::Item item{
        {15.0, 0.0},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.empty());
}

TEST_CASE("Gatherer collects item at start") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    collision_detector::Item item{
        {0.0, 0.0},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].gatherer_id == 0);
    REQUIRE(events[0].item_id == 0);
    REQUIRE_THAT(events[0].time, WithinAbs(0.0, 1e-10));
}

TEST_CASE("Gatherer collects item at end") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    collision_detector::Item item{
        {10.0, 0.0},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].gatherer_id == 0);
    REQUIRE(events[0].item_id == 0);
    REQUIRE_THAT(events[0].time, WithinAbs(1.0, 1e-10));
}

TEST_CASE("Gatherer collects item at side within radius") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    collision_detector::Item item{
        {5.0, 0.5},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].gatherer_id == 0);
    REQUIRE(events[0].item_id == 0);
    REQUIRE_THAT(events[0].time, WithinAbs(0.5, 1e-10));
    // Расстояние должно быть 0.5^2 = 0.25
    REQUIRE_THAT(events[0].sq_distance, WithinAbs(0.25, 1e-10));
}

TEST_CASE("Gatherer does not collect item at side beyond radius") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    collision_detector::Item item{
        {5.0, 1.2},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.empty());
}

TEST_CASE("Multiple gatherers collect multiple items") {
    collision_detector::Gatherer gatherer1{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    collision_detector::Gatherer gatherer2{
        {0.0, 5.0},
        {10.0, 5.0},
        0.6
    };
    collision_detector::Item item1{
        {5.0, 0.0},
        0.5
    };
    collision_detector::Item item2{
        {5.0, 5.0},
        0.5
    };
    collision_detector::TestProvider provider({item1, item2}, {gatherer1, gatherer2});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 2);
    // Проверяем, что события упорядочены по времени
    REQUIRE(events[0].time <= events[1].time);
}

TEST_CASE("Events are sorted chronologically") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    collision_detector::Item item1{
        {1.0, 0.0},
        0.5
    };
    collision_detector::Item item2{
        {5.0, 0.0},
        0.5
    };
    collision_detector::Item item3{
        {9.0, 0.0},
        0.5
    };
    collision_detector::TestProvider provider({item1, item2, item3}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 3);
    REQUIRE_THAT(events[0].time, WithinAbs(0.1, 1e-10));
    REQUIRE_THAT(events[1].time, WithinAbs(0.5, 1e-10));
    REQUIRE_THAT(events[2].time, WithinAbs(0.9, 1e-10));
    REQUIRE(events[0].item_id == 0);
    REQUIRE(events[1].item_id == 1);
    REQUIRE(events[2].item_id == 2);
}

TEST_CASE("Gatherer collects item on diagonal path") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 10.0},
        0.6
    };
    collision_detector::Item item{
        {5.0, 5.0},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].gatherer_id == 0);
    REQUIRE(events[0].item_id == 0);
    REQUIRE_THAT(events[0].time, WithinAbs(0.5, 1e-10));
}

TEST_CASE("Gatherer with zero movement does not collect") {
    collision_detector::Gatherer gatherer{
        {5.0, 5.0},
        {5.0, 5.0},
        0.6
    };
    collision_detector::Item item{
        {5.0, 5.0},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.empty());
}

TEST_CASE("Gatherer collects item with large radius") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 0.0},
        2.0
    };
    collision_detector::Item item{
        {5.0, 2.0},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].gatherer_id == 0);
    REQUIRE(events[0].item_id == 0);
}

TEST_CASE("Multiple gatherers can collect same item") {
    collision_detector::Gatherer gatherer1{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    collision_detector::Gatherer gatherer2{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    collision_detector::Item item{
        {5.0, 0.0},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer1, gatherer2});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 2);
    REQUIRE(events[0].item_id == 0);
    REQUIRE(events[1].item_id == 0);
    REQUIRE((events[0].gatherer_id == 0 || events[0].gatherer_id == 1));
    REQUIRE((events[1].gatherer_id == 0 || events[1].gatherer_id == 1));
}

TEST_CASE("Gatherer collects item at boundary of radius") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    // Предмет на границе: расстояние 0.6 + 0.5 = 1.1
    collision_detector::Item item{
        {5.0, 1.1},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
    REQUIRE_THAT(events[0].sq_distance, WithinAbs(1.1 * 1.1, 1e-10));
}

TEST_CASE("Gatherer does not collect item just beyond radius") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    
    // Предмет чуть дальше границы
    collision_detector::Item item{
        {5.0, 1.1001},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.empty());
}

TEST_CASE("Vertical movement collects item") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {0.0, 10.0},
        0.6
    };
    collision_detector::Item item{
        {0.0, 5.0},
        0.5
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
    REQUIRE_THAT(events[0].time, WithinAbs(0.5, 1e-10));
}

TEST_CASE("Item with zero width can be collected") {
    collision_detector::Gatherer gatherer{
        {0.0, 0.0},
        {10.0, 0.0},
        0.6
    };
    collision_detector::Item item{
        {5.0, 0.0},
        0.0
    };
    collision_detector::TestProvider provider({item}, {gatherer});
    auto events = collision_detector::FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
}
