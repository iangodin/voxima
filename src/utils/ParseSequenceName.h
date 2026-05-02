
#pragma once

#include <string>
#include <tuple>
#include <algorithm>

std::tuple<std::string, int, std::string>
parse_sequence_name( const std::string &input )
{
	// We define "symbols" as potential placeholders (safe from shell/path conflicts)
	const std::string symbols = "@+=#$";

	// 1. Find the first character that matches our symbol list
	size_t start_pos = input.find_first_of( symbols );

	if ( start_pos == std::string::npos )
	{
		// No placeholder found. Return whole string as prefix, 0 padding, empty suffix.
		return { input, 0, "" };
	}

	// 2. Count how many times that specific character repeats consecutively
	char symbol = input[start_pos];
	size_t end_pos = input.find_first_not_of( symbol, start_pos );

	// If find_first_not_of returns npos, the symbol goes to the end of the string
	if ( end_pos == std::string::npos )
	{
		end_pos = input.length();
	}

	int count = static_cast<int>( end_pos - start_pos );

	// 3. Split the string into the three parts
	std::string prefix = input.substr( 0, start_pos );
	std::string suffix = input.substr( end_pos );

	return { prefix, count, suffix };
}
