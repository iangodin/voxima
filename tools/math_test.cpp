
#include "core/Stage.h"
#include "core/Screenplay.h"
#include "core/Producer.h"

#include "visuals/CreateMath.h"
#include "visuals/Group.h"

#include "themes/NewComputerModern.h"

#include "visuals/CreateText.h"

#include "core/Wait.h"

#include <lunasvg/lunasvg.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

Screenplay math_test( Stage &stage )
{
	auto large = FontStyle( "prose" ).huge();
	auto math_style = FontStyle( "math" ).huge();

	auto &label = stage.visuals.add( make_text( *stage.theme, "Math Layout Test 😀", large ) );
	center_group( label );
	co_await wait_for( 1 );
	stage.visuals.clear();

	create_math( stage,
		"\\int_{0}^{\\infty} \\frac{\\sin(x)}{x} dx = \\frac{\\pi}{2}", math_style );
	co_await wait_for( 1 );
	stage.visuals.clear();

	create_math( stage,
		"x+y", math_style );
	co_await wait_for( 1 );
	stage.visuals.clear();

	create_math( stage,
		"\\sqrt{x^2+y^2}", math_style );
	co_await wait_for( 1 );
	stage.visuals.clear();

	create_math( stage,
		"\\sum_{n=1}^{\\infty} \\frac{1}{n^2} = \\frac{\\pi^2}{6}", math_style );
	co_await wait_for( 1 );
	stage.visuals.clear();

	create_math( stage,
		"x = \\sum_{i=1}^{n} \\left( \\frac{i^2}{2} \\right)", math_style );
	co_await wait_for( 1 );
	stage.visuals.clear();

	create_math( stage,
		"A = \\left[\\matrix{\\frac{1}{2} & \\beta \\cr \\gamma & \\delta^2}\\right]", math_style );
	co_await wait_for( 1 );
	stage.visuals.clear();
}

int main()
{
	Theme newComputerModern = NewComputerModern::create_theme();
	Stage stage;
	stage.theme = Theme::get_by_name( "New Computer Modern" );
	stage.background = { 0.1f, 0.1f, 0.2f, 1.0f }; // Dark blue background

	auto movie = math_test( stage );
	Producer producer( std::move(movie), stage );

	while ( producer.tick() )
	{
		std::ostringstream svg;
		stage.save_svg( svg );
		{
			std::ostringstream svgfile;
			svgfile << "frames/frame_" << std::setfill('0') << std::setw(4) << producer.current_frame() << ".svg";
			std::ofstream svgoutput( svgfile.str() );
			svgoutput << svg.str();
			svgoutput.close();
		}

		{
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

			std::ostringstream filename;
			filename << "frames/frame_" << std::setfill('0') << std::setw(4) << producer.current_frame() << ".png";
			bitmap.writeToPng( filename.str() );
		}
		std::cout << "Produced frame " << producer.current_frame() << '\n';
	}

	return 0;
}
