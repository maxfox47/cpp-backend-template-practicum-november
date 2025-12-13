#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    // Тест 1: Пустая строка
    BOOST_TEST(UrlDecode(""sv) == ""s);

    // Тест 2: Строка без %-последовательностей
    BOOST_TEST(UrlDecode("Hello World"sv) == "Hello World"s);
    BOOST_TEST(UrlDecode("abc123"sv) == "abc123"s);
    BOOST_TEST(UrlDecode("test-string_123"sv) == "test-string_123"s);

    // Тест 3: Строка с валидными %-последовательностями (разный регистр)
    BOOST_TEST(UrlDecode("Hello%20World"sv) == "Hello World"s);
    BOOST_TEST(UrlDecode("Hello%2BWorld"sv) == "Hello+World"s);
    BOOST_TEST(UrlDecode("%21%40%23"sv) == "!@#"s);
    BOOST_TEST(UrlDecode("test%41TEST"sv) == "testATEST"s); // %41 = 'A'
    BOOST_TEST(UrlDecode("test%61test"sv) == "testatest"s); // %61 = 'a'
    BOOST_TEST(UrlDecode("%48%65%6C%6C%6F"sv) == "Hello"s); // Hello в hex
    BOOST_TEST(UrlDecode("Hello%20World%21"sv) == "Hello World!"s);

    // Тест 4: Строка с символом +
    BOOST_TEST(UrlDecode("Hello+World"sv) == "Hello World"s);
    BOOST_TEST(UrlDecode("test+string+123"sv) == "test string 123"s);
    BOOST_TEST(UrlDecode("+test+"sv) == " test "s);
    // Комбинация + и %-последовательностей
    BOOST_TEST(UrlDecode("Hello+World%21"sv) == "Hello World!"s);

    // Тест 5: Строка с неполными %-последовательностями (должно выбрасывать исключение)
    BOOST_CHECK_THROW(UrlDecode("%"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("test%"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%a"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("test%a"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%1"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("test%1"sv), std::invalid_argument);

    // Тест 6: Строка с невалидными %-последовательностями (должно выбрасывать исключение)
    BOOST_CHECK_THROW(UrlDecode("%GG"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%ZZ"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("test%XX"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%@@"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%  "sv), std::invalid_argument);

    // Тест 7: Зарезервированные символы без кодирования остаются как есть
    BOOST_TEST(UrlDecode("Hello World !"sv) == "Hello World !"s);
    BOOST_TEST(UrlDecode("test@example.com"sv) == "test@example.com"s);
    BOOST_TEST(UrlDecode("path/to/file"sv) == "path/to/file"s);
    BOOST_TEST(UrlDecode("query?param=value"sv) == "query?param=value"s);

    // Тест 8: Комбинации различных случаев
    BOOST_TEST(UrlDecode("Hello%20World%21"sv) == "Hello World!"s);
    BOOST_TEST(UrlDecode("%48%65%6C%6C%6F"sv) == "Hello"s);
    BOOST_TEST(UrlDecode("test+%26+test"sv) == "test & test"s); // %26 = '&'
    BOOST_TEST(UrlDecode("%2B%2B%2B"sv) == "+++"s); // %2B = '+'
}

