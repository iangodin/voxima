#include "CreateCode.h"

#include <core/Theme.h>
#include <visuals/CreateStyledText.h>
#include <visuals/Bounds.h>
#include <visuals/StyledText.h>

#include <algorithm>

std::string prepare_code( std::string_view source, size_t tab_width )
{
	if ( source.starts_with( '\n' ) )
	{
		source.remove_prefix( 1 );
	}
	else if ( source.starts_with( "\r\n" ) )
	{
		source.remove_prefix( 2 );
	}

	std::string out;
	out.reserve( source.size() );
	size_t column = 0;
	for ( char c: source )
	{
		if ( c == '\r' )
		{
			continue;
		}
		if ( c == '\t' )
		{
			const size_t width = tab_width > 0 ? tab_width - column % tab_width : 0;
			out.append( width, ' ' );
			column += width;
			continue;
		}
		out.push_back( c );
		// Columns count characters, not bytes: a UTF-8 continuation byte is not one
		if ( c == '\n' )
		{
			column = 0;
		}
		else if ( ( static_cast<unsigned char>( c ) & 0xC0 ) != 0x80 )
		{
			++column;
		}
	}

	if ( out.ends_with( '\n' ) )
	{
		out.pop_back();
	}
	return out;
}

size_t CodeListing::line_count() const
{
	return size() - 1; // all but the background
}

TextLine &CodeListing::line( size_t index ) const
{
	return dynamic_cast<TextLine &>( **( begin() + 1 + static_cast<std::ptrdiff_t>( index ) ) );
}

std::unique_ptr<CodeListing> make_code( Theme &theme, code::Language language, std::string_view source, const CodeLayout &layout )
{
	const std::string text = prepare_code( source, layout.tab_width );
	const CodePalette &palette = theme.code_palette();

	StyledText styled( text );
	auto apply = [&]( size_t start, size_t end, const CodeStyle &style )
	{
		styled.set_color( start, end, style.color );
		if ( style.weight )
		{
			styled.set_weight( start, end, *style.weight );
		}
		if ( style.slant )
		{
			styled.set_slant( start, end, *style.slant );
		}
	};

	styled.set_font_style( 0, text.size(), layout.style );
	apply( 0, text.size(), palette.plain );
	for ( const code::Span &span: code::highlight( language, text ) )
	{
		apply( span.start, span.end, palette.style_for( span.name ) );
	}

	TextLayout text_layout;
	text_layout.line_spacing = layout.line_spacing;
	auto listing = std::make_unique<CodeListing>();
	listing->background = &listing->add( std::make_unique<Group>() ); // first: behind the text
	std::vector<PlacedGlyphInfo> placed;
	make_styled_text( *listing, theme, styled, text_layout, &placed );

	// How the lines sit: the font's size, the distance between lines, and how wide the text is
	listing->em = theme.get_stack( layout.style ).font( 0 ).scale;
	listing->line_pitch = listing->em * layout.line_spacing;
	BoundingBoxVisitor ink;
	listing->accept( ink );
	if ( !ink.bounds.empty() )
	{
		listing->left = ink.bounds.min.x();
		listing->right = ink.bounds.max.x();
	}

	// In the order of the source, each with its line and column (columns in characters: a UTF-8 continuation byte
	// is not one)
	std::stable_sort( placed.begin(), placed.end(), []( const PlacedGlyphInfo &a, const PlacedGlyphInfo &b ) { return a.offset < b.offset; } );
	size_t line = 0;
	size_t column = 0;
	size_t at = 0;
	for ( const PlacedGlyphInfo &p: placed )
	{
		for ( ; at < p.offset; ++at )
		{
			if ( text[at] == '\n' )
			{
				++line;
				column = 0;
			}
			else if ( ( static_cast<unsigned char>( text[at] ) & 0xC0 ) != 0x80 )
			{
				++column;
			}
		}
		listing->glyphs.push_back( { p.offset, line, column, p.glyph } );
	}
	listing->text = text;
	return listing;
}
