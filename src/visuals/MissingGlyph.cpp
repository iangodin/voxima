#include "visuals/MissingGlyph.h"

#include "visuals/CreateText.h"
#include "visuals/Tags.h"

#include <algorithm>
#include <cstdio>
#include <sstream>

namespace
{

const Color MAGENTA{ 1.F, 0.F, 1.F, 1.F };

// The keyline round a marker, as a fraction of the font's size
constexpr float KEYLINE = 0.03F;

std::string hex_code( char32_t codepoint )
{
	char buffer[16];
	std::snprintf( buffer, sizeof( buffer ), "%04X", static_cast<unsigned>( codepoint ) );
	return buffer;
}

Point moved( Point p, float scale, Point offset )
{
	return { p.x() * scale + offset.x(), p.y() * scale + offset.y() };
}

// A digit's outline, scaled and moved into place
void append_transformed( std::vector<PathCommand> &out, const std::vector<PathCommand> &in, float scale, Point offset )
{
	for ( const auto &command: in )
	{
		std::visit(
			[&]( const auto &cmd )
			{
				using T = std::decay_t<decltype( cmd )>;
				if constexpr ( std::is_same_v<T, MoveTo> )
				{
					out.push_back( MoveTo{ moved( cmd.to, scale, offset ) } );
				}
				else if constexpr ( std::is_same_v<T, LineTo> )
				{
					out.push_back( LineTo{ moved( cmd.to, scale, offset ) } );
				}
				else if constexpr ( std::is_same_v<T, QuadTo> )
				{
					out.push_back( QuadTo{ moved( cmd.to, scale, offset ), moved( cmd.ctrl1, scale, offset ) } );
				}
				else if constexpr ( std::is_same_v<T, CubicTo> )
				{
					out.push_back( CubicTo{ moved( cmd.to, scale, offset ), moved( cmd.ctrl1, scale, offset ), moved( cmd.ctrl2, scale, offset ) } );
				}
				else
				{
					out.push_back( Close{} );
				}
			},
			command );
	}
}

std::vector<MissingGlyphReport> &records()
{
	static std::vector<MissingGlyphReport> list;
	return list;
}

}

bool needs_marker( const FontStack &stack, char32_t codepoint )
{
	if ( codepoint == REPLACEMENT_CHARACTER )
	{
		return true;
	}
	const auto entry = stack.find_codepoint( codepoint );
	return !entry || stack.font( entry->font_index ).notdef;
}

std::unique_ptr<Glyph> create_missing_glyph( char32_t codepoint, const FontFace &font )
{
	const float em = static_cast<float>( font.face->units_per_EM );

	// The digits of the code, from the font itself
	std::vector<std::unique_ptr<Glyph>> digits;
	bool have_digits = true;
	for ( char c: hex_code( codepoint ) )
	{
		const FT_UInt index = FT_Get_Char_Index( font.face.get(), static_cast<FT_ULong>( c ) );
		if ( index == 0 )
		{
			have_digits = false;
			break;
		}
		digits.push_back( create_glyph( index, &font ) );
	}

	float digit_height = 0.F;
	for ( const auto &d: digits )
	{
		digit_height = std::max( digit_height, d->bearingY );
	}
	have_digits = have_digits && digit_height > 0.F;

	const float scale = have_digits ? 0.32F * em / digit_height : 0.F;
	float digits_width = 0.F;
	if ( have_digits )
	{
		for ( const auto &d: digits )
		{
			digits_width += d->advanceX * scale;
		}
	}

	// The frame: as wide as the code needs, and about the height of a letter with room above and below
	const float frame = 0.06F * em;
	const float pad = 0.08F * em;
	const float inner_width = have_digits ? std::max( 0.5F * em, digits_width + 2.F * pad ) : 0.5F * em;
	const float x0 = 0.05F * em;
	const float x1 = x0 + inner_width + 2.F * frame;
	const float y0 = -0.12F * em;
	const float y1 = 0.78F * em;

	auto glyph = std::make_unique<Glyph>();
	glyph->font_face = &font;
	glyph->color = MAGENTA;

	// A keyline in "white", which is the theme's text color (see Color::resolved): the color every palette has to show against
	// its background. So the marker stands out from a magenta background as much as a white one, and from any neighbour
	// that is magenta, and is never lost because of what it is drawn on. (Its thickness is in stage units: a glyph's
	// stroke is not scaled with it.)
	glyph->stroke_color = Color{ 1.F, 1.F, 1.F, 1.F };
	glyph->stroke_thickness = KEYLINE * font.scale;
	tag_visual( *glyph, MISSING_GLYPH_TAG );

	// The frame is a box with a box cut out. The two wind opposite ways, which is what makes a hole; the
	// digits are then drawn inside it, and fill whichever way their own outlines wind.
	auto &c = glyph->commands;
	c.push_back( MoveTo{ { x0, y0 } } );
	c.push_back( LineTo{ { x1, y0 } } );
	c.push_back( LineTo{ { x1, y1 } } );
	c.push_back( LineTo{ { x0, y1 } } );
	c.push_back( Close{} );
	c.push_back( MoveTo{ { x0 + frame, y0 + frame } } );
	c.push_back( LineTo{ { x0 + frame, y1 - frame } } );
	c.push_back( LineTo{ { x1 - frame, y1 - frame } } );
	c.push_back( LineTo{ { x1 - frame, y0 + frame } } );
	c.push_back( Close{} );

	if ( have_digits )
	{
		float x = x0 + frame + ( inner_width - digits_width ) / 2.F;
		const float base = ( y0 + y1 ) / 2.F - digit_height * scale / 2.F;
		for ( const auto &d: digits )
		{
			append_transformed( c, d->commands, scale, { x, base } );
			x += d->advanceX * scale;
		}
	}

	glyph->bearingX = x0;
	glyph->bearingY = y1;
	glyph->advanceX = x1 + 0.05F * em;
	glyph->advanceY = 0.F;
	AABB box;
	box.add( { 0.F, y0 } );
	box.add( { glyph->advanceX, y1 } );
	glyph->extent = box;
	return glyph;
}

void report_missing_glyph( char32_t codepoint, std::string_view context )
{
	for ( auto &entry: records() )
	{
		if ( entry.codepoint == codepoint && entry.context == context )
		{
			++entry.count;
			return;
		}
	}
	records().push_back( { codepoint, std::string( context ), 1 } );
}

const std::vector<MissingGlyphReport> &missing_glyph_reports()
{
	return records();
}

void clear_missing_glyph_reports()
{
	records().clear();
}

std::string describe_missing_glyphs()
{
	if ( records().empty() )
	{
		return {};
	}

	size_t total = 0;
	for ( const auto &entry: records() )
	{
		total += entry.count;
	}

	std::ostringstream out;
	out << "WARNING: " << total << " missing glyph" << ( total == 1 ? "" : "s" )
		<< " (drawn as magenta boxes in the video):\n";
	for ( const auto &entry: records() )
	{
		constexpr size_t LIMIT = 60;
		std::string context = entry.context.substr( 0, LIMIT );
		if ( entry.context.size() > LIMIT )
		{
			context += "...";
		}
		out << "  U+" << hex_code( entry.codepoint ) << ( entry.count > 1 ? " x" + std::to_string( entry.count ) : "" ) << "  in \"" << context << "\"\n";
	}
	return out.str();
}
