#pragma once

#include "../app/application_listener.h"
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace infrastructure {

/**
 * Слушатель, который сохраняет состояние приложения в файл
 * с заданной периодичностью и при завершении работы.
 */
class SerializingListener : public app::ApplicationListener {
public:
    /**
     * @param state_file_path Путь к файлу для сохранения состояния
     * @param save_period Период автоматического сохранения (в миллисекундах игрового времени)
     *                    Если равен 0, автосохранение отключено
     */
    SerializingListener(std::filesystem::path state_file_path, 
                        app::GameTime save_period = app::GameTime{0});

    /**
     * Вызывается при каждом шаге игровых часов.
     * Сохраняет состояние, если прошло достаточно времени с последнего сохранения.
     */
    void OnTick(app::GameTime delta) override;

    /**
     * Сохраняет текущее состояние приложения в файл.
     * Использует атомарное переименование для безопасной записи.
     */
    void SaveState();

    /**
     * Восстанавливает состояние приложения из файла.
     * @return true, если состояние успешно восстановлено, false если файл не существует
     * @throws std::runtime_error если файл существует, но содержит некорректные данные
     */
    bool LoadState();

    /**
     * Устанавливает функцию для получения состояния приложения для сериализации.
     */
    void SetStateGetter(std::function<std::string()> getter);

    /**
     * Устанавливает функцию для восстановления состояния приложения из сериализованных данных.
     */
    void SetStateSetter(std::function<void(const std::string&)> setter);

private:
    std::filesystem::path state_file_path_;
    app::GameTime save_period_;
    app::GameTime time_since_last_save_{0};
    std::function<std::string()> state_getter_;
    std::function<void(const std::string&)> state_setter_;
};

}  // namespace infrastructure

