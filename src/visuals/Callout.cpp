#include "visuals/Callout.h"

#include "core/Stage.h"
#include "visuals/Placement.h"

Callout::Callout( std::unique_ptr<Visual> content, const Visual &target, const CalloutOptions &options ) :
	_target( &target ),
	_stand_in( std::make_unique<Rectangle>() )
{
	_panel = &add( make_panel( std::move( content ), options.panel ) );
	// (it starts a gap from the panel's edge, so it does not matter that it is drawn over it)
	_pointer = &add( std::make_unique<Connector>( *_panel, *_stand_in, options.pointer ) );
	update();
}

void Callout::update()
{
	const AABB box = box_of( *_target );
	if ( box.empty() )
	{
		return;
	}
	// In this group's frame
	_stand_in->half_width = ( box.max.x() - box.min.x() ) / 2.F;
	_stand_in->half_height = ( box.max.y() - box.min.y() ) / 2.F;
	_stand_in->position = Point{ ( box.min.x() + box.max.x() ) / 2.F, ( box.min.y() + box.max.y() ) / 2.F } - position;
	_panel->fit();
	_pointer->update();
}

std::unique_ptr<Callout> make_callout( std::unique_ptr<Visual> content, const Visual &target, const CalloutOptions &options )
{
	return std::make_unique<Callout>( std::move( content ), target, options );
}

Updaters::Handle follow( Stage &stage, Callout &callout )
{
	return stage.updaters.add( [&callout] { callout.update(); } );
}
