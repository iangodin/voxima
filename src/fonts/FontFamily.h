#pragma once

#include "FontFace.h"
#include "FontStyle.h"

#include <utils/Huml.h>

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

class Theme;

// A font family: what turns a style (a weight, a slant, a size) into a font file. It is one entry of a font stack (a
// stack is a list of families: what the first has no glyph for, the next may). Written in a theme file as a dict, in one
// of four forms:
//
//   Files: a file for each weight and slant it comes in. The keys are light, regular, bold, black, and their
//   italics: light_italic, italic (regular's), bold_italic, black_italic. `face` is which face of a collection file, if
//   they are. An optional `small` has the same keys, for small text (the tiny and small sizes: an optical size drawn for it).
//
//       newcm::
//         regular: "NewCM10-Regular.otf"
//         bold: "NewCM10-Bold.otf"
//         italic: "NewCM10-Italic.otf"
//         small::
//           regular: "NewCM08-Regular.otf"
//
//   A style with no file is the nearest that has one: the same slant with the weight moved toward regular (black to bold to
//   regular, light to regular), then the upright one. So a family with only regular and bold serves black, and light, and
//   any italic.
//
//   Variable: one file whose weight, width and slant are axes. The style sets them (light 300, regular 400, bold 700,
//   black 900; italic is slant 1); `weight`, `width` and `slant` fix any of them instead.
//
//       plex: variable: "IBMPlexSans-Variable.ttf"
//
//   Collection: a file of several faces, one for each weight (`faces`).
//
//       source_han::
//         collection: "SourceHanSerif.ttc"
//         faces:: light: 5, regular: 10, bold: 25, black: 30
//
//   Notdef: the catch-all font that ends a stack, drawing a box for every character (see FontFace::notdef).
//
//       notdef: notdef: "AND-Regular.otf"
class FontFamily
{
public:
	// One file for each weight and slant
	class Files
	{
	public:
		void set( Weight weight, bool italic, std::string file )
		{
			_files[{ weight, italic }] = std::move( file );
		}

		bool empty() const
		{
			return _files.empty();
		}

		// The file for a style, or the nearest (see FontFamily); empty if there are none
		const std::string &pick( Weight weight, bool italic ) const;

		// The files, for saying what is in it
		const std::map<std::pair<Weight, bool>, std::string> &all() const
		{
			return _files;
		}

	private:
		std::map<std::pair<Weight, bool>, std::string> _files;
	};

	// From what a theme file says of it (a dict); a mistake throws std::runtime_error, saying which family and what
	static FontFamily from_huml( std::string_view name, const huml::Value &value );

	const std::string &name() const
	{
		return _name;
	}

	// The font for a style at a size (in stage units), loaded by the theme
	FontFace load( Theme &theme, const FontProperties &properties, float size ) const;

private:
	enum class Kind
	{
		FILES,
		VARIABLE,
		COLLECTION,
		NOTDEF
	};

	std::string _name;
	Kind _kind = Kind::FILES;

	Files _files;
	std::optional<Files> _small; // for the tiny and small sizes
	size_t _face = 0;

	std::string _file; // variable, collection, notdef
	std::map<Weight, size_t> _faces; // collection: the face for each weight
	std::optional<int> _weight;      // variable: an axis fixed
	std::optional<int> _width;
	std::optional<int> _slant;
};
