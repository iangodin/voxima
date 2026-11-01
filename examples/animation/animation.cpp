// Animation verbs (fade, slide, grow, indicate, turn) with connectors that follow what moves, and the camera zooming and panning.
//
// Build it (see CMakeLists.txt), then
//
//     voxima --plugin build/animation --script-dir scripts --lang en --list-scenes
//     voxima --plugin build/animation --script-dir scripts --lang en --scene verbs --preview
//
// With --preview, rebuilding it while the window is open plays the scene again with the change.

#include <core/Animate.h>
#include <core/CameraMove.h>
#include <core/EaseThereAndBack.h>
#include <core/Plugin.h>
#include <core/SceneRegistry.h>
#include <core/Script.h>
#include <core/Stage.h>
#include <core/Wait.h>
#include <plot/Graph.h>
#include <visuals/Arrow.h>
#include <visuals/Diagram.h>
#include <visuals/Geometry.h>
#include <visuals/Panel.h>
#include <visuals/Placement.h>

#include <chrono>
#include <numbers>
#include <cmath>

// Times in the scenes are written in seconds (0.5s, 1.7s), so they last as long at any --fps
using namespace std::chrono_literals;

VOXIMA_PLUGIN();

namespace
{
// The animation verbs: things fade, slide and grow in, a connector follows them as they move, one is pointed out, and they leave
Scene show_verbs( Stage &stage, Script & )
{
	Theme &theme = *stage.theme;
	const Palette &palette = theme.palette();

	auto node = [&]( Color color )
	{
		auto circle = std::make_unique<Circle>();
		circle->radius = 0.7F;
		circle->color = color;
		auto &shape = stage.visuals.add( std::move( circle ) );
		return &shape;
	};
	auto *a = node( palette.blue );
	auto *b = node( palette.green );
	auto *c = node( palette.orange );
	a->position = { -4.F, 0.F };
	b->position = { 0.F, 0.F };
	c->position = { 4.F, 0.F };
	// --8<-- [start:connector]
	auto &ab = stage.visuals.add( make_connector( *a, *b, { .gap = 0.1F } ) );
	auto &bc = stage.visuals.add( make_connector( *b, *c, { .gap = 0.1F } ) );
	auto tie_ab = follow( stage, ab );
	auto tie_bc = follow( stage, bc );
	// --8<-- [end:connector]

	auto &caption = stage.visuals.add( make_text( theme, "Fade, slide, grow, indicate", FontStyle{ Intent::TITLE } ) );
	move_to( caption, { 0.F, 3.5F }, Anchor::TOP );

	// --8<-- [start:verbs]
	co_await choreograph( fade_in( stage, *a, 0.5s ), slide_in( stage, *b, { 0.F, -3.F }, 0.8s ), grow_in( stage, *c, 0.6s ), fade_in( stage, ab, 0.8s ), fade_in( stage, bc, 0.8s ) );
	co_await wait_for( stage, 0.3s );
	co_await indicate( stage, *b );
	co_await move_by( stage, *b, { 0.F, 2.5F }, 0.8s, ease_cubic_in_out ); // the connectors bend with it
	co_await move_by( stage, *b, { 0.F, -2.5F }, 0.8s, ease_cubic_in_out );
	co_await wait_for( stage, 0.3s );
	co_await choreograph( fade_out( stage, *a, 0.5s ), slide_out( stage, *b, { 0.F, 3.F }, 0.8s ), shrink_out( stage, *c, 0.6s ), fade_out( stage, ab, 0.5s ), fade_out( stage, bc, 0.5s ) );
	// --8<-- [end:verbs]
	co_await wait_for( stage, 0.5s );

	// Drawn on: a shape's outline traced, and text written
	auto &ring = stage.visuals.add( std::make_unique<Circle>() );
	ring.radius = 1.2F;
	ring.color = palette.purple;
	ring.position = { -3.F, -0.5F };
	auto &words = stage.visuals.add( make_text( theme, "Draw me", FontStyle{ Intent::TITLE }.large() ) );
	move_to( words, { 2.5F, -0.5F }, Anchor::CENTER );
	co_await choreograph( draw_on( stage, ring, 1.6s ), draw_on( stage, words, 1.6s ) );
	co_await wait_for( stage, 0.5s );

	// Pointing out
	co_await circumscribe( stage, words );
	co_await flash( stage, ring );
	co_await wait_for( stage, 0.5s );

	// Staggered: a row of squares, each coming in a little after the last; then a bar along a curve, turning to face its way
	stage.visuals.clear();
	stage.updaters.clear();
	auto &row = stage.visuals.add( std::make_unique<Group>() );
	for ( int i = 0; i < 6; ++i )
	{
		auto square = std::make_unique<Rectangle>();
		square->half_width = square->half_height = 0.4F;
		square->color = palette.series()[static_cast<size_t>( i ) % palette.series().size()];
		square->position = { -4.F + 1.2F * static_cast<float>( i ), 2.2F };
		row.add( std::move( square ) );
	}
	// --8<-- [start:stagger]
	co_await stagger( stage, children_of( row ), 0.12s, [&]( Visual &v ) { return slide_in( stage, v, { 0.F, 1.5F }, 0.5s ); } );
	// --8<-- [end:stagger]

	auto &dart = stage.visuals.add( std::make_unique<Rectangle>() );
	dart.half_width = 0.4F;
	dart.half_height = 0.12F;
	dart.color = palette.orange;
	Path path;
	path.commands = { MoveTo{ { -4.5F, -2.F } }, CubicTo{ { 0.F, 0.F }, { -3.F, 2.F }, { 3.F, -2.5F } }, CubicTo{ { 4.5F, 0.F }, { 3.5F, 1.F }, { 4.5F, -0.5F } } };
	dart.position = { -4.5F, -2.F };
	co_await move_along( stage, dart, path, 2.5s, ease_cubic_in_out, true );
	co_await turn_about( stage, dart, { 4.5F, -0.5F }, 360.F, 1.5s, ease_cubic_in_out );
	co_await wait_for( stage, 0.5s );
}

// A sine wave, and the camera zooming in on its peaks and panning along, then back out
Scene show_camera( Stage &stage, Script & )
{
	AxesOptions options;
	options.x = { -6.0, 6.0 };
	options.y = { -2.0, 2.0 };
	options.width = 12.F;
	options.height = 4.F;
	options.grid = true;
	options.placement = AxisPlacement::EDGE;
	auto &axes = stage.visuals.add( make_axes( *stage.theme, options ) );
	axes.add_plot( plot( axes, []( double x ) { return std::sin( x ); } ) );
	co_await wait_for( stage, 1s );

	const double pi = std::numbers::pi;
	// In on the first peak, (pi / 2, 1): a look at 3 times the size, moving on to the next peak, and out again
	// --8<-- [start:camera]
	const Point peak = axes.at( pi / 2, 1.0 );
	co_await camera_to( stage, { peak, 3.F }, 1.5s );
	co_await wait_for( stage, 1s );
	co_await camera_to( stage, { axes.at( 3 * pi / 2, -1.0 ), 3.F }, 2s ); // then the trough after it
	co_await wait_for( stage, 1s );
	co_await camera_to( stage, { { 0.F, 0.F }, 1.F }, 1.5s );
	// --8<-- [end:camera]
	co_await wait_for( stage, 1s );
}
}

// Frames round things, arcs, bars that grow from their base, and what is not the point dimmed
Scene show_helpers( Stage &stage, Script & )
{
	Theme &theme = *stage.theme;
	const Palette &palette = theme.palette();
	auto &bars = stage.visuals.add( std::make_unique<Group>() );
	bars.position = { -3.5F, -2.F };
	for ( int i = 0; i < 4; ++i )
	{
		const float height = 0.8F + 0.7F * static_cast<float>( i );
		auto &bar = bars.add( std::make_unique<Rectangle>( 0.4F, height, palette.blue ) );
		bar.position = { 1.1F * static_cast<float>( i ), height };
	}
	// --8<-- [start:grow-from-edge]
	co_await stagger( stage, children_of( bars ), 0.15s, [&]( Visual &bar ) { return grow_from_edge( stage, bar, Anchor::BOTTOM, 0.6s ); } );
	// --8<-- [end:grow-from-edge]

	auto &word = stage.visuals.add( make_text( theme, "Look here", FontStyle{ Intent::TITLE }.large() ) );
	move_to( word, { 3.F, 1.F }, Anchor::CENTER );
	// --8<-- [start:surround]
	auto &frame = stage.visuals.add( make_surround( word, { .radius = 0.1F, .outline = palette.yellow } ) );
	auto &arc = stage.visuals.add( make_arc( { 3.F, -1.5F }, 1.F, 0.F, 270.F, { .thickness = 0.08F, .color = palette.green } ) );
	// --8<-- [end:surround]
	co_await choreograph( fade_in( stage, frame, 0.5s ), draw_on( stage, arc, 1s ) );

	// --8<-- [start:dim]
	co_await fade_to( stage, bars, 0.3F );  // focus on the word ...
	co_await wait_for( stage, 1s );
	co_await fade_to( stage, bars, 1.F );   // ... and back
	// --8<-- [end:dim]
	co_await tween( stage, 0.8s, ease_there_and_back, [&]( float t ) { frame.stroke_thickness = 0.05F + 0.1F * t; } ); // a pulse
	co_await wait_for( stage, 0.5s );
}

REGISTER_SCENE( "verbs", "Fade, slide, grow and indicate, with connectors following the moving circles", show_verbs );
// Arrows between points and along a curve, drawn on
Scene show_arrows( Stage &stage, Script & )
{
	const Palette &palette = stage.theme->palette();
	// --8<-- [start:arrow]
	auto &straight = stage.visuals.add( make_arrow( { -5.F, -1.F }, { -1.F, 1.F }, { .color = palette.blue } ) );
	auto &curved = stage.visuals.add( make_arrow( *make_arc( { 3.F, -1.F }, 2.F, 180.F, 0.F ), { .arrow_at_start = true, .color = palette.orange } ) );
	curved.set_drawn( 0.F );                 // (not there until it is its turn)
	co_await draw( stage, straight, 0.8s );
	co_await draw( stage, curved, 1.2s );
	// --8<-- [end:arrow]
	co_await wait_for( stage, 1s );
}

REGISTER_SCENE( "arrows", "Arrows between points and along a curve, drawn on", show_arrows );
REGISTER_SCENE( "helpers", "A frame round a word, an arc, bars that grow from their base, and dimming", show_helpers );
REGISTER_SCENE( "camera", "Zooming in on a graph, and panning along it", show_camera );
