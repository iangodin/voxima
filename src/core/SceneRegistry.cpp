#include "SceneRegistry.h"

#include "Stage.h"
#include "Tween.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

template<>
std::unordered_map<std::string, SceneEntry *> &NamedRegistered<SceneEntry>::get_registry()
{
	static std::unordered_map<std::string, SceneEntry *> registry;
	return registry;
}

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
		out << "  (no scenes are registered: load some with --plugin)\n";
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
		if ( options.fade_out.count() > 0.0 && stage.visuals.size() > 0 )
		{
			co_await tween( stage, options.fade_out, []( float t ) { return t; }, [&stage]( float t ) { stage.visuals.color.a = 1.F - t; } );
		}
		stage.visuals.clear();
		stage.updaters.clear(); // (they refer to what was there)
		stage.visuals.color.a = 1.F;
	}

	// The new scene coming in: its visuals more visible with every frame, for as long as it takes
	Updaters::Handle fading_in;
	if ( options.fade_in.count() > 0.0 )
	{
		const int64_t frames = std::max<int64_t>( 1, stage.camera.frames( options.fade_in ) );
		auto frame = std::make_shared<int64_t>( 0 );
		stage.visuals.color.a = 0.F;
		fading_in = stage.updaters.add(
			[&stage, frames, frame]
			{
				++*frame;
				stage.visuals.color.a = std::min( 1.F, static_cast<float>( *frame ) / static_cast<float>( frames ) );
			} );
	}

	if ( stage.scene_event )
	{
		stage.scene_event( name, true );
	}
	co_await entry->run( stage, script );
	stage.visuals.color.a = 1.F;
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
