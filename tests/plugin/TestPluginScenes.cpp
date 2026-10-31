// A plugin for TestPlugin.cpp: one scene, which adds a visual made by the plugin's code and lasts a few frames

#include <core/Plugin.h>
#include <core/SceneRegistry.h>
#include <core/Stage.h>
#include <core/Wait.h>
#include <visuals/Circle.h>

#include <memory>

VOXIMA_PLUGIN();

namespace
{
// Its own kind of visual, so its vtable is in the plugin
class PluginCircle : public Circle
{
};

Scene plugin_scene( Stage &stage, Script & )
{
	stage.visuals.add( std::make_unique<PluginCircle>() );
	for ( int i = 0; i < 3; ++i )
	{
		co_await next_frame();
	}
}
}

REGISTER_SCENE( "plugin-test", "A scene from a plugin", plugin_scene );
