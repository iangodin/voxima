#include <themes/NewComputerModern.h>

#include <core/ThemeFile.h>

namespace NewComputerModern
{

// Its fonts and palette are in themes/new-computer-modern.huml
Theme create_theme( void )
{
	return themes::load( "New Computer Modern" );
}

}
