#pragma once

#include "core/Scene.h"
#include "visuals/ArrowTip.h"
#include "visuals/Connector.h"
#include "visuals/Group.h"
#include "visuals/Outline.h"
#include "visuals/Path.h"

#include <chrono>
#include <memory>

struct Stage;

// A free arrow: a line, straight or curved, with arrow heads, from one point to another. Where a connector joins two visuals
// and follows them, this stays where it is put. Move, scale or fade it and it all goes with it.
//
//     auto &axis = stage.visuals.add( make_arrow( { -3.F, 0.F }, { 3.F, 0.F } ) );
//     auto &bend = stage.visuals.add( make_arrow( *make_arc( { 0.F, 0.F }, 2.F, 0.F, 120.F ) ) );
//     co_await draw( stage, bend, 0.8s );                      // the line, then the head
//
// It takes a connector's options, but for `gap`, which is what it leaves round the visuals it joins.
class Arrow : public Group
{
public:
	Arrow( const Contour &contour, const ConnectorOptions &options );

	// How much of it to draw, 0 to 1: the head at the start if it has one, then the line, then the head at the end
	void set_drawn( float fraction );

	float drawn() const
	{
		return _drawn;
	}

	const Path &shaft() const
	{
		return *_shaft;
	}

	// The head at the end, and at the start (of no size when there is none)
	const ArrowTip &end_tip() const
	{
		return *_end_tip;
	}

	const ArrowTip &start_tip() const
	{
		return *_start_tip;
	}

	Arrow &operator*=( float scale ) override;

private:
	// Where the line runs and the heads sit, from the contour and the size of the heads
	void lay_out();

	Contour _contour;
	ConnectorOptions _options;
	Contour _line; // the contour without the part the heads take
	bool _has_end_tip = false;
	bool _has_start_tip = false;
	Point _end_base;
	Point _start_base;
	float _drawn = 1.F;
	Path *_shaft;
	ArrowTip *_end_tip;
	ArrowTip *_start_tip;
};

// An arrow in a straight line, from one point to another
std::unique_ptr<Arrow> make_arrow( Point from, Point to, const ConnectorOptions &options = {} );

// An arrow along a path (its first piece, see contour_of), from its start to its end, put where the path is
std::unique_ptr<Arrow> make_arrow( const Path &path, const ConnectorOptions &options = {} );

// Draw an arrow on, from nothing to all of it, over a length of time: the line and then the head
Scene draw( const Stage &stage, Arrow &arrow, std::chrono::duration<double> duration = std::chrono::milliseconds( 800 ) );
