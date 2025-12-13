#ifdef WIN32
#include <sdkddkver.h>
#endif
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <optional>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

// Функция для обработки HTTP-запросов
template<class Body, class Allocator>
http::response<http::string_body> HandleRequest(http::request<Body, http::basic_fields<Allocator>>&& req) {
    http::response<http::string_body> res;
    
    // Проверяем метод запроса
    if (req.method() == http::verb::get || req.method() == http::verb::head) {
        // Извлекаем target из URL (убираем ведущий символ /)
        std::string target(req.target());
        if (!target.empty() && target[0] == '/') {
            target = target.substr(1);
        }
        
        // Формируем тело ответа
        std::string body = "Hello, " + target;
        
        // Устанавливаем статус и заголовки
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/html");
        res.set(http::field::content_length, std::to_string(body.length()));
        
        // Для GET запроса добавляем тело, для HEAD - нет
        if (req.method() == http::verb::get) {
            res.body() = body;
        }
    } else {
        // Неподдерживаемый метод
        std::string body = "Invalid method";
        
        res.result(http::status::method_not_allowed);
        res.set(http::field::content_type, "text/html");
        res.set(http::field::allow, "GET, HEAD");
        res.set(http::field::content_length, std::to_string(body.length()));
        res.body() = body;
    }
    
    res.prepare_payload();
    return res;
}

int main() {
    const auto address = net::ip::make_address("127.0.0.1");
    const unsigned short port = 8080;
    
    // Создаем io_context для работы с сетью
    net::io_context ioc;
    
    // Создаем acceptor для принятия соединений
    tcp::acceptor acceptor(ioc, {address, port});
    
    // Выводим сообщение о готовности сервера
    std::cout << "Server has started..."sv << std::endl;
    
    while (true) {
        // Принимаем соединение клиента
        tcp::socket socket(ioc);
        acceptor.accept(socket);
        
        // Обрабатываем запрос в отдельном потоке
        std::thread([socket = std::move(socket)]() mutable {
            try {
                // Буфер для чтения запроса
                beast::flat_buffer buffer;
                
                // Читаем HTTP-запрос
                http::request<http::string_body> req;
                http::read(socket, buffer, req);
                
                // Обрабатываем запрос
                auto res = HandleRequest(std::move(req));
                
                // Отправляем ответ
                http::write(socket, res);
                
                // Закрываем соединение
                socket.shutdown(tcp::socket::shutdown_both);
            } catch (std::exception const& e) {
                // В случае ошибки просто закрываем соединение
                socket.shutdown(tcp::socket::shutdown_both);
            }
        }).detach();
    }
}
