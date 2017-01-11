#include "config.h"
#if TESTS_ACTIVE

#include "catch.hpp"
#include "../utils.h"
TEST_CASE("Split works on chars", "[utils]")
{
	static const std::string test_string = "1,2,3,4,5";
	static const std::vector<std::string> test_ok = { "1", "2", "3", "4", "5"};
	auto vec = split(test_string, ",");
	REQUIRE(vec == test_ok);
}

TEST_CASE("Split works on words", "[utils]")
{
	static const std::string test_string = "1delim2delim3delim4delim5";
	static const std::vector<std::string> test_ok = { "1", "2", "3", "4", "5" };
	auto vec = split(test_string, "delim");
	REQUIRE(vec == test_ok);
}

TEST_CASE("Split works on many delimeters in a row", "[utils]")
{
	static const std::string test_string = "1delimdelim2delim3delim4delimdelimdelim5";
	static const std::vector<std::string> test_ok = { "1", "2", "3", "4", "5" };	
	auto vec = split(test_string, "delim");
	REQUIRE(vec == test_ok);
}

#endif