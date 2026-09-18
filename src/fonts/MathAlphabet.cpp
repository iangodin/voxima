#include "fonts/MathAlphabet.h"
#include "utils/Utf8.h"

#include <optional>

namespace
{
struct Hole
{
	char32_t from;
	char32_t to;
};

// Letters whose math-alphabet slot was already taken by an older character
constexpr Hole ITALIC_HOLES[] = { { U'h', 0x210E } };
constexpr Hole SCRIPT_HOLES[] = { { U'B', 0x212C }, { U'E', 0x2130 }, { U'F', 0x2131 },
	{ U'H', 0x210B }, { U'I', 0x2110 }, { U'L', 0x2112 }, { U'M', 0x2133 }, { U'R', 0x211B },
	{ U'e', 0x212F }, { U'g', 0x210A }, { U'o', 0x2134 } };
constexpr Hole FRAKTUR_HOLES[] = { { U'C', 0x212D }, { U'H', 0x210C }, { U'I', 0x2111 },
	{ U'R', 0x211C }, { U'Z', 0x2128 } };
constexpr Hole DOUBLE_STRUCK_HOLES[] = { { U'C', 0x2102 }, { U'H', 0x210D }, { U'N', 0x2115 },
	{ U'P', 0x2119 }, { U'Q', 0x211A }, { U'R', 0x211D }, { U'Z', 0x2124 } };

template<size_t N>
char32_t find_hole( const Hole ( &holes )[N], char32_t c, char32_t fallback )
{
	for ( const auto &hole: holes )
	{
		if ( hole.from == c )
		{
			return hole.to;
		}
	}
	return fallback;
}

struct Alphabet
{
	char32_t upper = 0;         // where 'A' lands (0: no Latin form)
	char32_t lower = 0;         // where 'a' lands
	char32_t digit = 0;         // where '0' lands (0: digits unchanged)
	char32_t greek_upper = 0;   // where Α lands (0: capital Greek unchanged)
	char32_t greek_lower = 0;   // where α lands (0: lowercase Greek unchanged)
};

Alphabet alphabet_for( MathAlphabet variant )
{
	switch ( variant )
	{
		case MathAlphabet::ITALIC:
			return { 0x1D434, 0x1D44E, 0, 0, 0x1D6FC };
		case MathAlphabet::BOLD:
			return { 0x1D400, 0x1D41A, 0x1D7CE, 0x1D6A8, 0x1D6C2 };
		case MathAlphabet::BOLD_ITALIC:
			return { 0x1D468, 0x1D482, 0, 0x1D71C, 0x1D736 };
		case MathAlphabet::SCRIPT:
			return { 0x1D49C, 0x1D4B6, 0, 0, 0 };
		case MathAlphabet::FRAKTUR:
			return { 0x1D504, 0x1D51E, 0, 0, 0 };
		case MathAlphabet::DOUBLE_STRUCK:
			return { 0x1D538, 0x1D552, 0x1D7D8, 0, 0 };
		case MathAlphabet::SANS:
			return { 0x1D5A0, 0x1D5BA, 0x1D7E2, 0, 0 };
		case MathAlphabet::MONO:
			return { 0x1D670, 0x1D68A, 0x1D7F6, 0, 0 };
		case MathAlphabet::UPRIGHT:
			break;
	}
	return {};
}

char32_t hole_for( MathAlphabet variant, char32_t c, char32_t fallback )
{
	switch ( variant )
	{
		case MathAlphabet::ITALIC:
			return find_hole( ITALIC_HOLES, c, fallback );
		case MathAlphabet::SCRIPT:
			return find_hole( SCRIPT_HOLES, c, fallback );
		case MathAlphabet::FRAKTUR:
			return find_hole( FRAKTUR_HOLES, c, fallback );
		case MathAlphabet::DOUBLE_STRUCK:
			return find_hole( DOUBLE_STRUCK_HOLES, c, fallback );
		default:
			return fallback;
	}
}
}

char32_t map_math_char( char32_t c, MathAlphabet variant )
{
	const Alphabet a = alphabet_for( variant );

	if ( c >= U'A' && c <= U'Z' && a.upper )
	{
		return hole_for( variant, c, a.upper + ( c - U'A' ) );
	}
	if ( c >= U'a' && c <= U'z' && a.lower )
	{
		return hole_for( variant, c, a.lower + ( c - U'a' ) );
	}
	if ( c >= U'0' && c <= U'9' && a.digit )
	{
		return a.digit + ( c - U'0' );
	}
	// Α..Ω (03A2 is unassigned, the block keeps a slot for it)
	if ( c >= 0x391 && c <= 0x3A9 && a.greek_upper )
	{
		return a.greek_upper + ( c - 0x391 );
	}
	// α..ω, including final sigma
	if ( c >= 0x3B1 && c <= 0x3C9 && a.greek_lower )
	{
		return a.greek_lower + ( c - 0x3B1 );
	}
	return c;
}

std::string map_math_text( std::string_view utf8, MathAlphabet variant )
{
	if ( variant == MathAlphabet::UPRIGHT )
	{
		return std::string( utf8 );
	}

	std::string out;
	out.reserve( utf8.size() * 2 );
	while ( !utf8.empty() )
	{
		append_utf8( out, map_math_char( decode_utf8( utf8 ), variant ) );
	}
	return out;
}

MathAlphabet math_alphabet( const FontStyle &style )
{
	const FontProperties props = style.properties();
	const bool math_intent = style.intent() == Intent::MATH;
	const bool bold = props.weight == Weight::BOLD || props.weight == Weight::BLACK;

	std::optional<MathAlphabet> typeclass_alphabet;
	if ( props.typeclass == TypeClass::SANS )
	{
		typeclass_alphabet = MathAlphabet::SANS;
	}
	else if ( props.typeclass == TypeClass::MONO )
	{
		typeclass_alphabet = MathAlphabet::MONO;
	}
	else if ( props.typeclass == TypeClass::SCRIPT )
	{
		typeclass_alphabet = MathAlphabet::SCRIPT;
	}

	std::optional<Slant> slant = props.slant;
	if ( !slant && math_intent && !bold && !typeclass_alphabet )
	{
		slant = Slant::MATH_ITALIC;
	}

	if ( slant == Slant::MATH_ITALIC )
	{
		return bold ? MathAlphabet::BOLD_ITALIC : MathAlphabet::ITALIC;
	}
	if ( !slant && math_intent )
	{
		if ( typeclass_alphabet )
		{
			return *typeclass_alphabet;
		}
		if ( bold )
		{
			return MathAlphabet::BOLD;
		}
	}
	return MathAlphabet::UPRIGHT;
}
