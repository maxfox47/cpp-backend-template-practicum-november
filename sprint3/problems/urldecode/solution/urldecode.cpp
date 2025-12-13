#include "urldecode.h"

#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>

std::string UrlDecode(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '+') {
            // Пробел кодируется как +
            result += ' ';
        } else if (str[i] == '%') {
            // Проверяем, что есть минимум 2 символа после %
            if (i + 2 >= str.size()) {
                throw std::invalid_argument("Incomplete %-sequence");
            }
            
            // Извлекаем hex код
            std::string_view hex_code = str.substr(i + 1, 2);
            
            // Проверяем, что оба символа - hex цифры
            bool is_valid_hex = true;
            for (char c : hex_code) {
                if (!((c >= '0' && c <= '9') || 
                      (c >= 'A' && c <= 'F') || 
                      (c >= 'a' && c <= 'f'))) {
                    is_valid_hex = false;
                    break;
                }
            }
            
            if (!is_valid_hex) {
                throw std::invalid_argument("Invalid %-sequence");
            }
            
            // Преобразуем hex в число
            unsigned char value;
            auto [ptr, ec] = std::from_chars(hex_code.data(), hex_code.data() + 2, value, 16);
            
            if (ec != std::errc{}) {
                throw std::invalid_argument("Invalid %-sequence");
            }
            
            result += static_cast<char>(value);
            i += 2; // Пропускаем два символа после %
        } else {
            // Остальные символы копируем как есть
            result += str[i];
        }
    }
    
    return result;
}

