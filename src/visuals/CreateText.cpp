#include <visuals/Bounds.h>
#include <visuals/CreateText.h>
#include <visuals/CreateStyledText.h>
#include <visuals/MissingGlyph.h>
#include <visuals/StyledText.h>
#include <visuals/Group.h>

#include <fonts/FontFace.h>
#include <fonts/FontStack.h>
#include <fonts/FontStyle.h>

#include <core/Theme.h>

#include <utils/Utf8.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <hb-ft.h>
#include <hb.h>

#include <optional>
#include <string_view>
#include <vector>

std::vector<TextRange> create_ranges( std::string_view str, const FontStack &font )
{
	std::vector<TextRange> ranges;
	if ( str.empty() )
	{
		return ranges;
	}

	std::string_view remaining = str;
	std::optional<size_t> current_face;
	size_t start_offset = 0;

	while ( !remaining.empty() )
	{
		size_t offset = str.size() - remaining.size();
		char32_t codepoint = decode_utf8( remaining );
		// A character that will be drawn as a marker (no font has it, or it is U+FFFD) gets a range of its own kind
		const size_t face = needs_marker( font, codepoint ) ? TextRange::MISSING : font.lookup_codepoint( codepoint ).font_index;

		if ( !current_face.has_value() )
		{
			current_face = face;
		}

		if ( face != current_face )
		{
			ranges.push_back( { current_face.value(), start_offset, offset - start_offset } );
			current_face = face;
			start_offset = offset;
		}
	}

	if ( current_face.has_value() )
	{
		ranges.push_back( { current_face.value(), start_offset, str.size() - start_offset } );
	}
	return ranges;
}

void center_group( Group &group )
{
	BoundingBoxVisitor visitor;
	group.accept( visitor );

	Point center = {
		( visitor.bounds.min.x() + visitor.bounds.max.x() ) / 2.0f,
		( visitor.bounds.min.y() + visitor.bounds.max.y() ) / 2.0f };

	for ( auto &v: group )
	{
		v->position = v->position.movedBy( -center.x() / v->scale_x, -center.y() / v->scale_y );
	}
	group.position = { 0.0f, 0.0f };
}

std::unique_ptr<Glyph> create_glyph( unsigned int glyph_id, const FontFace *font_face )
{
	auto glyph = std::make_unique<Glyph>();
	glyph->font_face = font_face;
	glyph->index = glyph_id;

	FT_Face face = font_face->face.get();

	if ( FT_Load_Glyph( face, glyph_id, FT_LOAD_NO_SCALE ) != 0 )
	{
		return glyph;
	}

	FT_Outline_Funcs funcs = {
		[]( const FT_Vector *to, void *user ) -> int
		{
			auto *g = static_cast<Glyph *>( user );
			g->commands.push_back(
				MoveTo { { static_cast<float>( to->x ), static_cast<float>( to->y ) } } );
			return 0;
		},
		[]( const FT_Vector *to, void *user ) -> int
		{
			auto *g = static_cast<Glyph *>( user );
			g->commands.push_back(
				LineTo { { static_cast<float>( to->x ), static_cast<float>( to->y ) } } );
			return 0;
		},
		[]( const FT_Vector *control, const FT_Vector *to, void *user ) -> int
		{
			auto *g = static_cast<Glyph *>( user );
			g->commands.push_back(
				QuadTo { { static_cast<float>( to->x ), static_cast<float>( to->y ) },
		                 { static_cast<float>( control->x ), static_cast<float>( control->y ) } } );
			return 0;
		},
		[]( const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *user )
			-> int
		{
			auto *g = static_cast<Glyph *>( user );
			g->commands.push_back(
				CubicTo { { static_cast<float>( to->x ), static_cast<float>( to->y ) },
		                  { static_cast<float>( control1->x ), static_cast<float>( control1->y ) },
		                  { static_cast<float>( control2->x ), static_cast<float>( control2->y ) } } );
			return 0;
		},
		0,
		0 };

	FT_Outline_Decompose( &face->glyph->outline, &funcs, glyph.get() );
	glyph->commands.push_back( Close {} );

	glyph->bearingX = static_cast<float>( face->glyph->metrics.horiBearingX );
	glyph->bearingY = static_cast<float>( face->glyph->metrics.horiBearingY );
	glyph->advanceX = static_cast<float>( face->glyph->metrics.horiAdvance );
	glyph->advanceY = static_cast<float>( face->glyph->metrics.vertAdvance );

	// Layout box: the advance width, and the glyph's own height and depth
	const float height = static_cast<float>( face->glyph->metrics.horiBearingY );
	const float depth = static_cast<float>( face->glyph->metrics.height - face->glyph->metrics.horiBearingY );
	AABB box;
	box.add( { 0.F, -depth } );
	box.add( { glyph->advanceX, height } );
	glyph->extent = box;

	return glyph;
}

std::unique_ptr<Group> make_text(
	Theme &theme,
	const std::string &text,
	const FontStyle &style,
	const std::optional<std::string> &language,
	const TextLayout &layout )
{
	StyledText st( text );
	st.set_font_style( 0, st.size(), style );
	if ( language )
	{
		st.set_language( 0, st.size(), *language );
	}
	return make_styled_text( theme, st, layout );
}
