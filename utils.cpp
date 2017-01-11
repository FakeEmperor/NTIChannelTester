#include "utils.h"
#include <random>
#include <array>

std::vector<std::string> split(const std::string& s, const std::string &delimeter) {
	std::vector<std::string> ret;
	size_t i = 0, ds = delimeter.size();
	while (i != s.npos) {
		size_t inext = s.find(delimeter, i); // find the beginning of a word
		if (i != s.npos)
			ret.push_back(s.substr(i, inext-i)); //insert the word into vector
		i = inext!=s.npos ? s.find_first_not_of(delimeter, inext+ds) : inext;
	}
	return ret;
}


constexpr size_t __num_char_pairs = 3;
typedef std::array<std::pair<char, char>, __num_char_pairs> __char_pairs;
static const __char_pairs __allowed__chars = { { { '0', '9' },{ 'a', 'z' },{ 'A', 'Z' } } };
std::pair<char, __char_pairs> __normalize__pairs(const __char_pairs &pairs)
{
	auto cpy = pairs;
	char max = 0;
	std::transform(cpy.begin(), cpy.end(), cpy.begin(), [&max](const std::pair<char, char> &a)
	{
		char delta = a.second - a.first;
		auto res = std::make_pair<char, char>(std::move(char(max)), char(max + delta));
		max += delta;
		return res;
	});
	max = cpy[cpy.size() - 1].second;
	return std::make_pair<char, __char_pairs>(std::move(max), std::move(cpy));
};

std::string generate_alnum_str(size_t len)
{
	// random generation
	static const auto norm = __normalize__pairs(__allowed__chars);
	static std::uniform_int_distribution<> text_d(0, norm.first);
	std::random_device rd;
	std::mt19937 rng(rd());
	auto determine_char = [](char value) // used to get one pair of chars from uniformly distributed char
	{
		char rval = value;
		for (size_t i = 0; i<norm.second.size(); ++i)
			if (value <= norm.second[i].second && value >= norm.second[i].first)
				return __allowed__chars[i].first + rval;
			else rval -= norm.second[i].second - norm.second[i].first;
			throw std::runtime_error("[doGenerateSource_] Pair for generation not found");
	};
	
	std::string source(len, 0);
	std::generate_n(source.begin(), len, [&rng, &determine_char]() { return determine_char(text_d(rng)); });
	return source;
}
