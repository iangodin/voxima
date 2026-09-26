#include "CodePalette.h"

CodePalette CodePalette::dark()
{
	CodePalette palette;
	const auto rgb = Color::rgb;
	palette.plain = { rgb( 171, 178, 191 ) };

	palette.set( "comment", { rgb( 127, 132, 142 ), std::nullopt, Slant::ITALIC } );
	palette.set( "keyword", { rgb( 198, 120, 221 ) } );
	palette.set( "string", { rgb( 152, 195, 121 ) } );
	palette.set( "number", { rgb( 209, 154, 102 ) } );
	palette.set( "constant", { rgb( 209, 154, 102 ) } );
	palette.set( "type", { rgb( 229, 192, 123 ) } );
	palette.set( "function", { rgb( 97, 175, 239 ) } );
	palette.set( "property", { rgb( 224, 108, 117 ) } );
	palette.set( "variable", { rgb( 171, 178, 191 ) } );
	palette.set( "variable.builtin", { rgb( 224, 108, 117 ) } );
	palette.set( "label", { rgb( 224, 108, 117 ) } );
	palette.set( "operator", { rgb( 86, 182, 194 ) } );
	palette.set( "delimiter", { rgb( 171, 178, 191 ) } );
	return palette;
}
