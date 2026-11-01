#pragma once

#include <filesystem>
#include <optional>
#include <string>

// Where Voxima's data (fonts, speech models, voices) is looked for when no option says, in this order: a folder
// here (relative to where it is run, so a source tree or a project can have its own), the one an environment
// variable names, and the one installed with the program.
struct DataPlaces
{
	std::filesystem::path local;
	std::string environment_variable;
	std::filesystem::path installed; // empty if there is none

	// The usual ones: `local`, and share/voxima/<name> next to the program's bin/
	static DataPlaces standard( std::filesystem::path local, std::string environment_variable, const std::string &name );
};

// The first folder there is. An environment variable is believed without looking: if it is wrong, what is missing
// from it is reported as missing from there, rather than something else being used.
std::optional<std::filesystem::path> find_data_dir( const DataPlaces &places );

// The first of those folders that has this file in it, so that a folder without it does not hide one that does
std::optional<std::filesystem::path> find_data_file( const DataPlaces &places, const std::filesystem::path &file );
