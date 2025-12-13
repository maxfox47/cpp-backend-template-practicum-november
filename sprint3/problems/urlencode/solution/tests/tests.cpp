#include <gtest/gtest.h>

#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
}

// Тест 1: Пустая входная строка
TEST(UrlEncodeTestSuite, EmptyString) {
    EXPECT_EQ(UrlEncode(""sv), ""s);
}

// Тест 2: Входная строка без служебных символов
TEST(UrlEncodeTestSuite, StringWithoutSpecialChars) {
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
    EXPECT_EQ(UrlEncode("HelloWorld"sv), "HelloWorld"s);
    EXPECT_EQ(UrlEncode("abc123"sv), "abc123"s);
    EXPECT_EQ(UrlEncode("test-string_123"sv), "test-string_123"s);
    EXPECT_EQ(UrlEncode("test.string~123"sv), "test.string~123"s);
    EXPECT_EQ(UrlEncode("test_string-123"sv), "test_string-123"s);
}

// Тест 3: Входная строка со служебными символами (зарезервированные символы)
TEST(UrlEncodeTestSuite, StringWithReservedChars) {
    EXPECT_EQ(UrlEncode("Hello World!"sv), "Hello+World%21"s);
    EXPECT_EQ(UrlEncode("test#value"sv), "test%23value"s);
    EXPECT_EQ(UrlEncode("price$100"sv), "price%24100"s);
    EXPECT_EQ(UrlEncode("a&b"sv), "a%26b"s);
    EXPECT_EQ(UrlEncode("test'value"sv), "test%27value"s);
    EXPECT_EQ(UrlEncode("(test)"sv), "%28test%29"s);
    EXPECT_EQ(UrlEncode("abc*def"sv), "abc%2Adef"s);
    EXPECT_EQ(UrlEncode("path/to/file"sv), "path%2Fto%2Ffile"s);
    EXPECT_EQ(UrlEncode("time:12:00"sv), "time%3A12%3A00"s);
    EXPECT_EQ(UrlEncode("a;b"sv), "a%3Bb"s);
    EXPECT_EQ(UrlEncode("key=value"sv), "key%3Dvalue"s);
    EXPECT_EQ(UrlEncode("query?param=value"sv), "query%3Fparam%3Dvalue"s);
    EXPECT_EQ(UrlEncode("user@example.com"sv), "user%40example.com"s);
    EXPECT_EQ(UrlEncode("array[0]"sv), "array%5B0%5D"s);
    EXPECT_EQ(UrlEncode("test+value"sv), "test%2Bvalue"s);
    EXPECT_EQ(UrlEncode("a,b,c"sv), "a%2Cb%2Cc"s);
}

// Тест 4: Входная строка с пробелами
TEST(UrlEncodeTestSuite, StringWithSpaces) {
    EXPECT_EQ(UrlEncode("Hello World"sv), "Hello+World"s);
    EXPECT_EQ(UrlEncode("  "sv), "++"s);
    EXPECT_EQ(UrlEncode(" test "sv), "+test+"s);
    EXPECT_EQ(UrlEncode("a b c"sv), "a+b+c"s);
    EXPECT_EQ(UrlEncode("Hello  World"sv), "Hello++World"s);
}

// Тест 5: Входная строка с символами с кодами меньше 32 и большими или равными 128
TEST(UrlEncodeTestSuite, StringWithControlAndExtendedChars) {
    // Символы с кодами < 32 (управляющие символы)
    EXPECT_EQ(UrlEncode("\x00"sv), "%00"s);  // NULL
    EXPECT_EQ(UrlEncode("\x01"sv), "%01"s);  // SOH
    EXPECT_EQ(UrlEncode("\x09"sv), "%09"s);  // TAB
    EXPECT_EQ(UrlEncode("\x0A"sv), "%0A"s);  // LF (newline)
    EXPECT_EQ(UrlEncode("\x0D"sv), "%0D"s);  // CR
    EXPECT_EQ(UrlEncode("\x1F"sv), "%1F"s);  // Unit separator
    
    // Символы с кодами >= 128 (расширенные символы)
    EXPECT_EQ(UrlEncode("\x80"sv), "%80"s);  // 128
    EXPECT_EQ(UrlEncode("\xFF"sv), "%FF"s);  // 255
    EXPECT_EQ(UrlEncode("\xA0"sv), "%A0"s);  // 160 (неразрывный пробел)
    
    // Комбинации
    EXPECT_EQ(UrlEncode("test\x00test"sv), "test%00test"s);
    EXPECT_EQ(UrlEncode("test\x80test"sv), "test%80test"s);
    EXPECT_EQ(UrlEncode("\x01\x02\x03"sv), "%01%02%03"s);
}

// Тест 6: Комбинации различных случаев
TEST(UrlEncodeTestSuite, CombinedCases) {
    EXPECT_EQ(UrlEncode("Hello World!"sv), "Hello+World%21"s);
    EXPECT_EQ(UrlEncode("test@example.com"sv), "test%40example.com"s);
    EXPECT_EQ(UrlEncode("path/to/file?param=value"sv), "path%2Fto%2Ffile%3Fparam%3Dvalue"s);
    EXPECT_EQ(UrlEncode("a+b=c"sv), "a%2Bb%3Dc"s);
    EXPECT_EQ(UrlEncode("test\x00\x80"sv), "test%00%80"s);
    EXPECT_EQ(UrlEncode("Hello\x0AWorld"sv), "Hello%0AWorld"s);
}

// Тест 7: Символы, которые не должны кодироваться
TEST(UrlEncodeTestSuite, UnencodedChars) {
    EXPECT_EQ(UrlEncode("abcdefghijklmnopqrstuvwxyz"sv), "abcdefghijklmnopqrstuvwxyz"s);
    EXPECT_EQ(UrlEncode("ABCDEFGHIJKLMNOPQRSTUVWXYZ"sv), "ABCDEFGHIJKLMNOPQRSTUVWXYZ"s);
    EXPECT_EQ(UrlEncode("0123456789"sv), "0123456789"s);
    EXPECT_EQ(UrlEncode("-._~"sv), "-._~"s);
    EXPECT_EQ(UrlEncode("test-string_123.abc~"sv), "test-string_123.abc~"s);
}

