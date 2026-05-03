
#include <core/Stage.h>
#include <core/Screenplay.h>
#include <core/Producer.h>

#include <themes/AcademicClassic.h>
#include <themes/EngineeringBlueprint.h>
#include <themes/ModernTechnical.h>
#include <themes/PlayfulDiscovery.h>

#include "examples/MyMovie.h"

int main( int argc, char *argv[] )
{
	auto theme1 = AcademicClassic::create_theme();
	auto theme2 = EngineeringBlueprint::create_theme();
	auto theme3 = ModernTechnical::create_theme();
	auto theme4 = PlayfulDiscovery::create_theme();

	int result = 0;

	Stage stage;
	stage.theme = Theme::get_by_name( theme1.get_name() );
	stage.background = { 0.1f, 0.1f, 0.2f, 1.0f }; // Dark blue background

	auto movie = my_movie( stage );

	Producer producer( std::move(movie), stage );
	if ( !producer.produce( argc, argv ) )
		result = -1;

	return result;
}
