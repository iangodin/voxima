
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
	FRAKTUR,
	DOUBLE_STRUCK,
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
	MATH_ITALIC,
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

struct FontProperties
{
	std::optional<TypeClass> typeclass;
	std::optional<Weight> weight;
	std::optional<Slant> slant;
	std::optional<Width> width;
	std::optional<Scale> scale;
	// Multiplier applied on top of `scale`, for sizes the Scale steps don't cover
	std::optional<float> size_factor;
};

class FontStyle
{
public:
	FontStyle() = default;

	FontStyle( const FontStyle & ) = default;
	FontStyle( FontStyle && ) = default;

	explicit FontStyle( std::string_view intent ) :
		_intent( intent )
	{
	}

	FontStyle( std::string_view intent, FontProperties properties ) :
		_intent( intent ),
		_typeclass( properties.typeclass ),
		_weight( properties.weight ),
		_slant( properties.slant ),
		_width( properties.width ),
		_scale( properties.scale ),
		_size_factor( properties.size_factor )
	{
	}

	~FontStyle() = default;

	FontStyle &operator=( const FontStyle & ) = default;
	FontStyle &operator=( FontStyle && ) = default;

	const std::string &intent( void ) const
	{
		return _intent;
	}

	FontStyle &sans( void )
	{
		_typeclass = TypeClass::SANS;
		return *this;
	}

	FontStyle &serif( void )
	{
		_typeclass = TypeClass::SERIF;
		return *this;
	}

	FontStyle &mono( void )
	{
		_typeclass = TypeClass::MONO;
		return *this;
	}

	FontStyle &slab( void )
	{
		_typeclass = TypeClass::SLAB;
		return *this;
	}

	FontStyle &script( void )
	{
		_typeclass = TypeClass::SCRIPT;
		return *this;
	}

	// Math-alphabet requests only (𝔉𝔯𝔞𝔨𝔱𝔲𝔯, no font file has a Fraktur style of its own)
	FontStyle &fraktur( void )
	{
		_typeclass = TypeClass::FRAKTUR;
		return *this;
	}

	// Math-alphabet requests only (blackboard bold, e.g. ℝ, ℕ)
	FontStyle &double_struck( void )
	{
		_typeclass = TypeClass::DOUBLE_STRUCK;
		return *this;
	}

	FontStyle &light( void )
	{
		_weight = Weight::LIGHT;
		return *this;
	}

	FontStyle &regular( void )
	{
		_weight = Weight::REGULAR;
		return *this;
	}

	FontStyle &bold( void )
	{
		_weight = Weight::BOLD;
		return *this;
	}

	FontStyle &black( void )
	{
		_weight = Weight::BLACK;
		return *this;
	}

	FontStyle &upright( void )
	{
		_slant = Slant::UPRIGHT;
		return *this;
	}

	FontStyle &italic( void )
	{
		_slant = Slant::ITALIC;
		return *this;
	}

	FontStyle &oblique( void )
	{
		_slant = Slant::OBLIQUE;
		return *this;
	}

	// The math intent already defaults to this. Not the same as italic(): that asks for an
	// italic font file, which math fonts don't have, so it has no visible effect there.
	FontStyle &math_italic( void )
	{
		_slant = Slant::MATH_ITALIC;
		return *this;
	}

	FontStyle &tiny( void )
	{
		_scale = Scale::TINY;
		return *this;
	}

	FontStyle &small( void )
	{
		_scale = Scale::SMALL;
		return *this;
	}

	FontStyle &medium( void )
	{
		_scale = Scale::MEDIUM;
		return *this;
	}

	FontStyle &large( void )
	{
		_scale = Scale::LARGE;
		return *this;
	}

	FontStyle &huge( void )
	{
		_scale = Scale::HUGE;
		return *this;
	}

	FontStyle &compressed( void )
	{
		_width = Width::COMPRESSED;
		return *this;
	}

	FontStyle &condensed( void )
	{
		_width = Width::CONDENSED;
		return *this;
	}

	FontStyle &standard( void )
	{
		_width = Width::STANDARD;
		return *this;
	}

	FontStyle &expanded( void )
	{
		_width = Width::EXPANDED;
		return *this;
	}

	FontStyle &set_intent( std::string_view intent )
	{
		_intent = std::string( intent );
		return *this;
	}

	FontStyle &set_typeclass( TypeClass typeclass )
	{
		_typeclass = typeclass;
		return *this;
	}

	FontStyle &set_weight( Weight weight )
	{
		_weight = weight;
		return *this;
	}

	FontStyle &set_slant( Slant slant )
	{
		_slant = slant;
		return *this;
	}

	// Multiply the size by `factor` (compounds with earlier calls)
	FontStyle &scaled( float factor )
	{
		_size_factor = _size_factor.value_or( 1.F ) * factor;
		return *this;
	}

	FontStyle &set_scale( Scale scale )
	{
		_scale = scale;
		return *this;
	}

	FontStyle &set_width( Width width )
	{
		_width = width;
		return *this;
	}

	bool operator==( const FontStyle &other ) const
	{
		return
			_intent == other._intent &&
			_typeclass == other._typeclass &&
			_weight == other._weight &&
			_slant == other._slant &&
			_width == other._width &&
			_scale == other._scale &&
			_size_factor == other._size_factor;
	}

	bool operator!=( const FontStyle &other ) const
	{
		return !operator==( other );
	}

	bool operator<( const FontStyle &other ) const
	{
		if ( _intent != other._intent ) return _intent < other._intent;
		if ( _typeclass != other._typeclass ) return _typeclass < other._typeclass;
		if ( _weight != other._weight ) return _weight < other._weight;
		if ( _slant != other._slant ) return _slant < other._slant;
		if ( _width != other._width ) return _width < other._width;
		if ( _scale != other._scale ) return _scale < other._scale;
		return _size_factor < other._size_factor;
	}

	FontProperties properties( void ) const
	{
		return {
			.typeclass = _typeclass,
			.weight = _weight,
			.slant = _slant,
			.width = _width,
			.scale = _scale,
			.size_factor = _size_factor,
		};
	}

public:
	std::string _intent{ Intent::PROSE };
	std::optional<TypeClass> _typeclass;
	std::optional<Weight> _weight;
	std::optional<Slant> _slant;
	std::optional<Width> _width;
	std::optional<Scale> _scale;
	std::optional<float> _size_factor;
};

