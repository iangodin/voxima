#include <visuals/CreateStyledText.h>
#include <visuals/CreateText.h>
#include <visuals/Group.h>
#include <visuals/Glyph.h>
#include <visuals/Bounds.h>

#include <fonts/FontFace.h>
#include <fonts/FontStack.h>
#include <fonts/FontStyle.h>

#include <core/Stage.h>
#include <core/Theme.h>

#include <hb-ft.h>
#include <hb.h>

#include <map>

namespace
{

struct HbFontCache
{
	std::map<const FontFacePtr, hb_font_t *> cache;

	hb_font_t *get( const FontFace &font )
	{
		auto it = cache.find( font.face );
		if ( it != cache.end() )
		{
			return it->second;
		}

		const int upem = font.face->units_per_EM;
		auto *hb_font = hb_ft_font_create( font.face.get(), nullptr );
		hb_font_set_scale( hb_font, upem, upem );
		cache[font.face] = hb_font;
		return hb_font;
	}

	~HbFontCache()
	{
		for ( auto [face, hb_font]: cache )
		{
			hb_font_destroy( hb_font );
		}
	}
};

}

Group &create_styled_text( Stage &stage, const StyledText &text )
{
	return create_styled_text( stage, stage.visuals, text );
}

Group &create_styled_text( Stage &stage, Group &group, const StyledText &text )
{
	Theme &theme = *stage.theme;
	auto &subgroup = group.add( std::make_unique<Group>() );

	HbFontCache font_cache;
	hb_buffer_t *buffer = hb_buffer_create();

	float current_y = 0.0f;
	const float line_height = 0.5f;

	const std::string &full_text = text.text();
	TextStyle default_style;

	for ( const auto &line_view: text.lines() )
	{
		float current_x = 0.0f;
		size_t line_start = static_cast<size_t>( line_view.data() - full_text.data() );
		size_t line_end = line_start + line_view.size();

		text.styles().for_each(
			line_start,
			line_end,
			default_style,
			[&]( size_t run_start, size_t run_end, const TextStyle &style )
			{
				std::string_view run_text( full_text.data() + run_start, run_end - run_start );
				if ( run_text.empty() )
				{
					return;
				}

				FontStack stack = theme.get_stack( style.font_style );
				auto ranges = create_ranges( run_text, stack );

				for ( auto &range: ranges )
				{
					hb_buffer_clear_contents( buffer );
					hb_buffer_add_utf8(
						buffer,
						run_text.data(),
						static_cast<int>( run_text.size() ),
						static_cast<unsigned int>( range.start ),
						static_cast<int>( range.length ) );
					hb_buffer_guess_segment_properties( buffer );

					if ( style.bidi_level )
					{
						hb_buffer_set_direction(
							buffer,
							( *style.bidi_level % 2 == 0 ) ? HB_DIRECTION_LTR : HB_DIRECTION_RTL );
					}

					const auto &font_face = stack.font( range.font_index );
					hb_shape( font_cache.get( font_face ), buffer, nullptr, 0 );

					unsigned int glyph_count;
					hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos( buffer, &glyph_count );
					hb_glyph_position_t *glyph_pos =
						hb_buffer_get_glyph_positions( buffer, &glyph_count );

					const int upem = font_face.face->units_per_EM;
					const float font_size = font_face.scale;
					const float glyph_scale = font_size / ( static_cast<float>( upem ) );

					for ( unsigned int i = 0; i < glyph_count; ++i )
					{
						hb_codepoint_t glyph_id = glyph_info[i].codepoint;
						hb_glyph_position_t pos = glyph_pos[i];

						auto glyph = create_glyph( glyph_id, &font_face );

						hb_glyph_extents_t extents;
						if ( hb_font_get_glyph_extents( font_cache.get( font_face ), glyph_id, &extents ) )
						{
							glyph->bearingX = static_cast<float>( extents.x_bearing );
							glyph->bearingY = static_cast<float>( extents.y_bearing );
						}
						glyph->advanceX = static_cast<float>( pos.x_advance );
						glyph->advanceY = static_cast<float>( pos.y_advance );

#ifdef DEBUG_GLYPH_METRICS
						// Debug shapes (Font Units)
						const float s = 50.0f;
						// 1) Diamond at origin
						glyph->commands.push_back( { PathCommand::MoveTo, { 0, -s } } );
						glyph->commands.push_back( { PathCommand::LineTo, { s, 0 } } );
						glyph->commands.push_back( { PathCommand::LineTo, { 0, s } } );
						glyph->commands.push_back( { PathCommand::LineTo, { -s, 0 } } );
						glyph->commands.push_back( { PathCommand::LineTo, { 0, -s } } );

						// 2) Triangle at bearing
						glyph->commands.push_back( { PathCommand::MoveTo, { glyph->bearingX, glyph->bearingY - s } } );
						glyph->commands.push_back( { PathCommand::LineTo, { glyph->bearingX + s, glyph->bearingY + s } } );
						glyph->commands.push_back( { PathCommand::LineTo, { glyph->bearingX - s, glyph->bearingY + s } } );
						glyph->commands.push_back( { PathCommand::LineTo, { glyph->bearingX, glyph->bearingY - s } } );

						// 3) Square at advance
						glyph->commands.push_back( { PathCommand::MoveTo, { glyph->advanceX - s, glyph->advanceY - s } } );
						glyph->commands.push_back( { PathCommand::LineTo, { glyph->advanceX + s, glyph->advanceY - s } } );
						glyph->commands.push_back( { PathCommand::LineTo, { glyph->advanceX + s, glyph->advanceY + s } } );
						glyph->commands.push_back( { PathCommand::LineTo, { glyph->advanceX - s, glyph->advanceY + s } } );
						glyph->commands.push_back( { PathCommand::LineTo, { glyph->advanceX - s, glyph->advanceY - s } } );
#endif

						glyph->position = Point{
							current_x / glyph_scale + static_cast<float>( pos.x_offset ),
							current_y / glyph_scale + static_cast<float>( pos.y_offset ) };

						if ( style.color )
						{
							glyph->color = *style.color;
						}

						*glyph *= glyph_scale;
						subgroup.add( std::move( glyph ) );

						current_x += static_cast<float>( pos.x_advance ) * glyph_scale;
						current_y += static_cast<float>( pos.y_advance ) * glyph_scale;
					}
				}
			} );

		current_y -= line_height;
	}

	hb_buffer_destroy( buffer );
	center_group( subgroup );

	return subgroup;
}
