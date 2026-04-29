
#include "Stage.h"
#include "Screenplay.h"
#include "Producer.h"
#include "Font.h"
#include "Group.h"
#include "NewComputerModern.h"
#include "CreateText.h"

#include <lunasvg/lunasvg.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

Font math_font( "" );

Screenplay math_test( Stage &stage )
{
	float font_size = 0.5F;

	create_text( stage,
		"Math Layout Test", TextStyle{ "prose" }.huge() );
	co_await wait_for( 1 );

	stage.visuals.set( 0, math_font.create_math(
		"\\int_{0}^{\\infty} \\frac{\\sin(x)}{x} dx = \\frac{\\pi}{2}", font_size ) );
	co_await wait_for( 1 );

	stage.visuals.set( 0, math_font.create_math(
		"x+y", font_size) );
	co_await wait_for( 1 );

	stage.visuals.set( 0, math_font.create_math(
		"\\sqrt{x^2+y^2}", font_size ) );
	co_await wait_for( 1 );

	stage.visuals.set( 0, math_font.create_math(
		"\\sum_{n=1}^{\\infty} \\frac{1}{n^2} = \\frac{\\pi^2}{6}", font_size ) );
	co_await wait_for( 1 );

	stage.visuals.set( 0, math_font.create_math(
		"x = \\sum_{i=1}^{n} \\left( \\frac{i^2}{2} \\right)", font_size ) );
	co_await wait_for( 1 );

	stage.visuals.set( 0, math_font.create_math(
		"A = \\left[\\matrix{\\frac{1}{2} & \\beta \\cr \\gamma & \\delta^2}\\right]", font_size ) );
	co_await wait_for( 1 );
}

int main()
{
	Stage stage;
	stage.theme = NewComputerModern::create_theme();
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
