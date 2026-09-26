#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <audio/Timeline.h>

using Catch::Approx;

namespace
{
Timeline timeline( std::vector<std::pair<std::string, double>> cuts, double end )
{
	Timeline t;
	for ( auto &[id, seconds]: cuts )
	{
		t.cuts.push_back( { id, seconds } );
	}
	t.end = end;
	return t;
}

// The same script in two languages: a marker "a" in the middle of a line, and a second line
Timeline english()
{
	return timeline( { { "\nstart", 0.0 }, { "a", 1.0 }, { "\nline1", 3.0 }, { "b", 3.5 } }, 6.0 );
}

Timeline french()
{
	return timeline( { { "\nstart", 0.0 }, { "a", 0.8 }, { "\nline1", 3.5 }, { "b", 4.5 } }, 6.5 );
}
}

TEST_CASE( "a timeline is divided into segments by its cuts", "[timeline]" )
{
	const Timeline t = english();
	REQUIRE( t.segments() == 4 );
	CHECK( t.segment_start( 1 ) == 1.0 );
	CHECK( t.segment_end( 1 ) == 3.0 );
	CHECK( t.segment_length( 0 ) == 1.0 );
	CHECK( t.segment_length( 1 ) == 2.0 );
	CHECK( t.segment_length( 2 ) == 0.5 );
	CHECK( t.segment_length( 3 ) == 2.5 ); // the last runs to the end
}

TEST_CASE( "languages are in step only if they are cut in the same places", "[timeline]" )
{
	CHECK( same_structure( english(), french() ) );
	CHECK( same_structure( english(), english() ) );

	SECTION( "a missing marker" )
	{
		const Timeline fewer = timeline( { { "\nstart", 0.0 }, { "a", 1.0 }, { "\nline1", 3.0 } }, 6.0 );
		CHECK_FALSE( same_structure( english(), fewer ) );
	}

	SECTION( "a different name, or another order" )
	{
		const Timeline renamed = timeline( { { "\nstart", 0.0 }, { "x", 1.0 }, { "\nline1", 3.0 }, { "b", 3.5 } }, 6.0 );
		const Timeline swapped = timeline( { { "\nstart", 0.0 }, { "\nline1", 1.0 }, { "a", 3.0 }, { "b", 3.5 } }, 6.0 );
		CHECK_FALSE( same_structure( english(), renamed ) );
		CHECK_FALSE( same_structure( english(), swapped ) );
	}
}

TEST_CASE( "each segment is as long as the slowest language takes", "[timeline]" )
{
	const Timeline en = english();
	const Timeline fr = french();

	// en: 1.0, 2.0, 0.5, 2.5    fr: 0.8, 2.7, 1.0, 2.0
	const auto lengths = barrier_lengths( { &en, &fr } );
	REQUIRE( lengths.size() == 4 );
	CHECK( lengths[0] == Approx( 1.0 ) );
	CHECK( lengths[1] == Approx( 2.7 ) );
	CHECK( lengths[2] == Approx( 1.0 ) );
	CHECK( lengths[3] == Approx( 2.5 ) );

	const auto starts = segment_starts( lengths );
	REQUIRE( starts.size() == 5 );
	CHECK( starts[0] == 0.0 );
	CHECK( starts[1] == Approx( 1.0 ) );
	CHECK( starts[2] == Approx( 3.7 ) );
	CHECK( starts[3] == Approx( 4.7 ) );
	CHECK( starts[4] == Approx( 7.2 ) ); // the total

	SECTION( "one language on its own keeps its own lengths" )
	{
		const auto own = barrier_lengths( { &en } );
		CHECK( own == std::vector<double>{ 1.0, 2.0, 0.5, 2.5 } );
	}

	SECTION( "nothing to compare" )
	{
		CHECK( barrier_lengths( {} ).empty() );
	}
}

TEST_CASE( "every language's cues land at the same time on the shared timeline", "[timeline]" )
{
	const Timeline en = english();
	const Timeline fr = french();
	const auto starts = segment_starts( barrier_lengths( { &en, &fr } ) );

	// the marker "a" is the start of segment 1 in both languages, at 1.0 on the shared timeline
	CHECK( retimed( en, starts, 1.0 ) == Approx( 1.0 ) );
	CHECK( retimed( fr, starts, 0.8 ) == Approx( 1.0 ) );
	// and the line and "b" after it
	CHECK( retimed( en, starts, 3.0 ) == Approx( 3.7 ) );
	CHECK( retimed( fr, starts, 3.5 ) == Approx( 3.7 ) );
	// "b" starts the last segment, whose new start is the end of the segment before it
	CHECK( retimed( en, starts, 3.5 ) == Approx( 4.7 ) );
	CHECK( retimed( fr, starts, 4.5 ) == Approx( 4.7 ) );
}

TEST_CASE( "a time inside a segment keeps its place in it", "[timeline]" )
{
	const Timeline en = english();
	const Timeline fr = french();
	const auto starts = segment_starts( barrier_lengths( { &en, &fr } ) );

	// half a second into segment 1, wherever the segment starts
	CHECK( retimed( en, starts, 1.5 ) == Approx( starts[1] + 0.5 ) );
	CHECK( retimed( fr, starts, 1.3 ) == Approx( starts[1] + 0.5 ) );

	// before the first cut is the first segment; past the end carries on in the last one
	CHECK( retimed( en, starts, 0.25 ) == Approx( 0.25 ) );
	CHECK( retimed( en, starts, 6.5 ) == Approx( starts[3] + 3.0 ) );

	SECTION( "it never goes backwards" )
	{
		double last = -1.0;
		for ( double t = 0.0; t <= 6.0; t += 0.05 )
		{
			const double now = retimed( en, starts, t );
			CHECK( now >= last );
			last = now;
		}
	}
}

TEST_CASE( "pieces say what audio goes where", "[timeline]" )
{
	const Timeline fr = french();
	const Timeline en = english();
	const auto starts = segment_starts( barrier_lengths( { &en, &fr } ) );
	const auto p = pieces( fr, starts );

	REQUIRE( p.size() == 4 );
	CHECK( p[1].from == Approx( 0.8 ) );
	CHECK( p[1].to == Approx( 3.5 ) );
	CHECK( p[1].dest == Approx( 1.0 ) );
	// the French segment is as long as its barrier (2.7): no padding; segment 0 (0.8 long, barrier 1.0) has 0.2 of it
	CHECK( p[0].to - p[0].from == Approx( 0.8 ) );
	CHECK( starts[1] - starts[0] == Approx( 1.0 ) );
}

TEST_CASE( "marker_cut cuts a little before the word a marker comes in front of", "[timeline]" )
{
	// "one two three": one 0.0-1.0, two 1.5-2.5 (half a second of pause), three 2.56-3.6 (0.06 of pause)
	const std::vector<TimedWord> words{
		{ "one", 0, 0, 3, 0.0, 1.0 },
		{ "two", 0, 4, 7, 1.5, 2.5 },
		{ "three", 0, 8, 13, 2.56, 3.6 } };

	SECTION( "in a long pause: the lead, no more" )
	{
		// the marker is the start of "two" at 1.5; the pause is 0.5, half of it more than the lead
		CHECK( marker_cut( words, 4, 1.5, 0.0 ) == Approx( 1.5 - CUT_LEAD_SECONDS ) );
	}

	SECTION( "in a short pause: half of it, so the cut stays in the pause" )
	{
		CHECK( marker_cut( words, 8, 2.56, 0.0 ) == Approx( 2.56 - 0.03 ) );
	}

	SECTION( "right up against the word before: no lead" )
	{
		const std::vector<TimedWord> tight{ { "a", 0, 0, 1, 0.0, 1.0 }, { "b", 0, 2, 3, 1.0, 2.0 } };
		CHECK( marker_cut( tight, 2, 1.0, 0.0 ) == Approx( 1.0 ) );
	}

	SECTION( "the first word of a line: the pause is from the start of the line" )
	{
		// speech starts at 0.3 after the line's start at 0.0: a gap of 0.3, half of it is 0.15, more than the lead
		const std::vector<TimedWord> late{ { "go", 0, 0, 2, 0.3, 0.8 } };
		CHECK( marker_cut( late, 0, 0.3, 0.0 ) == Approx( 0.3 - CUT_LEAD_SECONDS ) );
	}

	SECTION( "inside a word, or after the last one: where the marker is" )
	{
		CHECK( marker_cut( words, 10, 2.9, 0.0 ) == Approx( 2.9 ) );
		CHECK( marker_cut( words, 13, 3.6, 0.0 ) == Approx( 3.6 ) );
	}

	SECTION( "a line with no words" )
	{
		CHECK( marker_cut( {}, 0, 5.0, 5.0 ) == Approx( 5.0 ) );
	}

	SECTION( "never before the start of the line" )
	{
		const std::vector<TimedWord> at_once{ { "go", 0, 0, 2, 2.0, 2.5 } };
		CHECK( marker_cut( at_once, 0, 2.0, 2.0 ) >= 2.0 );
	}
}
