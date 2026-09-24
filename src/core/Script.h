
#pragma once

#include <utils/Huml.h>

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
	huml::Value _missing = { huml::Missing{} };
	std::filesystem::path _directory;
};
