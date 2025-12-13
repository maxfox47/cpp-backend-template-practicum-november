#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <boost/json.hpp>
#include "model/dog.h"
#include "model/player.h"

namespace model {

class Map {
public:
    explicit Map(boost::json::value json_data) : json_data_(std::move(json_data)) {}
    
    std::string GetId() const {
        return std::string(json_data_.as_object().at("id").as_string());
    }
    
    bool HasId(const std::string& id) const {
        if (!json_data_.as_object().contains("id")) return false;
        return std::string(json_data_.as_object().at("id").as_string()) == id;
    }
    
    const boost::json::value& GetJsonData() const { return json_data_; }

private:
    boost::json::value json_data_;
};

} // namespace model





