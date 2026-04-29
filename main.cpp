
#include "Stage.h"
#include "Screenplay.h"
#include "Producer.h"
#include "Timer.h"
#include "Group.h"
#include "Circle.h"
#include "Rectangle.h"
#include "RoundRectangle.h"
#include "NewComputerModern.h"
#include "MyMovie.h"

#include <lunasvg/lunasvg.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

int main()
{
	Stage stage;
	stage.theme = NewComputerModern::create_theme();
	stage.background = { 0.1f, 0.1f, 0.2f, 1.0f }; // Dark blue background

	// 1. Create the narrative
	auto movie = my_movie( stage );

	// 2. Hand it to the Producer
	Producer producer( std::move(movie), stage );

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

		std::ostringstream svgfile;
		svgfile << "frames/frame_" << std::setfill('0') << std::setw(4) << producer.current_frame() << ".svg";
		std::ofstream svgoutput( svgfile.str() );
		svgoutput << svg.str();
		svgoutput.close();
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
		std::ostringstream filename;
		filename << "frames/frame_" << std::setfill('0') << std::setw(4) << producer.current_frame() << ".png";
		bitmap.writeToPng( filename.str() );
		std::cout << "PNG writing: " << timer5.stop() << "ms\n";

		std::cout << "Produced frame " << producer.current_frame() << '\n';
	}

	return 0;
}
