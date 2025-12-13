#pragma once

#include <chrono>

namespace app {

using GameTime = std::chrono::milliseconds;

/**
 * Интерфейс для слушателей событий приложения.
 * Используется для реализации паттерна Observer.
 */
class ApplicationListener {
public:
    virtual ~ApplicationListener() = default;

    /**
     * Вызывается при каждом шаге игровых часов.
     * @param delta Время, прошедшее с предыдущего шага
     */
    virtual void OnTick(GameTime delta) = 0;
};

}  // namespace app

