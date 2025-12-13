#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        try {
            // Получаем ингредиенты со склада
            auto bread = store_.GetBread();
            auto sausage = store_.GetSausage();
            
            // Создаем shared_ptr для отслеживания состояния приготовления
            struct CookingState {
                std::shared_ptr<Bread> bread;
                std::shared_ptr<Sausage> sausage;
                bool bread_ready = false;
                bool sausage_ready = false;
                HotDogHandler handler;
                int hotdog_id;
                
                CookingState(std::shared_ptr<Bread> b, std::shared_ptr<Sausage> s, HotDogHandler h, int id)
                    : bread(std::move(b)), sausage(std::move(s)), handler(std::move(h)), hotdog_id(id) {}
            };
            
            static int next_hotdog_id = 1;
            auto state = std::make_shared<CookingState>(bread, sausage, std::move(handler), next_hotdog_id++);
            
            // Функция для проверки готовности и сборки хот-дога
            auto check_ready = [state, this]() {
                if (state->bread_ready && state->sausage_ready) {
                    try {
                        // Собираем хот-дог
                        HotDog hotdog(state->hotdog_id, state->sausage, state->bread);
                        state->handler(Result<HotDog>(std::move(hotdog)));
                    } catch (const std::exception& e) {
                        state->handler(Result<HotDog>(std::make_exception_ptr(e)));
                    }
                }
            };
            
            // Начинаем приготовление хлеба
            bread->StartBake(*gas_cooker_, [state, check_ready, this]() {
                // Устанавливаем таймер на 1 секунду для хлеба
                auto timer = std::make_shared<net::steady_timer>(io_);
                timer->expires_after(std::chrono::milliseconds(1000));
                timer->async_wait([state, check_ready, timer](boost::system::error_code ec) {
                    if (!ec) {
                        state->bread->StopBaking();
                        state->bread_ready = true;
                        check_ready();
                    }
                });
            });
            
            // Начинаем приготовление сосиски
            sausage->StartFry(*gas_cooker_, [state, check_ready, this]() {
                // Устанавливаем таймер на 1.5 секунды для сосиски
                auto timer = std::make_shared<net::steady_timer>(io_);
                timer->expires_after(std::chrono::milliseconds(1500));
                timer->async_wait([state, check_ready, timer](boost::system::error_code ec) {
                    if (!ec) {
                        state->sausage->StopFry();
                        state->sausage_ready = true;
                        check_ready();
                    }
                });
            });
            
        } catch (const std::exception& e) {
            handler(Result<HotDog>(std::make_exception_ptr(e)));
        }
    }

private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};
