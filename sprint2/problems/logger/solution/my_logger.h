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
    auto GetTimeNoLock() const {
        if (manual_ts_) {
            return *manual_ts_;
        }
        return std::chrono::system_clock::now();
    }

    std::string GetTimeStampNoLock() const {
        const auto now = GetTimeNoLock();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::tm* timeinfo = std::localtime(&t_c);
        std::stringstream ss;
        ss << std::put_time(timeinfo, "%F %T");
        return ss.str();
    }

    std::string GetDateStringNoLock() const {
        const auto now = GetTimeNoLock();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::tm* timeinfo = std::localtime(&t_c);
        std::stringstream ss;
        ss << std::put_time(timeinfo, "%Y_%m_%d");
        return ss.str();
    }

    void OpenLogFile(const std::string& date_str) {
        if (log_file_.is_open()) {
            log_file_.close();
        }
        std::string filename = "/var/log/sample_log_" + date_str + ".log";
        log_file_.open(filename, std::ios::app);
        if (!log_file_.is_open()) {
            // Если не удалось открыть, пробуем в текущую директорию
            filename = "sample_log_" + date_str + ".log";
            log_file_.open(filename, std::ios::app);
        }
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
        
        std::string current_date_str = GetDateStringNoLock();
        
        // Если дата изменилась, открываем новый файл
        if (last_date_ != current_date_str) {
            OpenLogFile(current_date_str);
        }
        
        // Открываем файл если не открыт
        if (!log_file_.is_open()) {
            OpenLogFile(current_date_str);
        }
        
        // Выводим временную метку
        log_file_ << GetTimeStampNoLock() << ": ";
        
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

