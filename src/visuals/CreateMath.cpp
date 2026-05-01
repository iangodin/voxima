
#include "fonts/MFLFontFace.h"
#include "core/Stage.h"
#include "fonts/Theme.h"
#include "visuals/CreateMath.h"
#include "visuals/CreateText.h"
#include "visuals/Group.h"
#include "visuals/Rectangle.h"
#include "visuals/Glyph.h"
#include "visuals/Bounds.h"

#include <mfl/layout.hpp>
#include <iostream>

Group &create_math( Stage &stage, const std::string &formula, const TextStyle &style )
{
	return create_math( stage.visuals, stage.theme, formula, style );
}

Group &create_math( Group &group, Theme &theme, const std::string &formula, const TextStyle &style )
{
	auto &subgroup = group.add( std::make_unique<Group>() );
	MathFonts fonts = theme.get_math( style );

	float base_point_size = fonts.roman().scale * Theme::FONT_SCALE;

	auto layout = mfl::layout( formula, mfl::points( base_point_size ), [&]( mfl::font_family family ) {
		switch ( family )
		{
			case mfl::font_family::roman: return std::make_unique<MFLFontFace>( fonts.roman().face.get() );
			case mfl::font_family::italic: return std::make_unique<MFLFontFace>( fonts.italic().face.get() );
			case mfl::font_family::bold: return std::make_unique<MFLFontFace>( fonts.bold().face.get() );
			case mfl::font_family::sans: return std::make_unique<MFLFontFace>( fonts.sans().face.get() );
			case mfl::font_family::mono: return std::make_unique<MFLFontFace>( fonts.mono().face.get() );
		}
		return std::make_unique<MFLFontFace>( fonts.roman().face.get() );
	} );

	if ( layout.error )
	{
		std::cerr << "MFL Layout Error: " << *layout.error << std::endl;
		return subgroup;
	}

	for ( const auto &g : layout.glyphs )
	{
		const FontFace *selected_face = nullptr;
		switch ( g.family )
		{
			case mfl::font_family::roman: selected_face = &fonts.roman(); break;
			case mfl::font_family::italic: selected_face = &fonts.italic(); break;
			case mfl::font_family::bold: selected_face = &fonts.bold(); break;
			case mfl::font_family::sans: selected_face = &fonts.sans(); break;
			case mfl::font_family::mono: selected_face = &fonts.mono(); break;
		}

		FT_Face face = selected_face->face.get();
		float upem = static_cast<float>( face->units_per_EM );

		float glyph_scale = static_cast<float>( g.size.value() ) / ( upem * Theme::FONT_SCALE );
		auto glyph = create_glyph( static_cast<unsigned int>( g.index ), selected_face );
		glyph->position = Point{
			static_cast<float>( g.x.value() ) * upem / static_cast<float>( g.size.value() ),
			static_cast<float>( g.y.value() ) * upem / static_cast<float>( g.size.value() ) };

		*glyph *= glyph_scale;
		subgroup.add( std::move( glyph ) );
	}

	for ( const auto &line : layout.lines )
	{
		const float hw = static_cast<float>( line.length.value() ) / ( 2.F * Theme::FONT_SCALE );
		const float hh = static_cast<float>( line.thickness.value() ) / ( 2.F * Theme::FONT_SCALE );
		const float x = ( static_cast<float>( line.x.value() ) / Theme::FONT_SCALE ) + hw;
		const float y = ( static_cast<float>( line.y.value() ) / Theme::FONT_SCALE ) + hh;

		auto rect = std::make_unique<Rectangle>();
		rect->half_width = hw;
		rect->half_height = hh;
		rect->position = Point{ x, y };
		subgroup.add( std::move( rect ) );
	}

	center_group( subgroup );
	return subgroup;
}
