#include <catch2/catch_test_macros.hpp>

#include <code/Highlight.h>

#include <string>
#include <string_view>

using namespace code;

namespace
{
// What the span covering the (first) occurrence of `part` is called, "" if it is plain, or "(split)" if more
// than one span covers it
std::string name_of( const std::vector<Span> &spans, std::string_view source, std::string_view part, size_t from = 0 )
{
	const size_t at = source.find( part, from );
	REQUIRE( at != std::string_view::npos );
	std::string name;
	bool found = false;
	for ( const Span &s: spans )
	{
		if ( s.end <= at || s.start >= at + part.size() )
		{
			continue;
		}
		if ( s.start > at || s.end < at + part.size() || found )
		{
			return "(split)";
		}
		name = s.name;
		found = true;
	}
	return name;
}
}

TEST_CASE( "language_named knows C and C++", "[code]" )
{
	CHECK( language_named( "c" ) == Language::C );
	CHECK( language_named( "C++" ) == Language::CPP );
	CHECK( language_named( "cpp" ) == Language::CPP );
	CHECK( language_named( "cxx" ) == Language::CPP );
	CHECK_FALSE( language_named( "python" ) );
}

TEST_CASE( "C++ is highlighted part by part", "[code]" )
{
	const std::string_view source =
		"#include <vector>\n"
		"// Count to the most\n"
		"template<typename T>\n"
		"int count( const std::vector<T> &items )\n"
		"{\n"
		"\tconstexpr int MOST = 10;\n"
		"\tauto total = items.size() + 3;\n"
		"\tif ( total > MOST ) return MOST;\n"
		"\tstd::printf( \"%d\", total );\n"
		"\treturn total;\n"
		"}\n";
	const auto spans = highlight( Language::CPP, source );

	CHECK( name_of( spans, source, "#include" ) == "keyword" );
	CHECK( name_of( spans, source, "<vector>" ) == "string" );
	CHECK( name_of( spans, source, "// Count to the most" ) == "comment" );
	CHECK( name_of( spans, source, "template" ) == "keyword" ); // from C++'s query
	CHECK( name_of( spans, source, "typename" ) == "keyword" );
	CHECK( name_of( spans, source, "int" ) == "type" );
	CHECK( name_of( spans, source, "count" ) == "function" ); // a later, more particular pattern than "variable"
	CHECK( name_of( spans, source, "const" ) == "keyword" ); // (the first one, before std::vector)
	CHECK( name_of( spans, source, "constexpr" ) == "keyword" );
	CHECK( name_of( spans, source, "MOST" ) == "constant" ); // #match? on all capitals
	CHECK( name_of( spans, source, "total" ) == "variable" );
	CHECK( name_of( spans, source, "10" ) == "number" );
	CHECK( name_of( spans, source, "auto" ) == "type" );
	CHECK( name_of( spans, source, "size" ) == "function" ); // a method called
	CHECK( name_of( spans, source, "printf" ) == "function" );
	CHECK( name_of( spans, source, "\"%d\"" ) == "string" );
	CHECK( name_of( spans, source, "return" ) == "keyword" );
	CHECK( name_of( spans, source, "if" ) == "keyword" );

	// In order, and never overlapping
	for ( size_t i = 1; i < spans.size(); ++i )
	{
		CHECK( spans[i].start >= spans[i - 1].end );
	}
}

TEST_CASE( "C is highlighted with C's query alone", "[code]" )
{
	const std::string_view source = "static int twice( int x ) { return x * 2; }\n";
	const auto spans = highlight( Language::C, source );
	CHECK( name_of( spans, source, "static" ) == "keyword" );
	CHECK( name_of( spans, source, "twice" ) == "function" );
	CHECK( name_of( spans, source, "x" ) == "variable" );
	CHECK( name_of( spans, source, "2" ) == "number" );
}

TEST_CASE( "Broken code is still highlighted where it can be", "[code]" )
{
	const std::string_view source = "int main( { return 0;\n";
	const auto spans = highlight( Language::CPP, source );
	CHECK( name_of( spans, source, "return" ) == "keyword" );
	CHECK( name_of( spans, source, "0" ) == "number" );
	CHECK( highlight( Language::CPP, "" ).empty() );
}
