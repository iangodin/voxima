#include <themes/ModernTechnical.h>

#include <core/ThemeFile.h>

namespace ModernTechnical
{

// Its fonts and palette are in themes/modern-technical.huml
Theme create_theme( void )
{
	return themes::load( "Modern Technical" );
}

}
