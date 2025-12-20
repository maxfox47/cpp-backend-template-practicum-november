#pragma once

#include <filesystem>
#include <string>

#include "model.h"
#include <boost/json.hpp>

namespace json_loader {

std::string LoadJsonFile(const std::filesystem::path& json_path);
model::Game LoadGame(const std::filesystem::path& json_path);

} // namespace json_loader
