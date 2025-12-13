#pragma once

#include "api_handler.h"
#include "http_server.h"
#include "logger.h"
#include "model.h"

#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace json = boost::json;

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace net = boost::asio;

std::string ToLower(std::string str);
std::string GetMimeType(const std::filesystem::path& ext);
std::string DecodeUrl(std::string_view str);
bool IsSubPath(fs::path path, fs::path base);

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
 public:
	using Strand = net::strand<net::io_context::executor_type>;

	explicit RequestHandler(model::Game& game, fs::path static_files, Strand strand, bool randomize,
									bool auto_tick)
		 : api_handler_(game, randomize, auto_tick), static_files_(static_files),
			api_strand_(strand) {}

	RequestHandler(const RequestHandler&) = delete;
	RequestHandler& operator=(const RequestHandler&) = delete;

	template <typename Body, typename Allocator, typename Send>
	void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
		try {
			EndPoint endpoint(std::string(req.target()));
			if (endpoint.IsApiReq()) {
				auto handle = [&endpoint, self = shared_from_this(), send,
									req = std::forward<decltype(req)>(req)] {
					try {
						assert(self->api_strand_.running_in_this_thread());
						return self->api_handler_(endpoint, req, std::move(send));
					} catch (const std::exception& e) {
						return send(ServerError(req,
														json::serialize(json::object{{"code", "internalError"},
																							  {"message", e.what()}}),
														"application/json"));
					}
				};
				return net::dispatch(api_strand_, handle);
			}

			if (req.method() != http::verb::get) {
				return send(
					 BadRequest(req,
									json::serialize(json::object{{"code", "badRequest"},
																		  {"message", "Unsupported http method"}}),
									"application/json"));
			}

			return GetStaticFiles(endpoint, req, std::move(send));
		} catch (const std::exception& e) {
			return send(ServerError(
				 req, json::serialize(json::object{{"code", "internalError"}, {"message", e.what()}}),
				 "application/json"));
		}
	}

 private:
	template <typename Body, typename Allocator, typename Send>
	void GetStaticFiles(const EndPoint& endpoint,
							  const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
		std::string path_str = DecodeUrl(endpoint.GetEndPoint());

		if (path_str.empty() || path_str.back() == '/') {
			path_str += "index.html";
		}

		if (path_str.front() == '/') {
			path_str = path_str.substr(1);
		}

		fs::path path = static_files_ / path_str;

		if (!IsSubPath(path, static_files_)) {
			return send(
				 BadRequest(req, "Attempt to get a file outside the root directory", "text/plain"));
		}

		if (!fs::exists(path) || !fs::is_regular_file(path)) {
			return send(NotFound(req, "File not found", "text/plain"));
		}

		auto mime_type = GetMimeType(path.extension());

		std::ifstream file(path, std::ios::binary);
		if (!file) {
			return send(NotFound(req, "File not found", "text/plain"));
		}

		std::string content;
		file.seekg(0, std::ios::end);
		content.resize(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(&content[0], content.size());

		http::response<http::string_body> res{http::status::ok, req.version()};
		res.set(http::field::content_type, mime_type);
		res.set(http::field::content_length, std::to_string(content.size()));
		res.keep_alive(req.keep_alive());
		if (req.method() == http::verb::get) {
			res.body() = std::move(content);
		}
		res.prepare_payload();

		return send(std::move(res));
	}

	ApiHandler api_handler_;
	fs::path static_files_;
	Strand api_strand_;
};

class LoggingRequestHandler {
 public:
	LoggingRequestHandler(RequestHandler& req) : decorated_(req) {}

	template <typename Body, typename Allocator, typename Send>
	void operator()(boost::beast::http::request<Body, http::basic_fields<Allocator>>&& req,
						 const std::string& client_ip, Send&& send) {
		LogRequest(req, client_ip);

		auto start_time = std::chrono::steady_clock::now();

		auto send_wrapper = [send = std::forward<Send>(send), client_ip, start_time,
									this](auto&& response) {
			LogResponse(response, client_ip, start_time);
			send(std::forward<decltype(response)>(response));
		};

		decorated_(std::forward<decltype(req)>(req), send_wrapper);
	}

 private:
	template <typename Response>
	static void LogResponse(Response&& response, const std::string& client_ip,
									std::chrono::steady_clock::time_point start_time) {
		auto end_time = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

		std::string content_type_str = "unknown";
		if (response.find(http::field::content_type) != response.end()) {
			content_type_str = std::string(response[http::field::content_type]);
		}

		BOOST_LOG_TRIVIAL(info) << boost::log::add_value(ip_address, client_ip)
										<< boost::log::add_value(response_time,
																		 static_cast<int>(duration.count()))
										<< boost::log::add_value(status_code,
																		 static_cast<int>(response.result()))
										<< boost::log::add_value(content_type, content_type_str)
										<< "response sent";
	}

	template <typename Body, typename Allocator>
	static void LogRequest(http::request<Body, http::basic_fields<Allocator>> req,
								  std::string client_ip) {
		BOOST_LOG_TRIVIAL(info) << boost::log::add_value(ip_address, client_ip)
										<< boost::log::add_value(uri, std::string(req.target()))
										<< boost::log::add_value(http_method,
																		 std::string(req.method_string()))
										<< "request received";
	}

	RequestHandler& decorated_;
};

} // namespace http_handler
