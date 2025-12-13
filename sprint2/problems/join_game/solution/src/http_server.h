#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <chrono>

#include "logging.h"

namespace http_server {

namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
namespace json = boost::json;

inline void ReportError(beast::error_code ec, std::string_view where) {
    json::object data{{"code", ec.value()}, {"text", ec.message()}, {"where", std::string(where)}};
    BOOST_LOG_TRIVIAL(error) << logging::add_value(additional_data, json::value(data)) << "error";
}

class SessionBase {
public:
    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;
    void Run();
    virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;
protected:
    using HttpRequest = beast::http::request<beast::http::string_body>;
    explicit SessionBase(tcp::socket&& socket)
        : stream_(std::move(socket)) {}

    template <typename Body, typename Fields>
    void Write(beast::http::response<Body, Fields>&& response) {
        auto safe_response = std::make_shared<beast::http::response<Body, Fields>>(std::move(response));
        auto self = GetSharedThis();
        beast::http::async_write(stream_, *safe_response,
            [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                self->OnWrite(safe_response->need_eof(), ec, bytes_written);
            });
    }

    virtual ~SessionBase() = default;
    virtual void HandleRequest(HttpRequest&& request) = 0;

private:
    void Read();
    void OnRead(beast::error_code ec, std::size_t bytes_read);
    void Close();
    void OnWrite(bool close, beast::error_code ec, std::size_t bytes_written);

private:
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;
protected:
    std::string GetRemoteIp() const {
        beast::error_code ec;
        auto ep = stream_.socket().remote_endpoint(ec);
        if (ec) return {};
        return ep.address().to_string();
    }
};

template <typename RequestHandler>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
public:
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& request_handler)
        : SessionBase(std::move(socket))
        , request_handler_(std::forward<Handler>(request_handler)) {}

protected:
    void HandleRequest(HttpRequest&& request) override {
        // log request
        std::string ip = GetRemoteIp();
        json::object req_data{{"ip", ip},
                              {"URI", std::string(request.target())},
                              {"method", std::string(request.method_string())}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, json::value(req_data))
                                << "request received";

        auto start = std::chrono::steady_clock::now();

        request_handler_(std::move(request), [self = this->shared_from_this(), start, ip](auto&& response) mutable {
            int code = static_cast<int>(response.result_int());
            std::string content_type;
            auto it = response.find(http::field::content_type);
            if (it != response.end()) {
                content_type = std::string(it->value());
            }
            auto end = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            json::object resp_data{{"ip", ip},
                                   {"response_time", static_cast<std::int64_t>(ms)},
                                   {"code", code}};
            if (content_type.empty()) {
                resp_data["content_type"] = nullptr;
            } else {
                resp_data["content_type"] = content_type;
            }
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, json::value(resp_data))
                                    << "response sent";

            self->Write(std::forward<decltype(response)>(response));
        });
    }

private:
    std::shared_ptr<SessionBase> GetSharedThis() override { return this->shared_from_this(); }
    RequestHandler request_handler_;
};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc))
        , request_handler_(std::forward<Handler>(request_handler)) {
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(net::socket_base::max_listen_connections);
    }
    void Run() { DoAccept(); }

private:
    void AsyncRunSession(tcp::socket&& socket) {
        std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();
    }
    void DoAccept() {
        acceptor_.async_accept(net::make_strand(ioc_),
            beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
    }
    void OnAccept(sys::error_code ec, tcp::socket socket) {
        if (ec) {
            return ReportError(ec, "accept");
        }
        AsyncRunSession(std::move(socket));
        DoAccept();
    }

private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler request_handler_;
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    using MyListener = Listener<std::decay_t<RequestHandler>>;
    std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
}

} // namespace http_server





