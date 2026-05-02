
#include <core/Stage.h>
#include <core/Screenplay.h>
#include <core/Producer.h>

#include <themes/NewComputerModern.h>

#include "examples/MyMovie.h"

int main( int argc, char *argv[] )
{
	int result = 0;
	auto theme = NewComputerModern::create_theme();

	Stage stage;
	stage.theme = Theme::get_by_name( theme.get_name() );
	stage.background = { 0.1f, 0.1f, 0.2f, 1.0f }; // Dark blue background

	auto movie = my_movie( stage );

	Producer producer( std::move(movie), stage );
	if ( !producer.produce( argc, argv ) )
		result = -1;

	return result;
}
