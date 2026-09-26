#include <themes/PlayfulDiscovery.h>

#include <core/ThemeFile.h>

namespace PlayfulDiscovery
{

// Its fonts and palette are in themes/playful-discovery.huml
Theme create_theme( void )
{
	return themes::load( "Playful Discovery" );
}

}
