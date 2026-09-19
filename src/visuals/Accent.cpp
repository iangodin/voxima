#include "visuals/Accent.h"
#include "visuals/CreateText.h"
#include "visuals/MathMetrics.h"
#include <algorithm>

Accent::Accent( const MathFont &font, float s, std::unique_ptr<Visual> &&base, char32_t accent )
{
	// Baseline of the base to y = 0 and its left edge to x = 0, keeping track of where it
	// wants the accent centered
	const ScriptBox b = measure_box( *base );
	const float dx = -b.box.min.x();
	const float attach_x = top_accent_x( *base ).value_or( ( b.box.min.x() + b.box.max.x() ) / 2.F ) + dx;
	base->position = base->position.movedBy( dx, -b.baseline );

	const unsigned int index = font.get_glyph_index( accent );
	auto glyph = create_glyph( index, &font.face() );
	glyph->topAccentAttachment = font.top_accent_attachment( index );
	*glyph *= s;

	// Center its attachment point over the base's, and raise it if the base is taller than
	// the height the accent glyph is drawn for
	const float raise = std::max( 0.F, b.height() - font.accent_base_height() * s );
	glyph->position = { attach_x - *glyph->topAccentAttachment, raise };

	add( std::move( base ) );
	add( std::move( glyph ) );

	math_axis = font.axis_height() * s;

	// Like TeX, the box is as wide as the base; the accent may overhang it
	AABB box = layout_extent( *this );
	box.min = { 0.F, box.min.y() };
	box.max = { b.box.width(), box.max.y() };
	extent = box;
}
