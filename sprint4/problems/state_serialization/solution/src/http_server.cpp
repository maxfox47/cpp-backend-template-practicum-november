#include "http_server.h"

#include <boost/asio/dispatch.hpp>

using namespace std::literals;

namespace http_server {

SessionBase::SessionBase(tcp::socket&& socket) : stream_(std::move(socket)) {}

std::string SessionBase::GetRemoteAddress() const {
	beast::error_code ec;
	auto endpoint = stream_.socket().remote_endpoint(ec);
	if (ec) {
		return "";
	}
	return endpoint.address().to_string();
}

void SessionBase::Run() {
	net::dispatch(stream_.get_executor(),
					  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
	using namespace std::literals;
	request_ = {};
	stream_.expires_after(30s);
	http::async_read(stream_, buffer_, request_,
						  beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
	if (ec == http::error::end_of_stream) {
		return Close();
	}
	if (ec) {
		return ReportError(ec, "read"s);
	}
	HandleRequest(std::move(request_));
}

void SessionBase::OnWrite(bool close, beast::error_code ec,
								  [[maybe_unused]] std::size_t bytes_written) {
	if (ec) {
		return ReportError(ec, "write"s);
	}

	if (close) {
		return Close();
	}

	Read();
}

void SessionBase::Close() {
	try {
		stream_.socket().shutdown(tcp::socket::shutdown_send);
	} catch (...) {
	}
}

} // namespace http_server

