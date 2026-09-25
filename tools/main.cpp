
#include <core/Screenplay.h>
#include <core/Producer.h>

#include <exception>
#include <iostream>

#include <themes/AcademicClassic.h>
#include <themes/EngineeringBlueprint.h>
#include <themes/ModernTechnical.h>
#include <themes/PlayfulDiscovery.h>
#include <themes/NewComputerModern.h>

int main( int argc, char *argv[] )
{
	// These auto-register
	auto theme1 = AcademicClassic::create_theme();
	auto theme2 = EngineeringBlueprint::create_theme();
	auto theme3 = ModernTechnical::create_theme();
	auto theme4 = PlayfulDiscovery::create_theme();
	auto theme5 = NewComputerModern::create_theme();

	int result = 0;

	// A mistake on the command line (an unknown scene, a missing script directory ...) is reported, not a crash
	try
	{
		Producer producer;
		if ( !producer.produce( argc, argv ) )
			result = -1;
	}
	catch ( const std::exception &e )
	{
		std::cerr << "Error: " << e.what() << '\n';
		result = 1;
	}

	return result;
}

