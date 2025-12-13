#include "audio.h"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#include <boost/asio.hpp>

using namespace std::literals;

void StartServer(uint16_t port) {
    using boost::asio::ip::udp;

    boost::asio::io_context io;
    udp::endpoint listen_endpoint(udp::v4(), port);
    udp::socket socket(io);
    socket.open(udp::v4());
    socket.bind(listen_endpoint);

    Player player(ma_format_u8, 1);

    std::cout << "Server listening on port " << port << std::endl;

    // UDP максимальная полезная нагрузка ~65Кб
    std::vector<char> buffer(65536);
    while (true) {
        udp::endpoint remote_endpoint;
        boost::system::error_code ec;
        size_t bytes_received = socket.receive_from(boost::asio::buffer(buffer), remote_endpoint, 0, ec);
        if (ec) {
            std::cerr << "Receive error: " << ec.message() << std::endl;
            continue;
        }

        size_t frame_size = static_cast<size_t>(player.GetFrameSize());
        size_t frames = frame_size == 0 ? 0 : bytes_received / frame_size;
        if (frames == 0) {
            continue;
        }

        // Длительность воспроизведения из количества фреймов при 44100 Гц
        std::chrono::duration<double> play_dur(static_cast<double>(frames) / 44100.0);
        player.PlayBuffer(buffer.data(), frames, play_dur);
    }
}

void StartClient(uint16_t port) {
    using boost::asio::ip::udp;

    boost::asio::io_context io;
    udp::socket socket(io);
    socket.open(udp::v4());

    Recorder recorder(ma_format_u8, 1);

    while (true) {
        std::cout << "Введите IP-адрес сервера (пусто для выхода): " << std::flush;
        std::string ip;
        if (!std::getline(std::cin, ip)) {
            break;
        }
        if (ip.empty()) {
            break;
        }

        boost::system::error_code ec;
        auto addr = boost::asio::ip::make_address(ip, ec);
        if (ec) {
            std::cerr << "Неверный IP: " << ec.message() << std::endl;
            continue;
        }
        udp::endpoint server_endpoint(addr, port);

        std::cout << "Запись сообщения... Нажмите Enter, чтобы начать." << std::endl;
        std::string dummy;
        std::getline(std::cin, dummy);

        auto rec_result = recorder.Record(65000, 1.5s);

        size_t frame_size = static_cast<size_t>(recorder.GetFrameSize());
        size_t bytes_to_send = rec_result.frames * frame_size;
        if (bytes_to_send == 0) {
            std::cout << "Пустая запись, пропуск." << std::endl;
            continue;
        }

        size_t sent = socket.send_to(boost::asio::buffer(rec_result.data.data(), bytes_to_send), server_endpoint, 0, ec);
        if (ec) {
            std::cerr << "Ошибка отправки: " << ec.message() << std::endl;
            continue;
        }
        std::cout << "Отправлено байт: " << sent << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: radio <client|server> <port>" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

    try {
        if (mode == "server") {
            StartServer(port);
        } else if (mode == "client") {
            StartClient(port);
        } else {
            std::cerr << "Unknown mode: " << mode << std::endl;
            return 1;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 2;
    }

    return 0;
}
