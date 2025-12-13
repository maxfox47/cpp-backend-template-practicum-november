#include "htmldecode.h"

#include <string>
#include <string_view>
#include <cctype>

std::string HtmlDecode(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    
    size_t i = 0;
    while (i < str.size()) {
        if (str[i] == '&') {
            // Пытаемся найти мнемонику
            size_t start = i;
            size_t j = i + 1;
            
            // Собираем имя мнемоники (до ; или до конца/небуквенного символа)
            std::string mnemonic_name;
            bool has_semicolon = false;
            
            // Проверяем, что после & идут буквы
            if (j < str.size() && std::isalpha(str[j])) {
                // Определяем регистр первой буквы
                bool is_upper = std::isupper(str[j]);
                bool is_lower = std::islower(str[j]);
                
                // Собираем имя мнемоники, проверяя единообразие регистра
                while (j < str.size() && std::isalpha(str[j])) {
                    char c = str[j];
                    // Проверяем единообразие регистра
                    if ((is_upper && !std::isupper(c)) || (is_lower && !std::islower(c))) {
                        // Смешанный регистр - не мнемоника
                        break;
                    }
                    mnemonic_name += c;
                    ++j;
                }
                
                // Проверяем опциональную точку с запятой
                if (j < str.size() && str[j] == ';') {
                    has_semicolon = true;
                    ++j;
                }
                
                // Проверяем, является ли это известной мнемоникой
                if (mnemonic_name == "lt" || mnemonic_name == "LT") {
                    result += '<';
                    i = j;
                    continue;
                } else if (mnemonic_name == "gt" || mnemonic_name == "GT") {
                    result += '>';
                    i = j;
                    continue;
                } else if (mnemonic_name == "amp" || mnemonic_name == "AMP") {
                    result += '&';
                    i = j;
                    continue;
                } else if (mnemonic_name == "apos" || mnemonic_name == "APOS") {
                    result += '\'';
                    i = j;
                    continue;
                } else if (mnemonic_name == "quot" || mnemonic_name == "QUOT") {
                    result += '"';
                    i = j;
                    continue;
                }
                // Если не мнемоника, копируем всю последовательность как есть
                // (от & до текущей позиции j, включая имя и опциональную точку с запятой)
                for (size_t k = start; k < j; ++k) {
                    result += str[k];
                }
                i = j;
                continue;
            }
            
            // Не мнемоника (после & нет букв) - копируем &
            result += '&';
            ++i;
        } else {
            result += str[i];
            ++i;
        }
    }
    
    return result;
}

