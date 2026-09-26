#include <themes/AcademicClassic.h>

#include <core/ThemeFile.h>

namespace AcademicClassic
{

// Its fonts and palette are in themes/academic-classic.huml
Theme create_theme( void )
{
	return themes::load( "Academic Classic" );
}

}
