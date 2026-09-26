#include "SceneRegistry.h"

#include "Stage.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::vector<const SceneEntry *> SceneEntry::all()
{
	std::vector<const SceneEntry *> scenes;
	for ( const auto &[name, entry]: get_registry() )
	{
		scenes.push_back( entry );
	}
	std::sort( scenes.begin(), scenes.end(), []( const SceneEntry *a, const SceneEntry *b ) { return a->get_name() < b->get_name(); } );
	return scenes;
}

std::vector<std::string> unknown_scenes( const std::vector<std::string> &names )
{
	std::vector<std::string> unknown;
	for ( const auto &name: names )
	{
		if ( !SceneEntry::get_by_name( name ) )
		{
			unknown.push_back( name );
		}
	}
	return unknown;
}

std::string describe_scenes()
{
	const auto scenes = SceneEntry::all();
	size_t width = 0;
	for ( const auto *scene: scenes )
	{
		width = std::max( width, scene->get_name().size() );
	}

	std::ostringstream out;
	if ( scenes.empty() )
	{
		out << "  (no scenes are registered)\n";
	}
	for ( const auto *scene: scenes )
	{
		out << "  " << scene->get_name() << std::string( width - scene->get_name().size() + 2, ' ' ) << scene->description() << '\n';
	}
	return out.str();
}

Scene play( Stage &stage, Script &script, std::string name, PlayOptions options )
{
	const SceneEntry *entry = SceneEntry::get_by_name( name );
	if ( !entry )
	{
		std::cerr << "play: there is no scene called '" << name << "'. Scenes:\n" << describe_scenes();
		throw std::invalid_argument( "unknown scene: " + name );
	}

	if ( options.clear_stage )
	{
		stage.visuals.clear();
		stage.updaters.clear(); // (they refer to what was there)
	}

	if ( stage.scene_event )
	{
		stage.scene_event( name, true );
	}
	co_await entry->run( stage, script );
	if ( stage.scene_event )
	{
		stage.scene_event( name, false );
	}
}

Scene sequence( Stage &stage, Script &script, std::vector<std::string> names, PlayOptions options )
{
	for ( const auto &name: names )
	{
		co_await play( stage, script, name, options );
	}
}
