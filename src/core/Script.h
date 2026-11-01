
#pragma once

#include <utils/Huml.h>
#include <utils/Pronounce.h>

#include <filesystem>
#include <string>
#include <vector>

class Script
{
public:
	void set_directory( std::filesystem::path directory )
	{
		_directory = std::move( directory );
	}

	const std::filesystem::path &directory() const
	{
		return _directory;
	}

	bool load( std::string lang );

	// Make these (already loaded) languages the ones in use, in this order: the first is the master. This is how a
	// render for a single language is made from a script that has loaded several. Throws if one is not loaded.
	void select_languages( const std::vector<std::string> &languages );

	template<typename ...Keys>
	const huml::Value &fetch( std::string_view lang, Keys &&...keys ) const
	{
		const auto &script = _native_scripts.find( lang );
		if ( script != _native_scripts.end() )
		{
			return script->second.fetch( std::forward<Keys>( keys )... );
		}
		else
		{
			return _missing;
		}
	}

	// A string from the script's "labels" section, the usual place for short on-screen text (titles, captions):
	// fetch( lang, "labels", key ), with a fallback for one that is missing.
	std::string label( std::string_view lang, std::string_view key, std::string_view fallback = "�" ) const
	{
		return std::string( fetch( lang, "labels", key ).as_string( fallback ) );
	}

	// The script's "pronounce" section for a language: how words are said, wherever they appear (see utils/Pronounce.h).
	// Checked when the script is loaded.
	const Pronunciations &pronunciations( std::string_view lang ) const
	{
		static const Pronunciations none;
		const auto found = _pronunciations.find( lang );
		return found == _pronunciations.end() ? none : found->second;
	}

	const auto &languages( void ) const
	{
		return _languages;
	}

	// The first language passed to --lang ("master"): what on-screen visuals
	// render in when a Scene doesn't otherwise pick a language per element.
	const std::string &master_language( void ) const
	{
		return _languages.front();
	}

	friend std::ostream &operator<<( std::ostream &output, const Script &script )
	{
		for ( auto &[name, lang]: script._native_scripts )
		{
			output << "Language: " << name << '\n';
			output << lang << '\n';
		}
		return output;
	}

private:
	std::vector<std::string> _languages;
	huml::Value load_recursive( const std::string &lang );
	std::map<std::string,huml::Value, std::less<>> _native_scripts;
	std::map<std::string, Pronunciations, std::less<>> _pronunciations;
	huml::Value _missing = { huml::Missing{} };
	std::filesystem::path _directory;
};
