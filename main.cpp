
#include "Stage.h"
#include "Screenplay.h"
#include "Producer.h"

#include "Visuals.h"
#include "Circle.h"
#include "Rectangle.h"
#include "RoundRectangle.h"
#include "MyMovie.h"

#include <lunasvg/lunasvg.h>

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
		std::ostringstream filename;
		filename << "frames/frame_" << std::setfill('0') << std::setw(4) << producer.current_frame() << ".png";
		std::ostringstream svg;
		stage.save_svg( svg );

		auto doc = lunasvg::Document::loadFromData( svg.str() );
		if ( doc == nullptr )
		{
			std::cerr << "ERROR: could not load SVG\n";
			return -1;
		}

		auto bitmap = doc->renderToBitmap();
		if ( bitmap.isNull() )
		{
			std::cerr << "ERROR: could not render to bitmap\n";
			return -1;
		}

		bitmap.writeToPng( filename.str() );

		std::cout << "Produced frame " << producer.current_frame() << '\n';
	}

	return 0;
}
