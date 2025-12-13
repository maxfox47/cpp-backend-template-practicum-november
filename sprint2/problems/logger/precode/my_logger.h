#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    auto GetTime() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (manual_ts_) {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&t_c), "%F %T");
        return ss.str();
    }

    void OpenLogFile(const std::string& date_str) {
        if (log_file_.is_open()) {
            log_file_.close();
        }
        
        std::string filename = "/var/log/sample_log_" + date_str + ".log";
        log_file_.open(filename, std::ios::app);
        last_date_ = date_str;
    }

    Logger() = default;
    Logger(const Logger&) = delete;
    
    ~Logger() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss_date;
        ss_date << std::put_time(std::localtime(&t_c), "%Y_%m_%d");
        std::string current_date_str = ss_date.str();
        
        // Если дата изменилась, открываем новый файл
        if (last_date_ != current_date_str) {
            OpenLogFile(current_date_str);
        }
        
        // Открываем файл если не открыт
        if (!log_file_.is_open()) {
            OpenLogFile(current_date_str);
        }
        
        // Выводим временную метку
        std::stringstream ss_time;
        ss_time << GetTimeStamp();
        log_file_ << ss_time.str() << ": ";
        
        // Выводим все аргументы с помощью fold expression
        (log_file_ << ... << args);
        
        log_file_ << std::endl;
        log_file_.flush();
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> lock(mutex_);
        manual_ts_ = ts;
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::ofstream log_file_;
    std::string last_date_;
    mutable std::mutex mutex_;
};
