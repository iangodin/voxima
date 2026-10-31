#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <themes/AcademicClassic.h>
#include <themes/EngineeringBlueprint.h>
#include <themes/ModernTechnical.h>
#include <themes/NewComputerModern.h>
#include <themes/PlayfulDiscovery.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

// The font stacks of the themes, for every intent, weight, slant and size that picks a different file: what each entry
// loads (the file and face, and a variable font's axes). It is the record the themes are kept to: whatever way they are made
// (in code, or from files), they must load these fonts. To make it anew after a change that is meant to alter it, run the
// tests with UPDATE_GOLDEN=1 (and look at the difference before committing it).

namespace
{

const char *name_of( Weight w )
{
	switch ( w )
	{
		case Weight::LIGHT: return "light";
		case Weight::REGULAR: return "regular";
		case Weight::BOLD: return "bold";
		case Weight::BLACK: return "black";
	}
	return "?";
}

const char *name_of( Slant s )
{
	switch ( s )
	{
		case Slant::UPRIGHT: return "upright";
		case Slant::ITALIC: return "italic";
		case Slant::OBLIQUE: return "oblique";
		case Slant::MATH_ITALIC: return "math-italic";
	}
	return "?";
}

const char *name_of( Scale s )
{
	switch ( s )
	{
		case Scale::TINY: return "tiny";
		case Scale::SMALL: return "small";
		case Scale::MEDIUM: return "medium";
		case Scale::LARGE: return "large";
		case Scale::HUGE: return "huge";
	}
	return "?";
}

std::string describe_stacks( Theme &theme )
{
	std::ostringstream out;
	for ( const char *intent: { "prose", "code", "title", "label", "annotation", "caption", "math" } )
	{
		for ( Weight weight: { Weight::LIGHT, Weight::REGULAR, Weight::BOLD, Weight::BLACK } )
		{
			for ( Slant slant: { Slant::UPRIGHT, Slant::ITALIC, Slant::OBLIQUE } )
			{
				// Tiny (like small: the smaller optical sizes), medium and huge
				for ( Scale scale: { Scale::TINY, Scale::MEDIUM, Scale::HUGE } )
				{
					FontProperties properties;
					properties.weight = weight;
					properties.slant = slant;
					properties.scale = scale;
					FontStack stack = theme.get_stack( FontStyle( intent, properties ) );

					out << theme.get_name() << " | " << intent << ' ' << name_of( weight ) << ' ' << name_of( slant ) << ' ' << name_of( scale ) << " |";
					// (a font listed twice in a row is the same as once: the first with the glyph is the one used; the
					// axes of a font that has none are the defaults, and left out)
					std::string previous;
					std::string size;
					for ( size_t i = 0; i < stack.size(); ++i )
					{
						const FontFace &font = stack.font( i );
						std::string source = font.source;
						const std::string defaults = " w400 wd100 sl0 o0/0";
						if ( source.ends_with( defaults ) )
						{
							source.erase( source.size() - defaults.size() );
						}
						const std::string entry = source + ( font.notdef ? " notdef" : "" );
						if ( entry != previous )
						{
							out << ' ' << entry << ';';
						}
						previous = entry;
						size = std::to_string( font.scale ).substr( 0, 6 );
					}
					out << " @" << size;
					out << '\n';
				}
			}
		}
	}
	return out.str();
}

std::string read_file( const std::string &path )
{
	std::ifstream in( path, std::ios::binary );
	std::ostringstream text;
	text << in.rdbuf();
	return text.str();
}

}

TEST_CASE( "The themes load the fonts they always have", "[themes][fonts]" )
{
	std::vector<Theme> themes;
	themes.push_back( NewComputerModern::create_theme() );
	themes.push_back( AcademicClassic::create_theme() );
	themes.push_back( EngineeringBlueprint::create_theme() );
	themes.push_back( ModernTechnical::create_theme() );
	themes.push_back( PlayfulDiscovery::create_theme() );

	std::string actual;
	for ( Theme &theme: themes )
	{
		actual += describe_stacks( theme );
	}

	const std::string path = std::string( TEST_DATA_DIR ) + "/font_stacks.txt";
	if ( std::getenv( "UPDATE_GOLDEN" ) )
	{
		std::ofstream( path, std::ios::binary ) << actual;
		WARN( "wrote " << path );
		return;
	}

	const std::string expected = read_file( path );
	REQUIRE_FALSE( expected.empty() );
	if ( actual != expected )
	{
		// Say the first line that differs
		std::istringstream a( actual );
		std::istringstream e( expected );
		std::string la;
		std::string le;
		size_t line = 1;
		while ( std::getline( a, la ) && std::getline( e, le ) )
		{
			if ( la != le )
			{
				FAIL( "line " << line << " differs:\n  now:      " << la << "\n  expected: " << le );
			}
			++line;
		}
		FAIL( "the number of lines differs" );
	}
	CHECK( actual == expected );
}
