
#include "Utf8.h"
#include "FontStack.h"
#include "FontFace.h"
#include "Group.h"
#include "Stage.h"
#include "Theme.h"
#include "TextStyle.h"
#include "Bounds.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <hb.h>
#include <hb-ft.h>

#include <vector>
#include <string_view>
#include <algorithm>
#include <optional>

struct TextRange
{
	size_t font_index = 0;
	size_t start = 0;
	size_t length = 0;
};

std::vector<TextRange> create_ranges( std::string_view str, const FontStack &font )
{
	std::vector<TextRange> ranges;
	if ( str.empty() ) return ranges;

	std::string_view remaining = str;
	std::optional<size_t> current_face;
	size_t start_offset = 0;

	while ( !remaining.empty() )
	{
		size_t offset = str.size() - remaining.size();
		char32_t codepoint = decode_utf8( remaining );
		auto entry = font.lookup_codepoint( codepoint );

		if ( !current_face.has_value() )
		{
			current_face = entry.font_index;
		}
		if ( entry.font_index != current_face )
		{
			ranges.push_back( { entry.font_index, start_offset, offset - start_offset } );
			current_face = entry.font_index;
			start_offset = offset;
		}
	}

	if ( start_offset < str.size() )
	{
		ranges.push_back( { current_face.value(), start_offset, str.size() } );
	}
	return ranges;
}

void center_group( Group &group)
{
	BoundingBoxVisitor visitor;
	group.accept( visitor );

	Point center = { ( visitor.bounds.min.x + visitor.bounds.max.x ) / 2.0f,
					 ( visitor.bounds.min.y + visitor.bounds.max.y ) / 2.0f };

	for ( auto &v : group )
	{
		v->position.x -= center.x / v->scale_x;
		v->position.y -= center.y / v->scale_y;
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

	FT_Outline_Funcs funcs =
	{
		[](const FT_Vector* to, void* user) -> int
		{
			auto* g = static_cast<Glyph*>(user);
			g->commands.push_back({ PathCommand::MoveTo, { static_cast<float>(to->x), static_cast<float>(to->y) } });
			return 0;
		},
		[](const FT_Vector* to, void* user) -> int
		{
			auto* g = static_cast<Glyph*>(user);
			g->commands.push_back({ PathCommand::LineTo, { static_cast<float>(to->x), static_cast<float>(to->y) } });
			return 0;
		},
		[](const FT_Vector* control, const FT_Vector* to, void* user) -> int
		{
			auto* g = static_cast<Glyph*>(user);
			g->commands.push_back({ PathCommand::QuadTo, { static_cast<float>(to->x), static_cast<float>(to->y) }, { static_cast<float>(control->x), static_cast<float>(control->y) } });
			return 0;
		},
		[](const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user) -> int
		{
			auto* g = static_cast<Glyph*>(user);
			g->commands.push_back({ PathCommand::CubicTo, { static_cast<float>(to->x), static_cast<float>(to->y) }, { static_cast<float>(control1->x), static_cast<float>(control1->y) }, { static_cast<float>(control2->x), static_cast<float>(control2->y) } });
			return 0;
		}, 0, 0
	};

	FT_Outline_Decompose( &face->glyph->outline, &funcs, glyph.get() );
	return glyph;
}

Group &create_text( Stage &stage, const std::string &text, const TextStyle &style )
{
	constexpr float internal_scale = 60.F;

	auto &group = stage.visuals.add( std::make_unique<Group>() );

	FontStack stack = stage.theme.get_stack( style );

	std::vector<hb_font_t *> hb_fonts;
	{
		hb_fonts.resize( stack.size(), nullptr );
		auto make_hb_font = []( const FontFace &font ) -> hb_font_t *
		{
			const int upem = font.face->units_per_EM;
			auto *hb_font = hb_ft_font_create( font.face.get(), nullptr );
			hb_font_set_scale( hb_font, upem, upem);
			return hb_font;
		};
		std::transform( stack.begin(), stack.end(), hb_fonts.begin(), make_hb_font );
	}

	auto ranges = create_ranges( text, stack );

	hb_buffer_t *buffer = hb_buffer_create();
	for ( auto &range: ranges )
	{
		hb_buffer_clear_contents( buffer );
		int size = static_cast<int>( text.size() );
		unsigned int start = static_cast<unsigned int>( range.start );
		int length = static_cast<int>( range.length );
		hb_buffer_add_utf8( buffer, text.c_str(), size, start, length );
		hb_buffer_guess_segment_properties( buffer );

		hb_shape( hb_fonts.at( range.font_index ), buffer, NULL, 0 );

		const auto &font_face = stack.at( range.font_index );
		const int upem = font_face.face->units_per_EM;

		unsigned int glyph_count;
		hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos( buffer, &glyph_count );
		hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions( buffer, &glyph_count );

		const float font_size = font_face.scale;
		const float glyph_scale = font_size * internal_scale / static_cast<float>(upem);
		float current_x = 0.0f;
		float current_y = 0.0f;

		for ( unsigned int i = 0; i < glyph_count; ++i )
		{
			hb_codepoint_t glyph_id = glyph_info[i].codepoint;
			hb_glyph_position_t pos = glyph_pos[i];

			auto glyph = create_glyph( glyph_id, &font_face );
			glyph->scale_x = glyph_scale;
			glyph->scale_y = glyph_scale;
			glyph->position = Point{ current_x + static_cast<float>(pos.x_offset), current_y + static_cast<float>(pos.y_offset) };
			group.add( std::move( glyph ) );

			current_x += static_cast<float>(pos.x_advance);
			current_y += static_cast<float>(pos.y_advance);
		}
	}

	hb_buffer_destroy( buffer );

	for ( auto hb_font: hb_fonts )
	{
		hb_font_destroy( hb_font );
	}

	group *= 1.F / internal_scale;
	center_group( group );

	return group;
}

