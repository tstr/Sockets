/*
	String helper functions
*/

#pragma once

#include <string>
#include <vector>
#include <algorithm>

static std::string trim(const std::string& str)
{
	using namespace std;

	if (str == "")
		return "";

	size_t first = max(str.find_first_not_of(' '), str.find_first_not_of('\t'));
	size_t last = min(str.find_last_not_of(' '), str.find_last_not_of('\t'));

	if (first == std::string::npos)
		first = 0;

	return str.substr(first, (last - first + 1));
}

static std::vector<std::string> tokenize(const std::string& str, const char* delim)
{
	using namespace std;

	vector<string> tokens;

	size_t prev_pos = 0;
	size_t pos = 0;

	while ((pos = str.find_first_of(delim, prev_pos)) != string::npos)
	{
		if (pos > prev_pos)
			tokens.push_back(str.substr(prev_pos, pos - prev_pos));

		prev_pos = pos + 1;
	}

	if (prev_pos< str.length())
		tokens.push_back(str.substr(prev_pos, string::npos));

	return tokens;
}