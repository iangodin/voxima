#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <audio/Volume.h>

#include <algorithm>

using Catch::Approx;

TEST_CASE( "apply_volume scales every sample by the gain", "[audio]" )
{
	AudioBuffer buffer( 2, 48000, 2 );
	std::fill( buffer.begin(), buffer.end(), 0.5F );

	apply_volume( buffer, 0.5F );

	for ( const float *s = buffer.begin(); s != buffer.end(); ++s )
	{
		CHECK( *s == Approx( 0.25F ) );
	}
}

TEST_CASE( "apply_volume with unity gain leaves samples unchanged", "[audio]" )
{
	AudioBuffer buffer( 1, 48000, 1 );
	*buffer.begin() = 0.75F;

	apply_volume( buffer, 1.F );

	CHECK( *buffer.begin() == Approx( 0.75F ) );
}

TEST_CASE( "db_to_gain converts decibels to a linear factor", "[audio]" )
{
	CHECK( db_to_gain( 0.F ) == Approx( 1.F ) );
	CHECK( db_to_gain( 20.F ) == Approx( 10.F ) );
	CHECK( db_to_gain( -20.F ) == Approx( 0.1F ) );
}
