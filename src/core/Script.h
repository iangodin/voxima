
#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <utils/Huml.h>

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

	bool load( const std::string &lang );

	template<typename ...Keys>
	const huml::Value &fetch( std::string_view lang, Keys &&...keys ) const
	{
		const auto &script = _languages.find( lang );
		if ( script != _languages.end() )
		{
			return script->second.fetch( std::forward<Keys>( keys )... );
		}
		else
		{
			return _missing;
		}
	}

	friend std::ostream &operator<<( std::ostream &output, const Script &script )
	{
		for ( auto &[name, lang]: script._languages )
		{
			output << "Language: " << name << '\n';
			output << lang << '\n';
		}
		return output;
	}

private:
	huml::Value load_recursive( const std::string &lang );

	std::map<std::string,huml::Value, std::less<>> _languages;
	huml::Value _missing = { huml::Missing{} };
	std::filesystem::path _directory;
};
