
#include <core/Stage.h>
#include <core/Screenplay.h>
#include <core/Producer.h>

#include <utils/Timer.h>

#include <visuals/Group.h>
#include <visuals/Circle.h>
#include <visuals/Rectangle.h>
#include <visuals/RoundRectangle.h>

#include <themes/NewComputerModern.h>

#include <exporters/FfmpegRender.h>

#include "examples/MyMovie.h"

#include <lunasvg/lunasvg.h>

#include <iostream>
#include <sstream>

int main()
{
	Stage stage;
	stage.theme = NewComputerModern::create_theme();
	stage.background = { 0.1f, 0.1f, 0.2f, 1.0f }; // Dark blue background

	// 1. Create the narrative
	auto movie = my_movie( stage );

	// 2. Hand it to the Producer
	Producer producer( std::move(movie), stage );

	FfmpegRender renderer( "output.mp4" );

	// 3. Run until 'tick' returns false
	while ( true )
	{
		Timer timer1;
		{
			if ( !producer.tick() )
				break;
			std::cout << "Tick: " << timer1.stop() << "ms\n";
		}

		Timer timer2;
		std::ostringstream svg;
		stage.save_svg( svg );
		std::cout << "SVG generation: " << timer2.stop() << "ms\n";

		Timer timer3;
		auto doc = lunasvg::Document::loadFromData( svg.str() );
		if ( doc == nullptr )
		{
			std::cerr << "ERROR: could not load SVG\n";
			return -1;
		}
		std::cout << "SVG parsing: " << timer3.stop() << "ms\n";

		Timer timer4;
		auto bitmap = doc->renderToBitmap();
		if ( bitmap.isNull() )
		{
			std::cerr << "ERROR: could not render to bitmap\n";
			return -1;
		}
		std::cout << "SVG rendering: " << timer4.stop() << "ms\n";

		Timer timer5;
		renderer.encode( bitmap );
		std::cout << "Video encoding: " << timer5.stop() << "ms\n";

		std::cout << "Produced frame " << producer.current_frame() << '\n';
	}

	return 0;
}
