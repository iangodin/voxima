#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

// A plugin of scenes (see Plugin.h), loaded into the program. Loading it registers its scenes (their
// REGISTER_SCENE objects are made as it loads), and unloading it unregisters them.
//
// What is loaded is a copy of the file, so the file itself can be built again while the program runs (Windows
// does not let a loaded DLL be written over); changed() says when it has been, for the preview to reload it.
//
// Nothing made by a plugin's code may outlive it: its scenes' coroutines, the visuals they made, anything
// holding its functions. Clear them all before unloading it.
class Plugin
{
public:
	// `path` may leave out the extension (.dll on Windows, .so elsewhere). It is not loaded yet.
	explicit Plugin( std::filesystem::path path );
	~Plugin();

	Plugin( Plugin &&other ) noexcept;
	Plugin &operator=( Plugin &&other ) noexcept;
	Plugin( const Plugin & ) = delete;
	Plugin &operator=( const Plugin & ) = delete;

	// Load it (again, if it was loaded). Throws if it cannot be, or was not built as the program was; it is not
	// loaded then.
	void load();
	void unload();

	bool loaded() const
	{
		return _handle != nullptr;
	}

	const std::filesystem::path &path() const
	{
		return _path;
	}

	// Whether the file is not the one last loaded (or tried: one that failed waits to be built again), and has
	// stayed the same since the last time this was asked (so it is not still being written). Asked a few times a
	// second, it says so once the build has finished with it.
	bool changed();

private:
	using Stamp = std::pair<std::filesystem::file_time_type, std::uintmax_t>;

	// The file's time and size, or nothing if it cannot be read (it is being written, say)
	std::optional<Stamp> stamp() const;

	std::filesystem::path _path;
	std::filesystem::path _copy; // what is loaded
	void *_handle = nullptr;
	std::optional<Stamp> _loaded_stamp;
	std::optional<Stamp> _seen_stamp;
};

// Load every plugin file in a directory (those with the platform's extension, .dll or .so, in name order, so that
// when two register a scene of the same name the same one always wins). A file that cannot be loaded (it was built
// with another compiler or Voxima, say, or is not a plugin) is not an error: a warning naming it and why is
// written to `warnings`, and the rest are loaded. Throws if `directory` is not a directory.
std::vector<Plugin> load_plugins_in( const std::filesystem::path &directory, std::ostream &warnings );

// What the program was built with, which a plugin must match (VOXIMA_ABI, see Plugin.h)
const char *voxima_abi();
