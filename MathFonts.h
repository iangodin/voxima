#pragma once

#include "FontFace.h"

class MathFonts
{
public:
	MathFonts( FontFace roman, FontFace italic, FontFace bold, FontFace sans, FontFace mono )
	    : _roman( std::move( roman ) ),
	      _italic( std::move( italic ) ),
	      _bold( std::move( bold ) ),
	      _sans( std::move( sans ) ),
	      _mono( std::move( mono ) )
	{
	}

	const FontFace &roman() const { return _roman; }
	const FontFace &italic() const { return _italic; }
	const FontFace &bold() const { return _bold; }
	const FontFace &sans() const { return _sans; }
	const FontFace &mono() const { return _mono; }

private:
	FontFace _roman;
	FontFace _italic;
	FontFace _bold;
	FontFace _sans;
	FontFace _mono;
};
