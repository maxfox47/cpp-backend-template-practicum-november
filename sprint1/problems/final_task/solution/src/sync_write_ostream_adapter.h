#pragma once
#include "http_server.h"
namespace net = boost::asio;

namespace http_server {
    // Для синхронного вывода в Boost.Asio могут использовать любой тип, удовлетворяющий требованиям,
    // описанным в документе:
    // https://www.boost.org/doc/libs/1_78_0/doc/html/boost_asio/reference/SyncWriteStream.html
    class SyncWriteOStreamAdapter {
    public:
        explicit SyncWriteOStreamAdapter(std::ostream& os)
            : os_{os} {
        }

        template <typename ConstBufferSequence>
        size_t write_some(const ConstBufferSequence& cbs, sys::error_code& ec) {
            const size_t total_size = net::buffer_size(cbs);
            if (total_size == 0) {
                ec = {};
                return 0;
            }
            size_t bytes_written = 0;
            for (const auto& cb : cbs) {
                const size_t size = cb.size();
                const char* const data = reinterpret_cast<const char*>(cb.data());
                if (size > 0) {
                    if (!os_.write(reinterpret_cast<const char*>(data), size)) {
                        ec = make_error_code(boost::system::errc::io_error);
                        return bytes_written;
                    }
                    bytes_written += size;
                }
            }
            ec = {};
            return bytes_written;
        }

        template <typename ConstBufferSequence>
        size_t write_some(const ConstBufferSequence& cbs) {
            sys::error_code ec;
            const size_t bytes_written = write_some(cbs, ec);
            if (ec) {
                throw std::runtime_error("Failed to write");
            }
            return bytes_written;
        }

    private:
        std::ostream& os_;
    };
}