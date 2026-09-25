#include <catch2/catch_test_macros.hpp>
#include <exporters/SVGWriter.h>
#include <visuals/Glyph.h>
#include <visuals/Group.h>
#include <visuals/Image.h>

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
