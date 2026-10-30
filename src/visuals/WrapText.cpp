#include "visuals/WrapText.h"

#include "visuals/Layout.h"

#include <sstream>
#include <vector>

namespace
{

float width_of( Theme &theme, const std::string &text, const FontStyle &style, const std::optional<std::string> &language )
{
	const auto set = make_text( theme, text, style, language );
	const AABB box = layout_extent( *set );
	return box.empty() ? 0.F : box.max.x() - box.min.x();
}

std::vector<std::string> split_on( const std::string &text, char separator )
{
	std::vector<std::string> parts;
	std::string part;
	std::istringstream in( text );
	while ( std::getline( in, part, separator ) )
	{
		parts.push_back( part );
	}
	if ( !text.empty() && text.back() == separator )
	{
		parts.emplace_back();
	}
	return parts;
}

}

std::string wrap_lines( Theme &theme, const std::string &text, const FontStyle &style, float max_width, const std::optional<std::string> &language )
{
	std::string wrapped;
	bool first_paragraph = true;
	for ( const std::string &paragraph: split_on( text, '\n' ) )
	{
		if ( !first_paragraph )
		{
			wrapped += '\n';
		}
		first_paragraph = false;

		std::string line;
		for ( const std::string &word: split_on( paragraph, ' ' ) )
		{
			if ( word.empty() )
			{
				continue;
			}
			if ( line.empty() )
			{
				line = word;
			}
			else if ( width_of( theme, line + " " + word, style, language ) <= max_width )
			{
				line += " " + word;
			}
			else
			{
				wrapped += line + '\n';
				line = word;
			}
		}
		wrapped += line;
	}
	return wrapped;
}

std::unique_ptr<Group> make_paragraph( Theme &theme, const std::string &text, const FontStyle &style, float max_width, const std::optional<std::string> &language, const TextLayout &layout )
{
	return make_text( theme, wrap_lines( theme, text, style, max_width, language ), style, language, layout );
}
