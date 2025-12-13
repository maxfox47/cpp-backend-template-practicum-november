#pragma once
#include "sdk.h"
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <optional>

namespace http_server {

namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

class SessionBase {
public:
    virtual ~SessionBase() = default;
    virtual void Run() = 0;
};

template <typename RequestHandler>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
public:
    Session(tcp::socket&& socket, const RequestHandler& handler)
        : socket_(std::move(socket))
        , handler_(handler) {
    }

    void Run() override {
        net::dispatch(socket_.get_executor(),
                     beast::bind_front_handler(&Session::DoRead, this->shared_from_this()));
    }

private:
    void DoRead() {
        req_ = {};
        http::async_read(socket_, buffer_, req_,
                        beast::bind_front_handler(&Session::OnRead, this->shared_from_this()));
    }

    void OnRead(beast::error_code ec, std::size_t bytes_read) {
        boost::ignore_unused(bytes_read);
        if (ec == http::error::end_of_stream) {
            socket_.shutdown(tcp::socket::shutdown_send);
            return;
        }
        if (ec) {
            return;
        }
        HandleRequest();
    }

    void HandleRequest() {
        handler_(std::move(req_), [self = this->shared_from_this()](auto&& response) {
            self->OnWrite(std::move(response));
        });
    }

    void OnWrite(http::response<http::string_body>&& response) {
        response_ = std::move(response);
        bool keep_alive = response_->keep_alive();
        http::async_write(socket_, *response_,
                         beast::bind_front_handler(&Session::OnWriteComplete, this->shared_from_this(), keep_alive));
    }

    void OnWriteComplete(bool keep_alive, beast::error_code ec, std::size_t bytes_written) {
        boost::ignore_unused(bytes_written);
        if (ec) {
            return;
        }
        if (!keep_alive) {
            socket_.shutdown(tcp::socket::shutdown_send);
            return;
        }
        response_.reset();
        DoRead();
    }

private:
    tcp::socket socket_;
    RequestHandler handler_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    std::optional<http::response<http::string_body>> response_;
};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, const RequestHandler& handler)
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc))
        , handler_(handler) {
        beast::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            return;
        }
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            return;
        }
        acceptor_.bind(endpoint, ec);
        if (ec) {
            return;
        }
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            return;
        }
    }

    void Run() {
        DoAccept();
    }

private:
    void DoAccept() {
        acceptor_.async_accept(net::make_strand(ioc_),
                              beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
    }

    void OnAccept(beast::error_code ec, tcp::socket socket) {
        if (ec) {
            return;
        }
        std::make_shared<Session<RequestHandler>>(std::move(socket), handler_)->Run();
        DoAccept();
    }

private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler handler_;
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, const RequestHandler& handler) {
    std::make_shared<Listener<RequestHandler>>(ioc, endpoint, handler)->Run();
}

}  // namespace http_server
