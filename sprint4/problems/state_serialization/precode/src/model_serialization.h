#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/split_free.hpp>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "model.h"

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, FoundObject& obj, [[maybe_unused]] const unsigned version) {
    ar&(*obj.id);
    ar&(obj.type);
}

// Сериализация для enum Direction
template <typename Archive>
void serialize(Archive& ar, Direction& dir, [[maybe_unused]] const unsigned version) {
    int dir_value = static_cast<int>(dir);
    ar& dir_value;
    if (dir_value >= 0 && dir_value <= 3) {
        dir = static_cast<Direction>(dir_value);
    } else {
        dir = Direction::NORTH;  // Значение по умолчанию при ошибке
    }
}

}  // namespace model

namespace serialization {

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(dog.GetId())
        , name_(dog.GetName())
        , pos_(dog.GetPosition())
        , bag_capacity_(dog.GetBagCapacity())
        , speed_(dog.GetSpeed())
        , direction_(dog.GetDirection())
        , score_(dog.GetScore())
        , bag_content_(dog.GetBagContent()) {
    }

    [[nodiscard]] model::Dog Restore() const {
        model::Dog dog{id_, name_, pos_, bag_capacity_};
        dog.SetSpeed(speed_);
        dog.SetDirection(direction_);
        dog.AddScore(score_);
        for (const auto& item : bag_content_) {
            if (!dog.PutToBag(item)) {
                throw std::runtime_error("Failed to put bag content");
            }
        }
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar& name_;
        ar& pos_;
        ar& bag_capacity_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
    }

private:
    model::Dog::Id id_ = model::Dog::Id{0u};
    std::string name_;
    geom::Point2D pos_;
    size_t bag_capacity_ = 0;
    geom::Vec2D speed_;
    model::Direction direction_ = model::Direction::NORTH;
    model::Score score_ = 0;
    model::Dog::BagContent bag_content_;
};

// Представление потерянного предмета для сериализации
class LostObjectRepr {
public:
    LostObjectRepr() = default;

    explicit LostObjectRepr(const model::FoundObject& obj)
        : id_(obj.id)
        , type_(obj.type) {
    }

    [[nodiscard]] model::FoundObject Restore() const {
        return model::FoundObject{id_, type_};
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar& type_;
    }

private:
    model::FoundObject::Id id_ = model::FoundObject::Id{0u};
    model::LostObjectType type_ = 0u;
};

// Состояние карты для сериализации
struct MapState {
    std::string map_id;
    std::vector<DogRepr> dogs;
    std::vector<LostObjectRepr> lost_objects;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& map_id;
        ar& dogs;
        ar& lost_objects;
    }
};

// Информация о пользователе для сериализации
struct UserInfo {
    std::string token;
    uint32_t user_id;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& token;
        ar& user_id;
    }
};

// Полное состояние игры для сериализации
class GameStateRepr {
public:
    GameStateRepr() = default;

    std::vector<MapState> maps;
    std::vector<UserInfo> users;  // token -> user_id mapping

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& maps;
        ar& users;
    }
};

}  // namespace serialization
