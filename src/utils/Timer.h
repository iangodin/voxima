#pragma once

#include <chrono>

class Timer
{
public:
	Timer( std::string message ) :
		_message( std::move( message ) ),
		_start( std::chrono::high_resolution_clock::now() )
	{
	}

	~Timer( void )
	{
		std::cout << _message << ' ' << stop() << "ms\n";
	}

private:
	double stop() const
	{
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> elapsed = end - _start;
		return elapsed.count();
	}

	std::string _message;
	std::chrono::high_resolution_clock::time_point _start;
};
