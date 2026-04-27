#pragma once

#include "Visuals.h"

#include <string>
#include <memory>

class Font
{
public:
	Font( const std::string &path );
	~Font();

	std::unique_ptr<Visuals> create_text( const std::string &text, float font_size ) const;

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

