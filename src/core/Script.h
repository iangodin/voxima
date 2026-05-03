
#pragma once

#include <utils/Huml.h>

#include <string_view>
#include <memory>

class Script
{
public:
	Script() = default;
	Script( huml::Value &&root );

	Script( const Script & ) = default;
	Script( Script && ) = default;

	~Script() = default;

	Script &operator=( const Script & ) = default;
	Script &operator=( Script && ) = default;

	bool parse( std::string_view source );

	const huml::Value &root() const
	{
		if ( _impl )
		{
			return _impl->root;
		}
		else
		{
			return _missing;
		}
	}

private:
	struct Impl
	{
		huml::Value root;
	};

	std::shared_ptr<Impl> _impl = nullptr;
	huml::Value _missing = { huml::Missing{} };
};

