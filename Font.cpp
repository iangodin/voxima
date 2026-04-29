
#include "Font.h"
#include "MFLFontFace.h"
#include "Glyph.h"
#include "Bounds.h"

#include <mfl/font_family.hpp>
#include <mfl/layout.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <hb.h>
#include <hb-ft.h>

#include <iostream>
#include <cstring>

constexpr float internal_scale = 60.F;

Font::Font( const std::string &path ) :
	_path(path),
	_ft(),
	_face( std::make_unique<MFLFontFace>(mfl::font_family::sans, _ft) )
{
}

Font::~Font()
{
}

std::unique_ptr<Group> Font::create_text( const std::string &text, float font_size ) const
{
	auto group = std::make_unique<Group>();
	FT_Face ft_face = _ft.face( mfl::font_family::sans );

	hb_font_t *hb_font = hb_ft_font_create( ft_face, nullptr );

	hb_buffer_t* buffer = hb_buffer_create();
	hb_buffer_add_utf8( buffer, text.c_str(), -1, 0, -1 );
	hb_buffer_guess_segment_properties( buffer );

	int upem = ft_face->units_per_EM;
	hb_font_set_scale( hb_font, upem, upem);
	hb_shape( hb_font, buffer, NULL, 0 );

	unsigned int glyph_count;
	hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos( buffer, &glyph_count );
	hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions( buffer, &glyph_count );

	float glyph_scale = font_size * internal_scale / static_cast<float>(upem);
	float current_x = 0.0f;
	float current_y = 0.0f;

	for ( unsigned int i = 0; i < glyph_count; ++i )
	{
		hb_codepoint_t glyph_id = glyph_info[i].codepoint;
		hb_glyph_position_t pos = glyph_pos[i];

		auto glyph = create_glyph( glyph_id, mfl::font_family::sans );
		glyph->scale_x = glyph_scale;
		glyph->scale_y = glyph_scale;
		glyph->position = Point{ current_x + static_cast<float>(pos.x_offset), current_y + static_cast<float>(pos.y_offset) };
		group->add( std::move( glyph ) );

		current_x += static_cast<float>(pos.x_advance);
		current_y += static_cast<float>(pos.y_advance);
	}

	hb_buffer_destroy( buffer );
	hb_font_destroy( hb_font );

	*group *= 1.F / internal_scale;
	center_group( *group );
	return group;
}

std::unique_ptr<Group> Font::create_math( const std::string& formula, float font_size ) const
{
	auto group = std::make_unique<Group>();

	// 1. Generate the layout
	// MFL uses its own font_face_creator; we provide a lambda that uses our Freetype manager
	auto layout = mfl::layout(formula, mfl::points(font_size * internal_scale), [this](mfl::font_family family) {
		return std::make_unique<MFLFontFace>(family, _ft);
	});

	if (layout.error) {
		std::cerr << "MFL Layout Error: " << *layout.error << std::endl;
		return group;
	}

	// 2. Process Glyphs
	for (const auto& g : layout.glyphs)
	{
		// The MFL 'size' is the font size in points for this specific glyph (e.g. script size)
		// To get the correct scale for our path, we normalize by the font's UPEM.
		FT_Face face = _ft.face(g.family);
		float upem = static_cast<float>(face->units_per_EM);

		float glyph_scale = static_cast<float>(g.size.value()) / upem;
		auto glyph = create_glyph( static_cast<unsigned int>(g.index), g.family );
		glyph->scale_x = glyph_scale;
		glyph->scale_y = glyph_scale;

		// MFL points are now 1:1 with your world units
		// We need to unscale them by the glyph_scale and rescale by internal_scale?
		// Wait, MFL's g.x and g.y are already in the "internal scaled world" (because we passed font_size * internal_scale).
		// So g.x and g.y are already at the correct position for internal_scale.
		// But remember T = Pos / Scale.
		// Here Pos = g.x. Scale = glyph_scale.
		// So T = g.x / glyph_scale.
		glyph->position = Point{
			static_cast<float>(g.x.value()) / glyph_scale,
			static_cast<float>(g.y.value()) / glyph_scale
		};

		group->add(std::move(glyph));
	}

	// 3. Process Lines (Fraction bars, radicals, etc.)
	for (const auto& line : layout.lines)
	{
		const float hw = static_cast<float>(line.length.value()) / 2.F;
		const float hh = static_cast<float>(line.thickness.value()) / 2.F;
		const float x = static_cast<float>(line.x.value()) + hw;
		const float y = static_cast<float>(line.y.value()) + hh;

		auto rect = std::make_unique<Rectangle>();
		rect->half_width = hw;
		rect->half_height = hh;
		rect->position = Point{ x, y };
		group->add(std::move(rect));
	}

	*group *= 1.F / internal_scale;
	center_group( *group );
	return group;
}

std::unique_ptr<Glyph> Font::create_glyph(unsigned int glyph_id, mfl::font_family family ) const
{
	auto glyph = std::make_unique<Glyph>();
	glyph->font_face = reinterpret_cast<const FontFace *>( static_cast<intptr_t>( family ) );
	glyph->index = glyph_id;

	FT_Face face = _ft.face( family );

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
		}, 0, 0 };
	FT_Outline_Decompose( &face->glyph->outline, &funcs, glyph.get() );
	return glyph;
}

void Font::center_group( Group &group) const
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

