#pragma once

#include "core/Updaters.h"
#include "visuals/ArrowTip.h"
#include "visuals/Group.h"
#include "visuals/Line.h"

#include <memory>

struct ConnectorOptions
{
	// Space left between each end and the box it is attached to
	float gap = 0.1F;
	float thickness = 0.05F;

	// An arrow head at the end (the `to` side), and at the start
	bool arrow = true;
	bool arrow_at_start = false;
	float arrow_size = 0.15F; // half its width; it is twice that long

	// White is the theme's text color, as everywhere
	Color color = { 1.F, 1.F, 1.F, 1.F };
};

// A line, with arrow heads if wanted, from one visual to another: it starts where the straight line between the two boxes'
// centers leaves the first box, and ends where it enters the second (with a gap). The two visuals must be in the same
// frame as the connector (children of the same group, or of the stage).
//
//     auto &edge = stage.visuals.add( make_connector( a, b ) );
//     auto tie = follow( stage, edge );          // it goes on joining them as they move
//
// Move, scale or fade the Connector and it all goes with it. If the visuals moved, connect() again (or follow them).
class Connector : public Group
{
public:
	Connector( const Visual &from, const Visual &to, const ConnectorOptions &options );

	// Lay it out again from where the two visuals are now
	void update();

	const Line &line() const
	{
		return *_line;
	}

	// The head at the `to` end, and at the start (of no size when there is none)
	const ArrowTip &end_tip() const
	{
		return *_end_tip;
	}

	const ArrowTip &start_tip() const
	{
		return *_start_tip;
	}

	const ConnectorOptions &options() const
	{
		return _options;
	}

private:
	const Visual *_from;
	const Visual *_to;
	ConnectorOptions _options;
	Line *_line;
	ArrowTip *_end_tip;
	ArrowTip *_start_tip;
};

std::unique_ptr<Connector> make_connector( const Visual &from, const Visual &to, const ConnectorOptions &options = {} );

// Keep a connector joining its two visuals at every frame, until the handle goes
class Stage;
Updaters::Handle follow( Stage &stage, Connector &connector );
