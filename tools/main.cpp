
#include <core/Screenplay.h>
#include <core/Producer.h>

#include <themes/AcademicClassic.h>
#include <themes/EngineeringBlueprint.h>
#include <themes/ModernTechnical.h>
#include <themes/PlayfulDiscovery.h>
#include <themes/NewComputerModern.h>

Screenplay my_screenplay( Stage &stage, Script &script );

int main( int argc, char *argv[] )
{
	// These auto-register
	auto theme1 = AcademicClassic::create_theme();
	auto theme2 = EngineeringBlueprint::create_theme();
	auto theme3 = ModernTechnical::create_theme();
	auto theme4 = PlayfulDiscovery::create_theme();
	auto theme5 = NewComputerModern::create_theme();

	int result = 0;

	Producer producer( my_screenplay );
	if ( !producer.produce( argc, argv ) )
		result = -1;

	return result;
}

