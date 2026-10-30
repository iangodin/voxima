#include <catch2/catch_test_macros.hpp>
#include <exporters/SVGWriter.h>
#include <visuals/Glyph.h>
#include <visuals/Group.h>
#include <visuals/Image.h>
#include <visuals/Path.h>

#include <cmath>
#include <vector>

#include <memory>
#include <sstream>
#include <string>

namespace
{
// A small triangle outline, scaled the way text glyphs are (the scale is baked into the path)
std::unique_ptr<Glyph> make_glyph( float scale, unsigned int index = 42 )
{
	auto glyph = std::make_unique<Glyph>();
	glyph->index = index;
	glyph->commands = { MoveTo{ Point( 0.F, 0.F ) },
		LineTo{ Point( 1.F, 0.F ) },
		LineTo{ Point( 1.F, 1.F ) },
		Close{} };
	*glyph *= scale;
	return glyph;
}

size_t count( const std::string &text, const std::string &needle )
{
	size_t n = 0;
	for ( size_t pos = text.find( needle ); pos != std::string::npos;
	      pos = text.find( needle, pos + needle.size() ) )
	{
		++n;
	}
	return n;
}

bool contains( const std::string &text, const std::string &needle )
{
	return text.find( needle ) != std::string::npos;
}
}

TEST_CASE( "SVGWriter shares glyph definitions by outline", "[exporters]" )
{
	std::ostringstream body;
	std::ostringstream defs;
	SVGWriter writer( body );

	SECTION( "the same glyph at different sizes gets a definition per size" )
	{
		// Same font face and glyph index each time; only the baked-in size differs
		make_glyph( 1.F )->accept( writer );
		make_glyph( 2.F )->accept( writer );
		make_glyph( 1.F )->accept( writer );
		writer.write_defs( defs );

		CHECK( count( defs.str(), "<path id=" ) == 2 );
		CHECK( count( body.str(), "<use href=" ) == 3 );

		// the first and third glyph share definition 0, the double-size one has its own
		CHECK( count( body.str(), "href=\"#glyph_0\"" ) == 2 );
		CHECK( count( body.str(), "href=\"#glyph_1\"" ) == 1 );

		// each definition holds its own outline
		CHECK( contains( defs.str(), "L 1,0" ) );
		CHECK( contains( defs.str(), "L 2,0" ) );
	}

	SECTION( "identical outlines are shared even when the glyph index differs" )
	{
		make_glyph( 1.F, 1 )->accept( writer );
		make_glyph( 1.F, 2 )->accept( writer );
		writer.write_defs( defs );

		CHECK( count( defs.str(), "<path id=" ) == 1 );
		CHECK( count( body.str(), "href=\"#glyph_0\"" ) == 2 );
	}

	SECTION( "a different outline never reuses a definition, whatever its index" )
	{
		auto other = make_glyph( 1.F, 42 );
		other->commands = { MoveTo{ Point( 0.F, 0.F ) }, LineTo{ Point( 3.F, 0.F ) }, Close{} };

		make_glyph( 1.F, 42 )->accept( writer );
		other->accept( writer );
		writer.write_defs( defs );

		CHECK( count( defs.str(), "<path id=" ) == 2 );
		CHECK( contains( defs.str(), "L 3,0" ) );
	}

	SECTION( "no glyphs, no definitions" )
	{
		writer.write_defs( defs );
		CHECK( defs.str().empty() );
	}
}

TEST_CASE( "SVGWriter output is reproducible", "[exporters]" )
{
	auto write = []()
	{
		std::ostringstream body;
		std::ostringstream defs;
		SVGWriter writer( body );
		make_glyph( 1.F, 7 )->accept( writer );
		make_glyph( 0.5F, 7 )->accept( writer );
		make_glyph( 1.F, 9 )->accept( writer );
		writer.write_defs( defs );
		return std::make_pair( body.str(), defs.str() );
	};

	const auto first = write();
	const auto second = write();

	CHECK( first.first == second.first );
	CHECK( first.second == second.second );

	// ids are plain counters, not memory addresses
	CHECK_FALSE( contains( first.first, "0x" ) );
	CHECK_FALSE( contains( first.second, "0x" ) );
}

TEST_CASE( "SVGWriter fades an image by its color's alpha", "[exporters]" )
{
	auto write = []( float alpha, float group_alpha )
	{
		Group group;
		group.color = { 1.F, 1.F, 1.F, group_alpha };
		auto image = std::make_unique<Image>();
		image->path = "picture.png";
		image->color = { 1.F, 1.F, 1.F, alpha };
		group.add( std::move( image ) );

		std::ostringstream body;
		SVGWriter writer( body );
		group.accept( writer );
		return body.str();
	};

	SECTION( "an opaque picture is written as it always was" )
	{
		CHECK_FALSE( contains( write( 1.F, 1.F ), "opacity" ) );
	}

	SECTION( "a see-through one carries its alpha" )
	{
		CHECK( contains( write( 0.5F, 1.F ), "opacity=\"0.5000\"" ) );
	}

	SECTION( "and its group's alpha counts too" )
	{
		CHECK( contains( write( 0.5F, 0.5F ), "opacity=\"0.2500\"" ) );
	}
}

namespace
{
// The numbers after each "C" of a path's d="...", as its end points
std::vector<std::pair<float, float>> cubic_ends( const std::string &svg )
{
	std::vector<std::pair<float, float>> ends;
	std::istringstream in( svg.substr( svg.find( "d=\"" ) + 3 ) );
	std::string token;
	while ( in >> token && token.find( '"' ) == std::string::npos )
	{
		if ( token == "C" )
		{
			std::string c1, c2, to;
			in >> c1 >> c2 >> to;
			const auto comma = to.find( ',' );
			ends.push_back( { std::stof( to.substr( 0, comma ) ), std::stof( to.substr( comma + 1 ) ) } );
		}
	}
	return ends;
}
}

TEST_CASE( "SVGWriter cuts curves up to draw them within the flatness", "[exporters]" )
{
	// A quarter circle of radius 1 as one cubic (it bulges about 0.3 away from its chord)
	constexpr float k = 0.5523F;
	Path arc;
	arc.commands = { MoveTo{ Point( 1.F, 0.F ) }, CubicTo{ Point( 0.F, 1.F ), Point( 1.F, k ), Point( k, 1.F ) } };

	auto write = [&]( float flatness )
	{
		std::ostringstream body;
		SVGWriter writer( body, 2, flatness );
		arc.accept( writer );
		return body.str();
	};

	CHECK( cubic_ends( write( 0.F ) ).size() == 1 ); // as it is

	const auto ends = cubic_ends( write( 0.001F ) );
	CHECK( ends.size() > 4 );
	CHECK( ends.size() <= 64 );
	for ( const auto &[x, y]: ends )
	{
		// Every piece ends on the curve, which is (to within 0.03 %) the circle
		CHECK( std::abs( std::hypot( x, y ) - 1.F ) < 0.001F );
	}
	CHECK( ends.back().first == 0.F );
	CHECK( ends.back().second == 1.F );

	// A straight line is never cut
	Path line;
	line.commands = { MoveTo{ Point( 0.F, 0.F ) }, CubicTo{ Point( 3.F, 0.F ), Point( 1.F, 0.F ), Point( 2.F, 0.F ) } };
	std::ostringstream body;
	SVGWriter writer( body, 2, 0.001F );
	line.accept( writer );
	CHECK( cubic_ends( body.str() ).size() == 1 );
}

TEST_CASE( "SVGWriter leaves out a transform or an opacity too small to see", "[exporters]" )
{
	auto write = []( float scale, float rotation, float alpha )
	{
		auto image = std::make_unique<Image>();
		image->path = "picture.png";
		image->scale_x = image->scale_y = scale;
		image->rotation = rotation;
		image->color = { 1.F, 1.F, 1.F, alpha };
		std::ostringstream body;
		SVGWriter writer( body );
		image->accept( writer );
		return body.str();
	};

	// A picture is drawn a pixel off under any transform, or with any opacity, so a hair from none is none
	CHECK_FALSE( contains( write( 0.99998F, 0.F, 1.F ), "scale(0." ) ); // (every picture has its own scale(1, -1))
	CHECK_FALSE( contains( write( 1.F, 0.0002F, 1.F ), "rotate(" ) );
	CHECK_FALSE( contains( write( 1.F, 0.F, 0.9999F ), "opacity=" ) );

	CHECK( contains( write( 0.99F, 0.F, 1.F ), "scale(0." ) );
	CHECK( contains( write( 1.F, 0.5F, 1.F ), "rotate(" ) );
	CHECK( contains( write( 1.F, 0.F, 0.99F ), "opacity=" ) );
}
