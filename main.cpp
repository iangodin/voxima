
#include "Stage.h"
#include "Screenplay.h"
#include "Producer.h"

#include "Visuals.h"
#include "Circle.h"
#include "Rectangle.h"

#include "MyMovie.h"

#include <iostream>
#include <sstream>
#include <iomanip>

int main()
{
	Stage stage;
	stage.background = { 0.1f, 0.1f, 0.2f, 1.0f }; // Dark blue background

	// 1. Create the narrative
	auto movie = my_movie( stage );

	// 2. Hand it to the Producer
	Producer producer( std::move(movie), stage );

	// 3. Run until 'tick' returns false
	while ( producer.tick() )
	{
		std::cout << producer.current_frame() << ": " << stage.message << '\n';

		std::ostringstream oss;
		oss << "frames/frame_" << std::setfill('0') << std::setw(4) << producer.current_frame() << ".svg";
		stage.save_svg( oss.str() );
	}

	return 0;
}
