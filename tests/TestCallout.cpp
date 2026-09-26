#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Stage.h>
#include <visuals/Callout.h>
#include <visuals/Placement.h>
#include <visuals/Rectangle.h>

using Catch::Approx;

namespace
{
std::unique_ptr<Rectangle> box( float w, float h, float x = 0.F, float y = 0.F )
{
	auto r = std::make_unique<Rectangle>();
	r->half_width = w / 2.F;
	r->half_height = h / 2.F;
	r->position = { x, y };
	return r;
}
}

TEST_CASE( "a callout points from its panel to its target", "[callout]" )
{
	Group group;
	auto &target = group.add( box( 2.F, 2.F, 0.F, 0.F ) );
	auto &callout = group.add( make_callout( box( 3.F, 1.F ), target ) );
	move_to( callout.panel(), { 0.F, 5.F }, Anchor::CENTER );
	callout.update();

	// The panel is above the target, so the arrow runs down between them
	auto &tip = callout.pointer().end_tip();
	CHECK( tip.end.x() == Approx( 0.F ) );
	CHECK( tip.end.y() > 1.F );               // stops short of the target's top edge (y = 1)
	CHECK( tip.end.y() < 1.5F );
	CHECK( callout.pointer().line().start.y() < point_at( callout.panel(), Anchor::BOTTOM ).y() );
}

TEST_CASE( "a followed callout keeps pointing as its target moves", "[callout]" )
{
	Stage stage;
	auto &target = stage.visuals.add( box( 2.F, 2.F, 0.F, 0.F ) );
	auto &callout = stage.visuals.add( make_callout( box( 3.F, 1.F ), target ) );
	move_to( callout.panel(), { 0.F, 5.F }, Anchor::CENTER );
	auto tie = follow( stage, callout );

	target.position = { 6.F, 0.F };
	stage.updaters.run();
	CHECK( callout.pointer().end_tip().end.x() > 3.F );
}
