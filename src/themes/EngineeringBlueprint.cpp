#include <themes/EngineeringBlueprint.h>

#include <core/ThemeFile.h>

namespace EngineeringBlueprint
{

// Its fonts and palette are in themes/engineering-blueprint.huml
Theme create_theme( void )
{
	return themes::load( "Engineering Blueprint" );
}

}
