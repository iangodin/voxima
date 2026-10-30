// A plugin for TestPlugin.cpp that says it was built for something else, so it must be refused (and its scene,
// registered as it loaded, must go with it)

#include <core/Plugin.h>
#include <core/SceneRegistry.h>
#include <core/Wait.h>

extern "C" SONIMATION_PLUGIN_EXPORT const char *sonimation_plugin_abi()
{
	return "sonimation-0.0 some-other-compiler";
}

namespace
{
Scene wrong_abi_scene( Stage &, Script & )
{
	co_await next_frame();
}
}

SONIMATION_SCENE( "plugin-wrong-abi", "A scene from a plugin built for something else", wrong_abi_scene );
