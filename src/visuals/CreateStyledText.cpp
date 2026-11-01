#include <visuals/CreateStyledText.h>
#include <visuals/CreateText.h>
#include <visuals/Group.h>
#include <visuals/TextLine.h>
#include <visuals/Glyph.h>
#include <visuals/Bounds.h>
#include <visuals/MissingGlyph.h>
#include <visuals/Tags.h>

#include <fonts/FontFace.h>
#include <fonts/FontStack.h>
#include <fonts/FontStyle.h>
#include <fonts/MathAlphabet.h>

#include <core/Theme.h>

#include <utils/Words.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb-ft.h>
#include <hb-ot.h>
#include <hb.h>

#include <algorithm>
#include <map>
#include <string>

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

// Math alphabets swap each code point for another (usually a longer UTF-8
// sequence), so a cluster's byte offset in the shaped text is not one in the
// original. Both strings hold the same code points in the same order.
size_t original_offset( std::string_view shaped, std::string_view original, size_t cluster )
{
	const size_t codepoints = codepoint_count( shaped.substr( 0, cluster ) );
	size_t offset = 0;
	for ( size_t i = 0; i < codepoints && offset < original.size(); ++i )
	{
		std::string_view rest = original.substr( offset );
		const size_t before = rest.size();
		decode_utf8( rest );
		offset += before - rest.size();
	}
	return offset;
}

// A shaped glyph and where in the text it came from
struct PlacedGlyph
{
	size_t offset; // byte offset of its cluster in StyledText::text()
	std::unique_ptr<Glyph> glyph;
};

}

std::unique_ptr<Group> make_styled_text( Theme &theme, const StyledText &text, const TextLayout &layout )
{
	auto root = std::make_unique<Group>();
	make_styled_text( *root, theme, text, layout, nullptr );
	return root;
}

void make_styled_text( Group &root_group, Theme &theme, const StyledText &text, const TextLayout &layout, std::vector<PlacedGlyphInfo> *glyphs )
{
	Group *root = &root_group;

	HbFontCache font_cache;
	hb_buffer_t *buffer = hb_buffer_create();

	// The first line's baseline is at y = 0 and the origin is at the start of the line;
	// further lines go below. Callers position (or center) the result as they see fit.
	float current_y = 0.0f;
	float previous_em = 0.5f; // font size of the line above, for blank lines
	std::vector<TextLine *> line_groups;
	std::vector<float> line_widths;

	const std::string &full_text = text.text();
	TextStyle default_style;
	size_t word_counter = 0; // words so far, across lines (see "word<N>" tags below)

	for ( const auto &line_view: text.lines() )
	{
		float current_x = 0.0f;
		size_t line_start = static_cast<size_t>( line_view.data() - full_text.data() );
		size_t line_end = line_start + line_view.size();

		auto &line_group = root->add( std::make_unique<TextLine>() );
		line_group.baseline = current_y;
		float line_em = 0.0f; // largest font size on this line

		std::vector<PlacedGlyph> placed;

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

				// Math italic (and the other math alphabets) are separate code points
				const std::string_view original_text = run_text;
				std::string mapped;
				if ( const auto alphabet = math_alphabet( style.font_style ); alphabet != MathAlphabet::UPRIGHT )
				{
					mapped = map_math_text( run_text, alphabet );
					run_text = mapped;
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

					// Override guess_segment_properties' language (which falls
					// back to the process locale, not the text's actual known
					// language) the same way bidi_level already overrides its
					// guessed direction.
					if ( style.language )
					{
						hb_buffer_set_language(
							buffer, hb_language_from_string( style.language->c_str(), -1 ) );
					}

					// Characters no font draws (and U+FFFD) become loud magenta boxes with their code in them
					if ( range.font_index == TextRange::MISSING )
					{
						const auto &marker_font = stack.font( 0 );
						line_em = std::max( line_em, marker_font.scale );
						const float marker_scale = marker_font.scale / static_cast<float>( marker_font.face->units_per_EM );

						std::string_view rest = run_text.substr( range.start, range.length );
						while ( !rest.empty() )
						{
							const size_t at = run_text.size() - rest.size();
							const char32_t codepoint = decode_utf8( rest );

							auto marker = create_missing_glyph( codepoint, marker_font );
							marker->position = Point{ current_x / marker_scale, current_y / marker_scale };
							*marker *= marker_scale;
							const float advance = marker->advanceX;

							placed.push_back( PlacedGlyph{
								run_start + ( mapped.empty() ? at : original_offset( run_text, original_text, at ) ),
								std::move( marker ) } );
							current_x += advance;
							report_missing_glyph( codepoint, line_view );
						}
						continue;
					}

					const auto &font_face = stack.font( range.font_index );
					line_em = std::max( line_em, font_face.scale );
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

						// Math fonts say how far a superscript should clear each glyph
						hb_font_t *hb_font = font_cache.get( font_face );
						if ( hb_ot_math_has_data( hb_font_get_face( hb_font ) ) )
						{
							glyph->italicCorrection = static_cast<float>(
								hb_ot_math_get_glyph_italics_correction( hb_font, glyph_id ) );
							glyph->topAccentAttachment = static_cast<float>(
								hb_ot_math_get_glyph_top_accent_attachment( hb_font, glyph_id ) );
						}
						if ( glyph->extent )
						{
							// Shaped advance (includes kerning) replaces the nominal one
							glyph->extent->max = { glyph->advanceX, glyph->extent->max.y() };
						}

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

						const size_t cluster = glyph_info[i].cluster;
						placed.push_back( PlacedGlyph{
							run_start + ( mapped.empty() ? cluster : original_offset( run_text, original_text, cluster ) ),
							std::move( glyph ) } );

						current_x += static_cast<float>( pos.x_advance ) * glyph_scale;
						current_y += static_cast<float>( pos.y_advance ) * glyph_scale;
					}
				}
			} );

		// Gather each word's glyphs into a Group tagged "word<N>", N counting
		// words through the whole text (split_words(), the same splitter the
		// narration timing uses, so word N here is spoken word N). Glyph
		// positions are already absolute within the line and the word groups sit
		// at the origin, so this doesn't move anything. Glyphs outside any word
		// (spaces, lone punctuation) stay directly in the line. A word is
		// gathered by logical position, so an RTL word's glyphs are found even
		// though they are laid out in reverse.
		const auto spans = split_words( line_view );
		std::vector<Group *> word_groups( spans.size(), nullptr );
		for ( auto &item: placed )
		{
			if ( glyphs )
			{
				glyphs->push_back( { item.offset, item.glyph.get() } );
			}
			const size_t local = item.offset - line_start;
			auto it = std::upper_bound(
				spans.begin(), spans.end(), local, []( size_t value, const WordSpan &span ) { return value < span.begin; } );
			const size_t k = static_cast<size_t>( it - spans.begin() );
			if ( k > 0 && local < spans[k - 1].end )
			{
				Group *&group = word_groups[k - 1];
				if ( !group )
				{
					group = &line_group.add( std::make_unique<Group>() );
					tag_visual( *group, word_tag( word_counter + k - 1 ) );
				}
				group->add( std::move( item.glyph ) );
			}
			else
			{
				line_group.add( std::move( item.glyph ) );
			}
		}
		word_counter += spans.size();

		line_groups.push_back( &line_group );
		line_widths.push_back( current_x );

		if ( line_em <= 0.0f )
		{
			line_em = previous_em;
		}
		previous_em = line_em;
		current_y -= line_em * layout.line_spacing;
	}

	hb_buffer_destroy( buffer );

	// Align the lines within the widest one. Glyph positions are relative to their line, so
	// moving the line moves everything on it.
	if ( layout.align != TextAlign::LEFT )
	{
		const float block_width = *std::max_element( line_widths.begin(), line_widths.end() );
		for ( size_t i = 0; i < line_groups.size(); ++i )
		{
			const float slack = block_width - line_widths[i];
			const float shift = layout.align == TextAlign::CENTER ? slack / 2.0f : slack;
			line_groups[i]->position = { shift, 0.0f };
		}
	}
}
