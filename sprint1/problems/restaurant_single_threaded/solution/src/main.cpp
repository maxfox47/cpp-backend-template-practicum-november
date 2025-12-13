#ifdef WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <syncstream>
#include <unordered_map>

namespace net = boost::asio;
namespace sys = boost::system;
namespace ph = std::placeholders;
using namespace std::chrono;
using namespace std::literals;
using Timer = net::steady_timer;

class Hamburger {
public:
    [[nodiscard]] bool IsCutletRoasted() const {
        return cutlet_roasted_;
    }
    void SetCutletRoasted() {
        if (IsCutletRoasted()) {  // Котлету можно жарить только один раз
            throw std::logic_error("Cutlet has been roasted already"s);
        }
        cutlet_roasted_ = true;
    }

    [[nodiscard]] bool HasOnion() const {
        return has_onion_;
    }
    // Добавляем лук
    void AddOnion() {
        if (IsPacked()) {  // Если гамбургер упакован, класть лук в него нельзя
            throw std::logic_error("Hamburger has been packed already"s);
        }
        AssureCutletRoasted();  // Лук разрешается класть лишь после прожаривания котлеты
        has_onion_ = true;
    }

    [[nodiscard]] bool IsPacked() const {
        return is_packed_;
    }
    void Pack() {
        AssureCutletRoasted();  // Нельзя упаковывать гамбургер, если котлета не прожарена
        is_packed_ = true;
    }

private:
    // Убеждаемся, что котлета прожарена
    void AssureCutletRoasted() const {
        if (!cutlet_roasted_) {
            throw std::logic_error("Bread has not been roasted yet"s);
        }
    }

    bool cutlet_roasted_ = false;  // Обжарена ли котлета?
    bool has_onion_ = false;       // Есть ли лук?
    bool is_packed_ = false;       // Упакован ли гамбургер?
};

std::ostream& operator<<(std::ostream& os, const Hamburger& h) {
    return os << "Hamburger: "sv << (h.IsCutletRoasted() ? "roasted cutlet"sv : " raw cutlet"sv)
              << (h.HasOnion() ? ", onion"sv : ""sv)
              << (h.IsPacked() ? ", packed"sv : ", not packed"sv);
}

class Logger {
public:
    explicit Logger(std::string id)
        : id_(std::move(id)) {
    }

    void LogMessage(std::string_view message) const {
        std::osyncstream os{std::cout};
        os << id_ << "> ["sv << duration<double>(steady_clock::now() - start_time_).count()
           << "s] "sv << message << std::endl;
    }

private:
    std::string id_;
    steady_clock::time_point start_time_{steady_clock::now()};
};

// Функция, которая будет вызвана по окончании обработки заказа
using OrderHandler = std::function<void(sys::error_code ec, int id, Hamburger* hamburger)>;

class Order {
public:
    Order(int id, bool with_onion, OrderHandler handler, net::io_context& io)
        : id_(id), with_onion_(with_onion), handler_(std::move(handler)), 
          io_(io), timer_(io), logger_("Order " + std::to_string(id)) {
    }

    void Start() {
        logger_.LogMessage("Order started");
        // Начинаем приготовление гамбургера
        StartCooking();
    }

private:
    void StartCooking() {
        // Жарим котлету (1 секунда)
        timer_.expires_after(1s);
        timer_.async_wait([this](sys::error_code ec) {
            if (ec) {
                handler_(ec, id_, nullptr);
                return;
            }
            logger_.LogMessage("Cutlet roasted");
            hamburger_.SetCutletRoasted();
            
            if (with_onion_) {
                // Добавляем лук
                hamburger_.AddOnion();
                logger_.LogMessage("Onion added");
            }
            
            // Упаковываем гамбургер (0.5 секунды)
            PackHamburger();
        });
    }

    void PackHamburger() {
        timer_.expires_after(500ms);
        timer_.async_wait([this](sys::error_code ec) {
            if (ec) {
                handler_(ec, id_, nullptr);
                return;
            }
            hamburger_.Pack();
            logger_.LogMessage("Hamburger packed");
            handler_(sys::error_code{}, id_, &hamburger_);
        });
    }

    int id_;
    bool with_onion_;
    OrderHandler handler_;
    net::io_context& io_;
    Timer timer_;
    Hamburger hamburger_;
    Logger logger_;
};

class Restaurant {
public:
    explicit Restaurant(net::io_context& io)
        : io_(io) {
    }

    int MakeHamburger(bool with_onion, OrderHandler handler) {
        const int order_id = ++next_order_id_;
        auto order = std::make_unique<Order>(order_id, with_onion, std::move(handler), io_);
        order->Start();
        // Сохраняем заказ, чтобы он не был уничтожен до завершения
        orders_[order_id] = std::move(order);
        return order_id;
    }

private:
    net::io_context& io_;
    int next_order_id_ = 0;
    std::unordered_map<int, std::unique_ptr<Order>> orders_;
};

int main() {
    net::io_context io;

    Restaurant restaurant{io};

    Logger logger{"main"s};

    struct OrderResult {
        sys::error_code ec;
        Hamburger hamburger;
    };

    std::unordered_map<int, OrderResult> orders;
    auto handle_result = [&orders](sys::error_code ec, int id, Hamburger* h) {
        orders.emplace(id, OrderResult{ec, ec ? Hamburger{} : *h});
    };

    const int id1 = restaurant.MakeHamburger(false, handle_result);
    const int id2 = restaurant.MakeHamburger(true, handle_result);

    // До вызова io.run() никакие заказы не выполняются
    assert(orders.empty());
    io.run();

    // После вызова io.run() все заказы быть выполнены
    assert(orders.size() == 2u);
    {
        // Проверяем заказ без лука
        const auto& o = orders.at(id1);
        assert(!o.ec);
        assert(o.hamburger.IsCutletRoasted());
        assert(o.hamburger.IsPacked());
        assert(!o.hamburger.HasOnion());
    }
    {
        // Проверяем заказ с луком
        const auto& o = orders.at(id2);
        assert(!o.ec);
        assert(o.hamburger.IsCutletRoasted());
        assert(o.hamburger.IsPacked());
        assert(o.hamburger.HasOnion());
    }
}
