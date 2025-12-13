#include <catch2/catch_test_macros.hpp>

#include "../src/htmldecode.h"

using namespace std::literals;

TEST_CASE("Text without mnemonics", "[HtmlDecode]") {
    CHECK(HtmlDecode(""sv) == ""s);
    CHECK(HtmlDecode("hello"sv) == "hello"s);
}

// Тест 1: Пустая строка
TEST_CASE("Empty string", "[HtmlDecode]") {
    CHECK(HtmlDecode(""sv) == ""s);
}

// Тест 2: Строка без HTML-мнемоник
TEST_CASE("String without HTML mnemonics", "[HtmlDecode]") {
    CHECK(HtmlDecode("hello world"sv) == "hello world"s);
    CHECK(HtmlDecode("test123"sv) == "test123"s);
    CHECK(HtmlDecode("abc def ghi"sv) == "abc def ghi"s);
    CHECK(HtmlDecode("Johnson&Johnson"sv) == "Johnson&Johnson"s);
}

// Тест 3: Строка с HTML-мнемониками (строчные, с точкой с запятой)
TEST_CASE("String with HTML mnemonics lowercase with semicolon", "[HtmlDecode]") {
    CHECK(HtmlDecode("&lt;"sv) == "<"s);
    CHECK(HtmlDecode("&gt;"sv) == ">"s);
    CHECK(HtmlDecode("&amp;"sv) == "&"s);
    CHECK(HtmlDecode("&apos;"sv) == "'"s);
    CHECK(HtmlDecode("&quot;"sv) == "\""s);
    CHECK(HtmlDecode("Hello&lt;World&gt;"sv) == "Hello<World>"s);
    CHECK(HtmlDecode("M&amp;M"sv) == "M&M"s);
    CHECK(HtmlDecode("test&apos;string"sv) == "test'string"s);
    CHECK(HtmlDecode("&quot;quoted&quot;"sv) == "\"quoted\""s);
}

// Тест 4: Строка с HTML-мнемониками (строчные, без точки с запятой)
TEST_CASE("String with HTML mnemonics lowercase without semicolon", "[HtmlDecode]") {
    CHECK(HtmlDecode("&lt"sv) == "<"s);
    CHECK(HtmlDecode("&gt"sv) == ">"s);
    CHECK(HtmlDecode("&amp"sv) == "&"s);
    CHECK(HtmlDecode("&apos"sv) == "'"s);
    CHECK(HtmlDecode("&quot"sv) == "\""s);
    CHECK(HtmlDecode("Hello&ltWorld&gt"sv) == "Hello<World>"s);
    CHECK(HtmlDecode("M&ampM"sv) == "M&M"s);
    CHECK(HtmlDecode("test&aposstring"sv) == "test'string"s);
}

// Тест 5: Строка с HTML-мнемониками (заглавные, с точкой с запятой)
TEST_CASE("String with HTML mnemonics uppercase with semicolon", "[HtmlDecode]") {
    CHECK(HtmlDecode("&LT;"sv) == "<"s);
    CHECK(HtmlDecode("&GT;"sv) == ">"s);
    CHECK(HtmlDecode("&AMP;"sv) == "&"s);
    CHECK(HtmlDecode("&APOS;"sv) == "'"s);
    CHECK(HtmlDecode("&QUOT;"sv) == "\""s);
    CHECK(HtmlDecode("Hello&lt;World&gt;"sv) == "Hello<World>"s);
    CHECK(HtmlDecode("M&amp;M"sv) == "M&M"s);
    CHECK(HtmlDecode("M&APOS;s"sv) == "M's"s);
}

// Тест 6: Строка с HTML-мнемониками (заглавные, без точки с запятой)
TEST_CASE("String with HTML mnemonics uppercase without semicolon", "[HtmlDecode]") {
    CHECK(HtmlDecode("&LT"sv) == "<"s);
    CHECK(HtmlDecode("&GT"sv) == ">"s);
    CHECK(HtmlDecode("&AMP"sv) == "&"s);
    CHECK(HtmlDecode("&APOS"sv) == "'"s);
    CHECK(HtmlDecode("&QUOT"sv) == "\""s);
    CHECK(HtmlDecode("M&ampM"sv) == "M&M"s);
    CHECK(HtmlDecode("M&APOSs"sv) == "M's"s);
}

// Тест 7: Строка с HTML-мнемониками в смешанном регистре (не должны декодироваться)
TEST_CASE("String with HTML mnemonics in mixed case", "[HtmlDecode]") {
    CHECK(HtmlDecode("&Lt;"sv) == "&Lt;"s);
    CHECK(HtmlDecode("&lT;"sv) == "&lT;"s);
    CHECK(HtmlDecode("&AmP;"sv) == "&AmP;"s);
    CHECK(HtmlDecode("&aPos;"sv) == "&aPos;"s);
    CHECK(HtmlDecode("&QuOt;"sv) == "&QuOt;"s);
    CHECK(HtmlDecode("&Lt"sv) == "&Lt"s);
    CHECK(HtmlDecode("&lT"sv) == "&lT"s);
}

// Тест 8: Строка с HTML-мнемониками в начале, конце и середине
TEST_CASE("String with HTML mnemonics at start, end and middle", "[HtmlDecode]") {
    CHECK(HtmlDecode("&lt;start"sv) == "<start"s);
    CHECK(HtmlDecode("end&gt;"sv) == "end>"s);
    CHECK(HtmlDecode("&lt;middle&gt;"sv) == "<middle>"s);
    CHECK(HtmlDecode("&lt;start middle end&gt;"sv) == "<start middle end>"s);
    CHECK(HtmlDecode("&amp;begin middle&amp;end"sv) == "&begin middle&end"s);
}

// Тест 9: Строка с недописанными HTML-мнемониками
TEST_CASE("String with incomplete HTML mnemonics", "[HtmlDecode]") {
    CHECK(HtmlDecode("&"sv) == "&"s);
    CHECK(HtmlDecode("&a"sv) == "&a"s);
    CHECK(HtmlDecode("&ab"sv) == "&ab"s);
    CHECK(HtmlDecode("&abc"sv) == "&abc"s);
    CHECK(HtmlDecode("test&abracadabra"sv) == "test&abracadabra"s);
    CHECK(HtmlDecode("&unknown;"sv) == "&unknown;"s);
}

// Тест 10: Строка с HTML-мнемониками, заканчивающимися и не заканчивающимися на точку с запятой
TEST_CASE("String with HTML mnemonics with and without semicolon", "[HtmlDecode]") {
    CHECK(HtmlDecode("&lt;&lt"sv) == "<<"s);  // &lt; и &lt
    CHECK(HtmlDecode("&gt;&gt;"sv) == ">>"s);  // &gt; и &gt;
    CHECK(HtmlDecode("&amp;&amp"sv) == "&&"s);  // &amp; и &amp
    CHECK(HtmlDecode("&apos;&apos"sv) == "''"s);  // &apos; и &apos
    CHECK(HtmlDecode("&quot;&quot"sv) == "\"\""s);  // &quot; и &quot
    CHECK(HtmlDecode("M&amp;M&APOSs"sv) == "M&M's"s);
}

// Тест 11: Декодированные символы не участвуют в дальнейшем декодировании
TEST_CASE("Decoded characters do not participate in further decoding", "[HtmlDecode]") {
    CHECK(HtmlDecode("&amp;lt;"sv) == "&lt;"s);  // &amp; декодируется в &, затем &lt; остается как есть
    CHECK(HtmlDecode("&amp;gt;"sv) == "&gt;"s);
    CHECK(HtmlDecode("&amp;amp;"sv) == "&amp;"s);
    CHECK(HtmlDecode("&amp;apos;"sv) == "&apos;"s);
    CHECK(HtmlDecode("&amp;quot;"sv) == "&quot;"s);
}

// Тест 12: Комплексные случаи
TEST_CASE("Complex cases", "[HtmlDecode]") {
    CHECK(HtmlDecode("Johnson&amp;Johnson"sv) == "Johnson&Johnson"s);
    CHECK(HtmlDecode("Johnson&ampJohnson"sv) == "Johnson&Johnson"s);
    CHECK(HtmlDecode("Johnson&AMP;Johnson"sv) == "Johnson&Johnson"s);
    CHECK(HtmlDecode("Johnson&AMPJohnson"sv) == "Johnson&Johnson"s);
    CHECK(HtmlDecode("M&amp;M&APOSs"sv) == "M&M's"s);
    CHECK(HtmlDecode("&lt;tag&gt;content&lt;/tag&gt;"sv) == "<tag>content</tag>"s);
    CHECK(HtmlDecode("test&quot;quoted&quot;string"sv) == "test\"quoted\"string"s);
}

