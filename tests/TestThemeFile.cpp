#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <core/ThemeFile.h>
#include <fonts/FontFamily.h>
#include <utils/Huml.h>

#include <algorithm>
#include <filesystem>
#include <fstream>

using Catch::Matchers::ContainsSubstring;

namespace
{

huml::Value parse( const std::string &text )
{
	huml::Parser parser( text );
	auto result = parser.parse();
	REQUIRE( std::holds_alternative<huml::Value>( result ) );
	return std::get<huml::Value>( result );
}

FontFamily family_of( const std::string &text )
{
	return FontFamily::from_huml( "test", parse( text ) );
}

// What a family's files give for a style (the files are the dict's style keys)
std::string pick( const std::string &text, Weight weight, bool italic )
{
	static const char *keys[4][2] = { { "light", "light_italic" }, { "regular", "italic" }, { "bold", "bold_italic" }, { "black", "black_italic" } };
	static const Weight weights[4] = { Weight::LIGHT, Weight::REGULAR, Weight::BOLD, Weight::BLACK };

	const huml::Value dict = parse( text );
	family_of( text ); // (a family that is as it should be)
	FontFamily::Files files;
	for ( size_t w = 0; w < 4; ++w )
	{
		for ( int i = 0; i < 2; ++i )
		{
			if ( const auto &value = dict[keys[w][i]]; value.is_string() )
			{
				files.set( weights[w], i == 1, value.as_string() );
			}
		}
	}
	return files.pick( weight, italic );
}

std::string first_error( const std::function<void()> &f )
{
	try
	{
		f();
	}
	catch ( const std::exception &e )
	{
		return e.what();
	}
	return "";
}

std::string sources_of( Theme &theme, const char *intent, Weight weight = Weight::REGULAR, Scale scale = Scale::MEDIUM )
{
	FontProperties properties;
	properties.weight = weight;
	properties.scale = scale;
	FontStack stack = theme.get_stack( FontStyle( intent, properties ) );
	std::string out;
	for ( size_t i = 0; i < stack.size(); ++i )
		out += stack.font( i ).source.substr( 0, stack.font( i ).source.find( '#' ) ) + ";";
	return out;
}

}

TEST_CASE( "A style with no file is the nearest that has one", "[themes][fonts]" )
{
	const std::string regular_and_bold = "regular: \"R\"\nbold: \"B\"\n";
	CHECK( pick( regular_and_bold, Weight::REGULAR, false ) == "R" );
	CHECK( pick( regular_and_bold, Weight::BOLD, false ) == "B" );
	CHECK( pick( regular_and_bold, Weight::BLACK, false ) == "B" );   // black is bold
	CHECK( pick( regular_and_bold, Weight::LIGHT, false ) == "R" );   // light is regular
	CHECK( pick( regular_and_bold, Weight::REGULAR, true ) == "R" );  // no italic: the upright
	CHECK( pick( regular_and_bold, Weight::BOLD, true ) == "B" );

	// With italics: the same slant first, moving the weight toward regular, then the upright ones
	const std::string full = "regular: \"R\"\nitalic: \"I\"\nbold: \"B\"\nbold_italic: \"BI\"\nlight: \"L\"\n";
	CHECK( pick( full, Weight::BLACK, true ) == "BI" );
	CHECK( pick( full, Weight::LIGHT, true ) == "I" ); // no light italic: the (regular) italic, not the light upright
	CHECK( pick( full, Weight::LIGHT, false ) == "L" );

	// A family of one file serves every style
	CHECK( pick( "bold: \"only\"\n", Weight::REGULAR, false ) == "only" );
	CHECK( pick( "light: \"only\"\n", Weight::BOLD, true ) == "only" );
}

TEST_CASE( "A font family says what is wrong with it", "[themes][fonts]" )
{
	auto error = []( const std::string &text ) { return first_error( [&] { family_of( text ); } ); };

	CHECK_THAT( error( "regular: \"R\"\nbogus: \"x\"\n" ), ContainsSubstring( "font family 'test'" ) );
	CHECK_THAT( error( "regular: \"R\"\nbogus: \"x\"\n" ), ContainsSubstring( "unknown key 'bogus'" ) );
	CHECK_THAT( error( "regular: 12\n" ), ContainsSubstring( "file name in quotes" ) );
	CHECK_THAT( error( "face: 2\n" ), ContainsSubstring( "no files" ) );
	CHECK_THAT( error( "variable: \"V\"\ncollection: \"C\"\n" ), ContainsSubstring( "more than one" ) );
	CHECK_THAT( error( "collection: \"C\"\n" ), ContainsSubstring( "faces" ) );
	CHECK_THAT( error( "collection: \"C\"\nfaces:: medium: 3\n" ), ContainsSubstring( "not a weight" ) );
	CHECK_THAT( error( "notdef: \"N\"\nregular: \"R\"\n" ), ContainsSubstring( "only its file" ) );
	CHECK_THAT( error( "regular: \"R\"\nsmall:: bogus: \"x\"\n" ), ContainsSubstring( "unknown key 'bogus' in small" ) );
	CHECK( error( "regular: \"R\"\nsmall:: regular: \"r\"\n" ).empty() );
	CHECK( error( "variable: \"V\"\nweight: 400\n" ).empty() );
}

TEST_CASE( "The built-in themes are files, and their names are known", "[themes]" )
{
	auto names = themes::builtin_names();
	std::sort( names.begin(), names.end() );
	CHECK( names == std::vector<std::string>{ "Academic Classic", "Engineering Blueprint", "Modern Technical", "New Computer Modern", "Playful Discovery" } );

	Theme theme = themes::load( "Playful Discovery" );
	CHECK( theme.get_name() == "Playful Discovery" );
	CHECK( theme.palette().name == "catppuccin-mocha" );
	CHECK( theme.palette().background == Palette::get( "catppuccin-mocha" ).background ); // whole palette, background included

	CHECK_THAT( first_error( [] { themes::load( "Nonesuch" ); } ), ContainsSubstring( "Modern Technical" ) ); // it lists the ones there are
}

TEST_CASE( "A theme from text: families in stacks, and intents on stacks", "[themes]" )
{
	Theme theme = themes::from_text( R"huml(
meta::
  name: "Tiny"
  inherit: null

fonts::
  main::
    regular: "NewCM10-Regular.otf"
    bold: "NewCM10-Bold.otf"
  spare:: regular: "Amiri-Regular.ttf"
  notdef:: notdef: "AND-Regular.otf"

stacks::
  more:: "spare", "notdef"
  text:: "main", "...more"

intents::
  prose: "text"
)huml" );
	CHECK( theme.get_name() == "Tiny" );
	CHECK( sources_of( theme, "prose" ) == "NewCM10-Regular.otf;Amiri-Regular.ttf;AND-Regular.otf;" );
	CHECK( sources_of( theme, "prose", Weight::BOLD ) == "NewCM10-Bold.otf;Amiri-Regular.ttf;AND-Regular.otf;" );

	// An intent it does not have is an error as it always was, naming the intent
	CHECK_THROWS_WITH( theme.get_stack( FontStyle( "code" ) ), ContainsSubstring( "code" ) );
}

TEST_CASE( "A theme that inherits changes only what it says", "[themes]" )
{
	Theme base = themes::load( "Modern Technical" );
	Theme mine = themes::from_text( R"huml(
meta::
  name: "Mine"
  inherit: "Modern Technical"

palette: "paper"

fonts::
  fira::
    bold: "FiraGO-Regular.ttf"
  amiri:: regular: "Amiri-Regular.ttf"

stacks::
  code:: "amiri", "...fallbacks"
)huml" );

	CHECK( mine.get_name() == "Mine" );
	CHECK( mine.palette().name == "paper" );
	CHECK( base.palette().name == "github-dark" );

	// Merged into the family: only its bold is changed
	CHECK( sources_of( base, "prose", Weight::BOLD ).starts_with( "FiraGO-Bold.ttf;" ) );
	CHECK( sources_of( mine, "prose", Weight::BOLD ).starts_with( "FiraGO-Regular.ttf;" ) );
	CHECK( sources_of( mine, "prose", Weight::REGULAR ) == sources_of( base, "prose", Weight::REGULAR ) );
	CHECK( sources_of( mine, "title" ) == sources_of( base, "title" ) );

	// A stack given again is replaced whole: the code font is Amiri now, with the same fallbacks after it
	CHECK( sources_of( base, "code" ).starts_with( "FiraMono-Regular.ttf;" ) );
	CHECK( sources_of( mine, "code" ).starts_with( "Amiri-Regular.ttf;" ) );
	CHECK( sources_of( mine, "code" ).ends_with( sources_of( base, "code" ).substr( sources_of( base, "code" ).find( ';' ) + 1 ) ) );
}

TEST_CASE( "A theme file says where and what is wrong", "[themes]" )
{
	auto error = []( const std::string &body ) { return first_error( [&] { themes::from_text( "meta::\n  name: \"T\"\n  inherit: null\n\n" + body ); } ); };

	CHECK_THAT( error( "stacks::\n  s:: \"nothing\"\n" ), ContainsSubstring( "'nothing', which is not a font family" ) );
	CHECK_THAT( error( "fonts::\n  a:: regular: \"R\"\nstacks::\n  s:: \"...other\"\n" ), ContainsSubstring( "'...other', which is not a stack" ) );
	CHECK_THAT( error( "fonts::\n  a:: regular: \"R\"\nstacks::\n  s:: \"a\", \"...s\"\n" ), ContainsSubstring( "contains itself" ) );
	CHECK_THAT( error( "stacks::\n  s:: 5\n" ), ContainsSubstring( "not a family name in quotes" ) );
	CHECK_THAT( error( "stacks::\n  s: \"a\"\n" ), ContainsSubstring( "is a list of family names" ) );
	CHECK_THAT( error( "intents::\n  prose: \"nostack\"\n" ), ContainsSubstring( "intent 'prose' uses a stack that is not there" ) );
	CHECK_THAT( error( "colors::\n  a: 1\n" ), ContainsSubstring( "unknown section 'colors'" ) );
	CHECK_THAT( error( "palette: \"nonesuch\"\n" ), ContainsSubstring( "paper" ) ); // it lists the palettes
	CHECK_THAT( first_error( [] { themes::from_text( "fonts::\n  a:: regular: \"R\"\n" ); } ), ContainsSubstring( "meta" ) );
	CHECK_THAT( first_error( [] { themes::from_text( "meta::\n  name: \"T\"\n  inherit: \"Nonesuch\"\n" ); } ), ContainsSubstring( "no theme 'Nonesuch'" ) );
}

TEST_CASE( "Themes are found in directories, and can inherit each other", "[themes]" )
{
	const auto directory = std::filesystem::temp_directory_path() / "voxima-theme-test";
	std::filesystem::create_directories( directory );
	std::ofstream( directory / "first.huml" ) << "meta::\n  name: \"First\"\n  inherit: \"Modern Technical\"\n\npalette: \"paper\"\n";
	std::ofstream( directory / "second.huml" ) << "meta::\n  name: \"Second\"\n  inherit: \"first\"\n\nfonts::\n  fira::\n    bold: \"FiraGO-Regular.ttf\"\n";
	std::ofstream( directory / "loop.huml" ) << "meta::\n  name: \"Loop\"\n  inherit: \"loop\"\n";

	Theme second = themes::load( "second", { directory } );
	CHECK( second.get_name() == "Second" );
	CHECK( second.palette().name == "paper" ); // from first, through it
	CHECK( sources_of( second, "prose", Weight::BOLD ).starts_with( "FiraGO-Regular.ttf;" ) ); // its own change
	CHECK( sources_of( second, "code" ).starts_with( "FiraMono-Regular.ttf;" ) ); // Modern Technical's, through both

	// By path too
	Theme by_path = themes::load( ( directory / "first.huml" ).string() );
	CHECK( by_path.get_name() == "First" );

	CHECK_THAT( first_error( [&] { themes::load( "loop", { directory } ); } ), ContainsSubstring( "inherits itself" ) );
	std::filesystem::remove_all( directory );
}
