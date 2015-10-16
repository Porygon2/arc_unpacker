#include "test_support/catch.hh"
#include "types.h"

using namespace au;

TEST_CASE("bstr()", "[core][types]")
{
    bstr x;
    REQUIRE(x == ""_b);
    REQUIRE(x.size() == 0);
}

TEST_CASE("bstr.size and bstr.empty", "[core][types]")
{
    SECTION("Empty")
    {
        bstr x("");
        REQUIRE(x.size() == 0);
        REQUIRE(x.empty());
    }
    SECTION("Plain")
    {
        bstr x("test", 4);
        REQUIRE(x.size() == 4);
        REQUIRE(!x.empty());
    }
    SECTION("NULL terminated")
    {
        bstr x("\x00\x01", 2);
        REQUIRE(x.size() == 2);
        REQUIRE(!x.empty());
        bstr y(std::string("\x00\x01", 2));
        REQUIRE(y.size() == 2);
        REQUIRE(!y.empty());
    }
}

TEST_CASE("bstr(size_t, u8", "[core][types]")
{
    bstr x(6);
    REQUIRE(x == "\x00\x00\x00\x00\x00\x00"_b);
    REQUIRE(x.size() == 6);
    bstr y(6, '\xFF');
    REQUIRE(y == "\xFF\xFF\xFF\xFF\xFF\xFF"_b);
    REQUIRE(y.size() == 6);
}

TEST_CASE("bstr(char*)", "[core][types]")
{
    bstr x("test\x00\x01", 6);
    REQUIRE(x == "test\x00\x01"_b);
    REQUIRE(x.size() == 6);
}

TEST_CASE("bstr(std::string)", "[core][types]")
{
    bstr x(std::string("test\x00\x01", 6));
    REQUIRE(x == "test\x00\x01"_b);
    REQUIRE(x.size() == 6);
    bstr y(std::string("test\x00\x01"));
    REQUIRE(y == "test"_b);
    REQUIRE(y.size() == 4);
}

TEST_CASE("bstr.str", "[core][types]")
{
    bstr x("test\x00\x01", 6);
    REQUIRE(x.str().size() == 6);
    REQUIRE(x.str() == std::string("test\x00\x01", 6));
    bstr y("test\x00\x01", 6);
    REQUIRE(y.str(true).size() == 4);
    REQUIRE(y.str(true) == std::string("test", 4));
}

TEST_CASE("bstr.find", "[core][types]")
{
    bstr x("test\x00\x01", 6);
    REQUIRE(x.find("y"_b) == bstr::npos);
    REQUIRE(x.find("e"_b) != bstr::npos);
    REQUIRE(x.find("e"_b) == 1);
    REQUIRE(x.find("\x00\x01"_b) == 4);
}

TEST_CASE("bstr.substr", "[core][types]")
{
    bstr x("test\x00\x01", 6);
    SECTION("Plain substring")
    {
        REQUIRE(x.substr(0, 6) == x);
        REQUIRE(x.substr(0, 5) == "test\x00"_b);
        REQUIRE(x.substr(1, 5) == "est\x00\x01"_b);
        REQUIRE(x.substr(1) == "est\x00\x01"_b);
        REQUIRE(x.substr(2) == "st\x00\x01"_b);
        REQUIRE(x.substr(1, 0) == ""_b);
    }
    SECTION("Size out of bounds")
    {
        REQUIRE(x.substr(0, 7) == "test\x00\x01"_b);
        REQUIRE(x.substr(1, 7) == "est\x00\x01"_b);
    }
    SECTION("Pseudo negative offsets")
    {
        REQUIRE(x.substr(-1, 0) == ""_b);
        REQUIRE(x.substr(-1, 1) == ""_b);
        REQUIRE(x.substr(-1, 2) == ""_b); // note: not "t"_b...
        REQUIRE(x.substr(-1) == x.substr(0xFFFFFFFFull)); // because of this
        REQUIRE(x.substr(0xFFFFFFFFull) == x.substr(7)); // and this
    }
    SECTION("Pseudo negative sizes")
    {
        REQUIRE(x.substr(1, -1) == "est\x00\x01"_b); // similarly, not ""_b
        REQUIRE(x.substr(1, -1) == x.substr(1, 0xFFFFFFFFull));
        REQUIRE(x.substr(1, 0xFFFFFFFFull) == x.substr(1, 7));
    }
}

TEST_CASE("bstr.resize()", "[core][types]")
{
    bstr x = "\x01\x02"_b;
    x.resize(2);
    REQUIRE(x == "\x01\x02"_b);
    x.resize(4);
    REQUIRE(x == "\x01\x02\x00\x00"_b);
    x.resize(1);
    REQUIRE(x == "\x01"_b);
}

TEST_CASE("bstr.operator+", "[core][types]")
{
    bstr x = "\x00\x01"_b;
    bstr y = "\x00\x02"_b;
    bstr z = x + y;
    REQUIRE(z == "\x00\x01\x00\x02"_b);
}

TEST_CASE("bstr.operator+=(bstr)", "[core][types]")
{
    bstr x = "\x00\x01"_b;
    bstr y = "\x00\x02"_b;
    bstr z = ""_b;
    REQUIRE(z == ""_b);
    z += x;
    z += y;
    REQUIRE(z == "\x00\x01\x00\x02"_b);
}

TEST_CASE("bstr.operator+=(char)", "[core][types]")
{
    bstr z = ""_b;
    REQUIRE(z == ""_b);
    z += 'A';
    z += static_cast<u8>('\xFF');
    REQUIRE(z == "A\xFF"_b);
}

TEST_CASE("bstr.operator==", "[core][types]")
{
    SECTION("Plain")
    {
        bstr x = "a"_b;
        bstr y = "a"_b;
        REQUIRE(x == x);
        REQUIRE(x == y);
        REQUIRE(y == x);
        REQUIRE(y == y);
    }
    SECTION("NULL terminated")
    {
        bstr x = "\x00\x01"_b;
        bstr y = "\x00\x01"_b;
        REQUIRE(x == x);
        REQUIRE(x == y);
        REQUIRE(y == x);
        REQUIRE(y == y);
    }
}

TEST_CASE("bstr.operator!=", "[core][types]")
{
    SECTION("Plain")
    {
        bstr x = "a"_b;
        bstr y = "b"_b;
        REQUIRE(x != y);
        REQUIRE(y != x);
    }
    SECTION("NULL terminated")
    {
        bstr x = "\x00\x01"_b;
        bstr y = "\x00\x00"_b;
        REQUIRE(x != y);
        REQUIRE(y != x);
    }
}

TEST_CASE("bstr.operator[]", "[core][types]")
{
    bstr x = "\x00\x01"_b;
    REQUIRE(x[0] == '\x00');
    REQUIRE(x[1] == '\x01');
    x[0] = 0x0F;
    REQUIRE(x[0] == '\x0F');
}

TEST_CASE("bstr.get", "[core][types]")
{
    bstr x = "\x00\x01"_b;
    REQUIRE(x.get<char>()[0] == 0);
    REQUIRE(x.get<char>()[1] == 1);
    REQUIRE(bstr(x.get<char>(), 2) == "\x00\x01"_b);
}

TEST_CASE("bstr.end", "[core][types]")
{
    bstr x = "\x00\x01"_b;
    REQUIRE(x.end<char>()[-1] == 1);
    REQUIRE(x.end<char>() == x.get<char>() + 2);
    REQUIRE(x.end<u16>() == &x.get<u16>()[1]);
    REQUIRE(x.end<u16>()[-1] == 0x100);

    // align the boundary to the sizeof(T)
    bstr y = "\x00\x01\x02"_b;
    REQUIRE(y.end<u16>() == &y.get<u16>()[1]);
    REQUIRE(y.end<u16>()[-1] == 0x100);
}

TEST_CASE("iterating over bstr", "[core][types]")
{
    bstr tmp = ""_b;
    for (auto c : "123"_b)
    {
        tmp += c;
        tmp += "|"_b;
    }
    REQUIRE(tmp == "1|2|3|"_b);
}

TEST_CASE("bstr.at", "[core][types]")
{
    bstr x = "\x00\x01"_b;
    REQUIRE(x.at(0) == 0);
    REQUIRE(x.at(1) == 1);
    REQUIRE_THROWS(x.at(2));
    REQUIRE_THROWS(x.at(-1));
}
