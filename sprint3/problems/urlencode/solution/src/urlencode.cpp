#include "urlencode.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

std::string UrlEncode(std::string_view str) {
    std::ostringstream encoded;
    encoded << std::hex << std::uppercase;
    
    for (unsigned char c : str) {
        // Пробел кодируется как +
        if (c == ' ') {
            encoded << '+';
        }
        // Зарезервированные символы: !#$&'()*+,/:;=?@[]
        else if (c == '!' || c == '#' || c == '$' || c == '&' || c == '\'' ||
                 c == '(' || c == ')' || c == '*' || c == '+' || c == ',' ||
                 c == '/' || c == ':' || c == ';' || c == '=' || c == '?' ||
                 c == '@' || c == '[' || c == ']') {
            encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
        // Символы с кодами < 32 или >= 128
        else if (c < 32 || c >= 128) {
            encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
        // Остальные символы (буквы, цифры, -._~) остаются без изменений
        else {
            encoded << c;
        }
    }
    
    return encoded.str();
}

