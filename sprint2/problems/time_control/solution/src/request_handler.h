#pragma once

#include <boost/beast/http.hpp>
#include <boost/asio/strand.hpp>
#include <boost/json.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <sstream>
#include <algorithm>
#include <cstdio>

#include "logging.h"
#include "application/game.h"
#include "application/players.h"
#include "util/token.h"
#include "model/player.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace net = boost::asio;
using namespace std::literals;
namespace fs = std::filesystem;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

class ApiHandler;

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    
    RequestHandler(std::string config_path, std::string base_path, Strand api_strand,
                   application::Game& game, application::Players& players, util::PlayerTokens& token_generator)
        : config_path_{std::move(config_path)}
        , base_path_{std::move(base_path)}
        , api_strand_{api_strand}
        , game_{game}
        , players_{players}
        , token_generator_{token_generator} {
        LoadConfig();
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();
        
        try {
            std::string target_str = std::string(req.target());
            
            // Проверяем, является ли запрос API-запросом
            if (target_str.starts_with("/api/")) {
                auto handle = [self = shared_from_this(), send,
                               req = std::forward<decltype(req)>(req), version, keep_alive]() mutable {
                    try {
                        return send(self->HandleApiRequest(req));
                    } catch (...) {
                        return send(self->ReportServerError(version, keep_alive));
                    }
                };
                return net::dispatch(api_strand_, handle);
            }
            
            // Обработка статических файлов
            return send(HandleFileRequest(req));
            
        } catch (...) {
            return send(ReportServerError(version, keep_alive));
        }
    }

private:
    void LoadConfig();
    bool IsSubPath(fs::path path) const;
    static std::string UrlDecode(std::string_view str);
    static std::string GetMimeType(const std::string& extension);

    StringResponse HandleApiRequest(const StringRequest& request) const;
    StringResponse HandleFileRequest(const StringRequest& req) const;
    StringResponse ReportServerError(unsigned version, bool keep_alive) const;
    
    std::optional<util::Token> TryExtractToken(const StringRequest& request) const;
    StringResponse MakeUnauthorizedError(unsigned version, bool keep_alive, const std::string& code, const std::string& message) const;
    
    template <typename Fn>
    StringResponse ExecuteAuthorized(const StringRequest& request, Fn&& action) const {
        auto version = request.version();
        auto keep_alive = request.keep_alive();
        
        auto token_opt = TryExtractToken(request);
        if (!token_opt) {
            return MakeUnauthorizedError(version, keep_alive, "invalidToken", "Authorization header is required");
        }
        
        auto player = players_.FindByToken(*token_opt);
        if (!player) {
            return MakeUnauthorizedError(version, keep_alive, "unknownToken", "Player token has not been found");
        }
        
        return action(*token_opt, *player);
    }

private:
    std::string config_path_;
    std::string base_path_;
    Strand api_strand_;
    application::Game& game_;
    application::Players& players_;
    util::PlayerTokens& token_generator_;
    json::value config_data_;
};

} // namespace http_handler

