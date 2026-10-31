// A plugin of scenes: build it (see CMakeLists.txt), then
//
//     voxima --plugin build/hello --script-dir scripts --lang en --theme "Academic Classic" --preview
//
// With --preview, rebuilding it while the window is open plays the scene again with the change.

#include <core/Animate.h>
#include <core/Plugin.h>
#include <core/SceneRegistry.h>
#include <core/Script.h>
#include <core/Stage.h>
#include <core/Wait.h>
#include <visuals/Circle.h>
#include <visuals/CreateText.h>
#include <visuals/Placement.h>

#include <memory>
#include <string>

using namespace std::chrono_literals;

VOXIMA_PLUGIN();

namespace
{
// --8<-- [start:scene]
Scene hello( Stage &stage, Script &script )
{
	Theme &theme = *stage.theme;
	const std::string title{ script.fetch( script.master_language(), "title" ).as_string() };

	auto &words = stage.visuals.add( make_text( theme, title, FontStyle{ Intent::TITLE }.large() ) );
	move_to( words, { 0.F, 1.5F } );

	auto circle = std::make_unique<Circle>();
	circle->radius = 1.F;
	circle->color = theme.palette().blue;
	auto &dot = stage.visuals.add( std::move( circle ) );
	dot.position = { 0.F, -1.5F };

	co_await fade_in( stage, words, 0.8s );
	co_await grow_in( stage, dot, 0.6s );
	co_await indicate( stage, dot );
	co_await wait_for( stage, 1s );
}
// --8<-- [end:scene]
}

// "movie" is what plays without --scene
// --8<-- [start:register]
REGISTER_SCENE( "movie", "A title and a circle", hello );
// --8<-- [end:register]
