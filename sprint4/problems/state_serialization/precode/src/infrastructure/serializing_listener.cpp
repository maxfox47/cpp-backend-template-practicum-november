#include "serializing_listener.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

namespace infrastructure {

SerializingListener::SerializingListener(std::filesystem::path state_file_path, 
                                         app::GameTime save_period)
    : state_file_path_(std::move(state_file_path))
    , save_period_(save_period) {
}

void SerializingListener::OnTick(app::GameTime delta) {
    if (save_period_.count() == 0) {
        return;  // Автосохранение отключено
    }

    time_since_last_save_ += delta;

    if (time_since_last_save_ >= save_period_) {
        SaveState();
        time_since_last_save_ = app::GameTime{0};
    }
}

void SerializingListener::SaveState() {
    if (!state_getter_) {
        return;  // Функция получения состояния не установлена
    }

    try {
        // Получаем состояние приложения
        std::string state_data = state_getter_();

        // Создаем временный файл
        std::filesystem::path temp_file = state_file_path_;
        temp_file += ".tmp";

        // Записываем данные во временный файл
        {
            std::ofstream ofs(temp_file, std::ios::binary);
            if (!ofs) {
                throw std::runtime_error("Failed to open temporary file for writing");
            }
            ofs << state_data;
            if (!ofs) {
                throw std::runtime_error("Failed to write state to temporary file");
            }
        }

        // Атомарно переименовываем временный файл в целевой
        std::filesystem::rename(temp_file, state_file_path_);
    } catch (const std::exception& e) {
        std::cerr << "Error saving state: " << e.what() << std::endl;
        throw;
    }
}

bool SerializingListener::LoadState() {
    if (!std::filesystem::exists(state_file_path_)) {
        return false;  // Файл не существует, начинаем с чистого листа
    }

    try {
        // Читаем данные из файла
        std::ifstream ifs(state_file_path_, std::ios::binary);
        if (!ifs) {
            throw std::runtime_error("Failed to open state file for reading");
        }

        std::stringstream buffer;
        buffer << ifs.rdbuf();
        std::string state_data = buffer.str();

        if (!state_setter_) {
            throw std::runtime_error("State setter function is not set");
        }

        // Восстанавливаем состояние
        state_setter_(state_data);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading state: " << e.what() << std::endl;
        throw;
    }
}

void SerializingListener::SetStateGetter(std::function<std::string()> getter) {
    state_getter_ = std::move(getter);
}

void SerializingListener::SetStateSetter(std::function<void(const std::string&)> setter) {
    state_setter_ = std::move(setter);
}

}  // namespace infrastructure

