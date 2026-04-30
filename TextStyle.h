
#pragma once

#include <string>
#include <string_view>
#include <optional>

enum class TypeClass
{
	SANS,
	SERIF,
	SLAB,
	MONO,
	SCRIPT,
};

enum class Weight
{
	LIGHT,
	REGULAR,
	BOLD,
	BLACK,
};

enum class Slant
{
	UPRIGHT,
	ITALIC,
	OBLIQUE,
};

enum class Scale
{
	TINY,
	SMALL,
	MEDIUM,
	LARGE,
	HUGE,
};

enum class Width
{
	COMPRESSED,
	CONDENSED,
	STANDARD,
	EXPANDED,
};

namespace Intent
{
	constexpr std::string_view PROSE = "prose";
	constexpr std::string_view CODE = "code";
	constexpr std::string_view MATH = "math";
	constexpr std::string_view TITLE = "title";
	constexpr std::string_view LABEL = "label";
	constexpr std::string_view ANNOTATION = "annotation";
	constexpr std::string_view CAPTION = "caption";
}

struct TextProperties
{
	std::optional<TypeClass> typeclass;
	std::optional<Weight> weight;
	std::optional<Slant> slant;
	std::optional<Width> width;
	std::optional<Scale> scale;
};

class TextStyle
{
public:
	explicit TextStyle( std::string_view intent ) :
		_intent( intent )
	{
	}

	TextStyle( std::string_view intent, TextProperties properties ) :
		_intent( intent ),
		_typeclass( properties.typeclass ),
		_weight( properties.weight ),
		_slant( properties.slant ),
		_width( properties.width ),
		_scale( properties.scale )
	{
	}

	const std::string &intent( void ) const
	{
		return _intent;
	}

	TextStyle &sans( void )
	{
		_typeclass = TypeClass::SANS;
		return *this;
	}

	TextStyle &serif( void )
	{
		_typeclass = TypeClass::SERIF;
		return *this;
	}

	TextStyle &mono( void )
	{
		_typeclass = TypeClass::MONO;
		return *this;
	}

	TextStyle &slab( void )
	{
		_typeclass = TypeClass::SLAB;
		return *this;
	}

	TextStyle &script( void )
	{
		_typeclass = TypeClass::SCRIPT;
		return *this;
	}

	TextStyle &light( void )
	{
		_weight = Weight::LIGHT;
		return *this;
	}

	TextStyle &regular( void )
	{
		_weight = Weight::REGULAR;
		return *this;
	}

	TextStyle &bold( void )
	{
		_weight = Weight::BOLD;
		return *this;
	}

	TextStyle &black( void )
	{
		_weight = Weight::BLACK;
		return *this;
	}

	TextStyle &upright( void )
	{
		_slant = Slant::UPRIGHT;
		return *this;
	}

	TextStyle &italic( void )
	{
		_slant = Slant::ITALIC;
		return *this;
	}

	TextStyle &oblique( void )
	{
		_slant = Slant::OBLIQUE;
		return *this;
	}

	TextStyle &tiny( void )
	{
		_scale = Scale::TINY;
		return *this;
	}

	TextStyle &small( void )
	{
		_scale = Scale::SMALL;
		return *this;
	}

	TextStyle &medium( void )
	{
		_scale = Scale::MEDIUM;
		return *this;
	}

	TextStyle &large( void )
	{
		_scale = Scale::LARGE;
		return *this;
	}

	TextStyle &huge( void )
	{
		_scale = Scale::HUGE;
		return *this;
	}

	bool operator<( const TextStyle &other ) const
	{
		if ( _intent != other._intent ) return _intent < other._intent;
		if ( _typeclass != other._typeclass ) return _typeclass < other._typeclass;
		if ( _weight != other._weight ) return _weight < other._weight;
		if ( _slant != other._slant ) return _slant < other._slant;
		if ( _width != other._width ) return _width < other._width;
		return _scale < other._scale;
	}

	TextProperties properties( void ) const
	{
		return {
			.typeclass = _typeclass,
			.weight = _weight,
			.slant = _slant,
			.width = _width,
			.scale = _scale,
		};
	}

private:
	std::string _intent;
	std::optional<TypeClass> _typeclass;
	std::optional<Weight> _weight;
	std::optional<Slant> _slant;
	std::optional<Width> _width;
	std::optional<Scale> _scale;
};

