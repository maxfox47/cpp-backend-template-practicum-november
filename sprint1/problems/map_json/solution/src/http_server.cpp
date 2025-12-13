#include "http_server.h"
#include <boost/asio/dispatch.hpp>

using namespace std::literals;

namespace http_server {

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}

void SessionBase::Read() {
    request_ = {};
    stream_.expires_after(std::chrono::seconds(30));
    http::async_read(stream_, buffer_, request_,
                     beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnWrite(bool should_close, beast::error_code error_code, std::size_t bytes_transferred) {
    if (error_code) {
        return ReportError(error_code, "write"sv);
    }

    if (should_close) {
        return Close();
    }

    Read();
}

void SessionBase::OnRead(beast::error_code error_code, std::size_t bytes_transferred) {
    using namespace std::literals;
    if (error_code == http::error::end_of_stream) {
        return Close();
    }
    if (error_code) {
        return ReportError(error_code, "read"sv);
    }
    HandleRequest(std::move(request_));
}

void SessionBase::Run() {
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

}  // namespace http_server