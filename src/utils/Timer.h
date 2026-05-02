#pragma once

#include <chrono>
#include <iostream>
#include <iomanip>

class Timer
{
public:
	explicit Timer( std::string message = {} ) :
		_message( std::move( message ) ),
		_start( std::chrono::high_resolution_clock::now() )
	{
	}

	~Timer( void )
	{
		if ( !_message.empty() )
		{
			double ms = stop();
			std::cout << _message << std::fixed << std::setprecision( 3 ) << ms << "ms";
			if ( _count > 0 )
			{
				double per_frame = ms / static_cast<double>( _count );
				std::cout << ' ' << std::fixed << std::setprecision( 1 ) << 1000.0 / per_frame << "fps";
			}
			std::cout << '\n';
		}
	}

	void message( std::string message, size_t count = 0 )
	{
		_message = std::move( message );
		_count = count;
	}

	void abort( void )
	{
		_message.clear();
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
	size_t _count = 0;
};
