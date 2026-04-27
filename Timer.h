#pragma once

#include <chrono>

class Timer
{
public:
	Timer() : _start( std::chrono::high_resolution_clock::now() ) {}

	double stop() const
	{
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> elapsed = end - _start;
		return elapsed.count();
	}

private:
	std::chrono::high_resolution_clock::time_point _start;
};
