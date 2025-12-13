#pragma once

#include "application_listener.h"
#include <chrono>
#include <memory>
#include <vector>

namespace app {

/**
 * Базовый интерфейс для приложения, которое может уведомлять слушателей о тиках.
 * Реализация должна вызывать OnTick у всех зарегистрированных слушателей.
 */
class Application {
public:
    virtual ~Application() = default;

    /**
     * Выполняет шаг игровых часов.
     * @param delta Время, прошедшее с предыдущего шага
     */
    virtual void Tick(GameTime delta) = 0;

    /**
     * Добавляет слушателя событий приложения.
     * @param listener Указатель на слушателя (не владеет объектом)
     */
    void AddListener(ApplicationListener* listener) {
        listeners_.push_back(listener);
    }

protected:
    /**
     * Уведомляет всех слушателей о тике.
     * Должен вызываться из реализации метода Tick.
     */
    void NotifyListeners(GameTime delta) {
        for (auto* listener : listeners_) {
            listener->OnTick(delta);
        }
    }

private:
    std::vector<ApplicationListener*> listeners_;
};

}  // namespace app

