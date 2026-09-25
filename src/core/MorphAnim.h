#pragma once

#include "Scene.h"
#include "Stage.h"
#include "Wait.h"

#include <visuals/Morph.h>

#include <memory>
#include <stdexcept>

// Play an existing Morph from t = 0 to t = 1 over `frames`, shaped by an ease function
// (ease_cubic_in_out, ...). The morph's visual must already be on the stage.
template<typename Ease>
Scene morph_to( Morph &morph, int64_t frames, Ease &&ease )
{
	auto anim = ease( 0.F, 1.F, frames );
	while ( anim.next() )
	{
		morph.set( anim.value() );
		co_await next_frame();
	}
	morph.set( 1.F );
}

// Turn `from` into `to` on the stage, with `drive( morph )` (a Scene that calls morph.set() as it goes: morph_to,
// or something tied to the narration) deciding when. `from` must be a direct child of stage.visuals; it is taken
// off the stage for the morph (and dies with it, so do not use the reference afterwards), and `to` takes its place
// when the drive finishes, so the last frame is the real thing, not a flattened copy.
template<typename Drive>
Scene morph_driven( Stage &stage, Visual &from, std::unique_ptr<Visual> to, MorphOptions options, Drive &&drive )
{
	Morph shape( from, *to, options );
	std::unique_ptr<Visual> drawn = shape.release();
	Visual *drawn_ptr = drawn.get();

	std::unique_ptr<Visual> original = stage.visuals.replace( &from, std::move( drawn ) );
	if ( !original )
	{
		throw std::invalid_argument( "morph: 'from' must be a direct child of stage.visuals" );
	}

	co_await drive( shape );

	stage.visuals.replace( drawn_ptr, std::move( to ) );
}

// Turn `from` into `to` over `frames`, shaped by an ease function (see morph_driven for what happens to the visuals)
template<typename Ease>
Scene morph(
	Stage &stage,
	Visual &from,
	std::unique_ptr<Visual> to,
	int64_t frames,
	Ease &&ease,
	MorphOptions options = {} )
{
	co_await morph_driven(
		stage, from, std::move( to ), options, [&]( Morph &shape ) { return morph_to( shape, frames, ease ); } );
}
