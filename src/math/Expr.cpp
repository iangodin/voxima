#include "math/Expr.h"
#include "visuals/Accent.h"
#include "visuals/Bars.h"
#include "visuals/CreateMathLargeOperation.h"
#include "visuals/CreateText.h"
#include "visuals/Delimited.h"
#include "visuals/Fraction.h"
#include "visuals/Group.h"
#include "visuals/Layout.h"
#include "visuals/Limits.h"
#include "visuals/Line.h"
#include "visuals/Rectangle.h"
#include "visuals/Radical.h"
#include "visuals/Scripts.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <stdexcept>

namespace math
{

namespace
{
// An optional part: nothing if the expression is empty
std::unique_ptr<Visual> build_part( const Expr &part, const Context &context, MathStyle style )
{
	return part ? part.build( context, style ) : nullptr;
}
}

Expr Expr::from( Builder builder )
{
	Expr expr;
	expr._builder = std::make_shared<const Builder>( std::move( builder ) );
	return expr;
}

std::unique_ptr<Visual> Expr::build( const Context &context, MathStyle style ) const
{
	if ( !_builder )
	{
		throw std::invalid_argument( "math::Expr: building an empty expression" );
	}
	return ( *_builder )( context, style );
}

// ---- Text --------------------------------------------------------------------------------

Expr text( std::string characters )
{
	return Expr::from(
		[characters = std::move( characters )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{ return make_text( context.theme(), characters, context.font_style( style ) ); } );
}

Expr upright( std::string characters )
{
	return Expr::from(
		[characters = std::move( characters )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			FontStyle font_style = context.font_style( style );
			font_style.upright();
			return make_text( context.theme(), characters, font_style );
		} );
}

Expr words( std::string characters )
{
	return upright( std::move( characters ) );
}

Expr bold( std::string characters )
{
	return Expr::from(
		[characters = std::move( characters )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			FontStyle font_style = context.font_style( style );
			font_style.bold();
			return make_text( context.theme(), characters, font_style );
		} );
}

namespace
{
// text() in one of the other math alphabets: FontStyle::(*setter)() picks it, upright
Expr alphabet_text( std::string characters, FontStyle &( FontStyle::*setter )() )
{
	return Expr::from(
		[characters = std::move( characters ), setter]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			FontStyle font_style = context.font_style( style );
			( font_style.*setter )();
			return make_text( context.theme(), characters, font_style );
		} );
}
}

Expr script( std::string characters )
{
	return alphabet_text( std::move( characters ), &FontStyle::script );
}

Expr fraktur( std::string characters )
{
	return alphabet_text( std::move( characters ), &FontStyle::fraktur );
}

Expr double_struck( std::string characters )
{
	return alphabet_text( std::move( characters ), &FontStyle::double_struck );
}

Expr sans( std::string characters )
{
	return alphabet_text( std::move( characters ), &FontStyle::sans );
}

Expr mono( std::string characters )
{
	return alphabet_text( std::move( characters ), &FontStyle::mono );
}

// ---- Constructs --------------------------------------------------------------------------

Expr fraction( Expr numerator, Expr denominator )
{
	return Expr::from(
		[numerator = std::move( numerator ),
			denominator = std::move( denominator )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			return std::make_unique<Fraction>(
				m.font,
				m.glyph_scale,
				numerator.build( context, style.numerator() ),
				denominator.build( context, style.denominator() ),
				FractionOptions{ .style = style } );
		} );
}

Expr binomial( Expr top, Expr bottom )
{
	return Expr::from(
		[top = std::move( top ), bottom = std::move( bottom )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			return make_binomial(
				m.font,
				m.glyph_scale,
				top.build( context, style.numerator() ),
				bottom.build( context, style.denominator() ),
				style );
		} );
}

Expr radical( Expr radicand )
{
	return Expr::from(
		[radicand = std::move( radicand )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			return std::make_unique<Radical>(
				m.font, m.glyph_scale, radicand.build( context, style.radicand() ), style );
		} );
}

Expr root( Expr index, Expr radicand )
{
	return Expr::from(
		[index = std::move( index ), radicand = std::move( radicand )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			return std::make_unique<Radical>(
				m.font,
				m.glyph_scale,
				index.build( context, style.index() ),
				radicand.build( context, style.radicand() ),
				style );
		} );
}

Expr primed( Expr base, int count )
{
	std::string marks;
	for ( int i = 0; i < std::max( 1, count ); ++i )
	{
		marks += "′";
	}
	return superscript( std::move( base ), upright( std::move( marks ) ) );
}

Expr scripts( Expr base, Expr superscript_, Expr subscript_ )
{
	return Expr::from(
		[base = std::move( base ), superscript_ = std::move( superscript_ ),
			subscript_ = std::move( subscript_ )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			return std::make_unique<Scripts>(
				m.font,
				m.glyph_scale,
				base.build( context, style ),
				build_part( superscript_, context, style.superscript() ),
				build_part( subscript_, context, style.subscript() ),
				style );
		} );
}

Expr superscript( Expr base, Expr script )
{
	return scripts( std::move( base ), std::move( script ), Expr() );
}

Expr subscript( Expr base, Expr script )
{
	return scripts( std::move( base ), Expr(), std::move( script ) );
}

namespace
{
Expr stacked( Expr label, Expr base, bool over )
{
	return Expr::from(
		[label = std::move( label ), base = std::move( base ),
			over]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			auto built_label = label.build( context, over ? style.superscript() : style.subscript() );
			return std::make_unique<Limits>(
				m.font,
				m.glyph_scale,
				base.build( context, style ),
				over ? std::move( built_label ) : nullptr,
				over ? nullptr : std::move( built_label ) );
		} );
}
}

Expr overset( Expr label, Expr base )
{
	return stacked( std::move( label ), std::move( base ), true );
}

Expr underset( Expr label, Expr base )
{
	return stacked( std::move( label ), std::move( base ), false );
}

Expr stackrel( Expr label, Expr base )
{
	return overset( std::move( label ), std::move( base ) );
}

Expr substack( std::vector<Expr> lines )
{
	std::vector<std::vector<Expr>> rows;
	for ( Expr &line: lines )
	{
		rows.push_back( { std::move( line ) } );
	}
	MatrixOptions options;
	options.align = { ColumnAlign::CENTER };
	options.baseline_skip_em = 1.0F;
	return matrix( std::move( rows ), options );
}

Expr continued_fraction( std::vector<Expr> terms )
{
	if ( terms.empty() )
	{
		return Expr();
	}
	// From the last term outward: 1/a_n, then 1/(a_(n-1) + that) ...
	Expr tail = std::move( terms.back() );
	terms.pop_back();
	Expr inner = with_style( MathStyle::display(), fraction( text( "1" ), tail ) );
	for ( size_t i = terms.size(); i-- > 0; )
	{
		inner = with_style( MathStyle::display(),
			fraction( text( "1" ), row( { ord( terms[i] ), bin( "+" ), ord( inner ) } ) ) );
	}
	return inner;
}

Expr large_operator( char32_t symbol, Expr lower, Expr upper, std::optional<LimitPlacement> placement )
{
	return Expr::from(
		[symbol, lower = std::move( lower ), upper = std::move( upper ),
			placement]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			return make_large_operator(
				m.font,
				m.glyph_scale,
				symbol,
				build_part( upper, context, style.superscript() ),
				build_part( lower, context, style.subscript() ),
				style,
				placement );
		} );
}

Expr sum( Expr lower, Expr upper )
{
	return large_operator( U'∑', std::move( lower ), std::move( upper ) );
}

Expr product( Expr lower, Expr upper )
{
	return large_operator( U'∏', std::move( lower ), std::move( upper ) );
}

Expr integral( Expr lower, Expr upper )
{
	return large_operator( U'∫', std::move( lower ), std::move( upper ) );
}

Expr named_operator( std::string name, Expr lower, Expr upper, bool moving_limits )
{
	return Expr::from(
		[name = std::move( name ), lower = std::move( lower ), upper = std::move( upper ),
			moving_limits]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			FontStyle font_style = context.font_style( style );
			font_style.upright();

			return make_named_operator(
				m.font,
				m.glyph_scale,
				make_text( context.theme(), name, font_style ),
				build_part( upper, context, style.superscript() ),
				build_part( lower, context, style.subscript() ),
				style,
				moving_limits );
		} );
}

Expr lim( Expr lower )
{
	return named_operator( "lim", std::move( lower ), {}, true );
}

Expr limsup( Expr lower )
{
	return named_operator( "lim sup", std::move( lower ), {}, true );
}

Expr liminf( Expr lower )
{
	return named_operator( "lim inf", std::move( lower ), {}, true );
}

Expr max( Expr lower )
{
	return named_operator( "max", std::move( lower ), {}, true );
}

Expr min( Expr lower )
{
	return named_operator( "min", std::move( lower ), {}, true );
}

Expr sup( Expr lower )
{
	return named_operator( "sup", std::move( lower ), {}, true );
}

Expr inf( Expr lower )
{
	return named_operator( "inf", std::move( lower ), {}, true );
}

Expr gcd( Expr lower )
{
	return named_operator( "gcd", std::move( lower ), {}, true );
}

Expr sin( Expr sub ) { return named_operator( "sin", std::move( sub ) ); }
Expr cos( Expr sub ) { return named_operator( "cos", std::move( sub ) ); }
Expr tan( Expr sub ) { return named_operator( "tan", std::move( sub ) ); }
Expr cot( Expr sub ) { return named_operator( "cot", std::move( sub ) ); }
Expr sec( Expr sub ) { return named_operator( "sec", std::move( sub ) ); }
Expr csc( Expr sub ) { return named_operator( "csc", std::move( sub ) ); }
Expr arcsin( Expr sub ) { return named_operator( "arcsin", std::move( sub ) ); }
Expr arccos( Expr sub ) { return named_operator( "arccos", std::move( sub ) ); }
Expr arctan( Expr sub ) { return named_operator( "arctan", std::move( sub ) ); }
Expr sinh( Expr sub ) { return named_operator( "sinh", std::move( sub ) ); }
Expr cosh( Expr sub ) { return named_operator( "cosh", std::move( sub ) ); }
Expr tanh( Expr sub ) { return named_operator( "tanh", std::move( sub ) ); }
Expr log( Expr sub ) { return named_operator( "log", std::move( sub ) ); }
Expr ln( Expr sub ) { return named_operator( "ln", std::move( sub ) ); }
Expr exp( Expr sub ) { return named_operator( "exp", std::move( sub ) ); }
Expr det( Expr sub ) { return named_operator( "det", std::move( sub ) ); }
Expr dim( Expr sub ) { return named_operator( "dim", std::move( sub ) ); }
Expr deg( Expr sub ) { return named_operator( "deg", std::move( sub ) ); }
Expr arg( Expr sub ) { return named_operator( "arg", std::move( sub ) ); }

Expr fenced( char32_t left, Expr content, char32_t right )
{
	return Expr::from(
		[left, content = std::move( content ), right]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			return std::make_unique<Delimited>(
				m.font, m.glyph_scale, left, content.build( context, style ), right, style );
		} );
}

Expr big( char32_t delimiter, int size )
{
	static constexpr float SIZES[] = { 1.2F, 1.8F, 2.4F, 3.0F };
	const float em_tall = SIZES[std::clamp( size, 1, 4 ) - 1];
	return Expr::from(
		[delimiter, em_tall]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			const float em = m.glyph_scale * static_cast<float>( m.font.face().face->units_per_EM );
			auto glyph = create_math_large_op( m.font, delimiter, em_tall * em / m.glyph_scale );
			*glyph *= m.glyph_scale;
			return glyph;
		} );
}

Expr parens( Expr content )
{
	return fenced( U'(', std::move( content ), U')' );
}

Expr brackets( Expr content )
{
	return fenced( U'[', std::move( content ), U']' );
}

Expr braces( Expr content )
{
	return fenced( U'{', std::move( content ), U'}' );
}

Expr bars( Expr content )
{
	return fenced( U'|', std::move( content ), U'|' );
}

Expr overline( Expr content )
{
	return Expr::from(
		[content = std::move( content )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			return std::make_unique<Overline>( m.font, m.glyph_scale, content.build( context, style.cramp() ) );
		} );
}

Expr underline( Expr content )
{
	return Expr::from(
		[content = std::move( content )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			return std::make_unique<Underline>( m.font, m.glyph_scale, content.build( context, style ) );
		} );
}

Expr overbrace( Expr content )
{
	return Expr::from(
		[content = std::move( content )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			return std::make_unique<Overbrace>( m.font, m.glyph_scale, content.build( context, style.cramp() ) );
		} );
}

Expr underbrace( Expr content )
{
	return Expr::from(
		[content = std::move( content )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			return std::make_unique<Underbrace>( m.font, m.glyph_scale, content.build( context, style ) );
		} );
}

namespace
{
Expr wide_accent( Expr base, char32_t accent_glyph )
{
	return Expr::from(
		[base = std::move( base ), accent_glyph]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			return std::make_unique<WideAccent>( m.font, m.glyph_scale, base.build( context, style.cramp() ), accent_glyph );
		} );
}
}

Expr widehat( Expr base )
{
	return wide_accent( std::move( base ), U'\u0302' );
}

Expr widetilde( Expr base )
{
	return wide_accent( std::move( base ), U'\u0303' );
}

Expr overrightarrow( Expr base )
{
	return wide_accent( std::move( base ), U'\u20D7' );
}

Expr overleftarrow( Expr base )
{
	return wide_accent( std::move( base ), U'\u20D6' );
}

Expr accent( Expr base, char32_t accent_glyph )
{
	return Expr::from(
		[base = std::move( base ), accent_glyph]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			return std::make_unique<Accent>(
				m.font, m.glyph_scale, base.build( context, style.cramp() ), accent_glyph );
		} );
}

Expr hat( Expr base )
{
	return accent( std::move( base ), U'ˆ' );
}

Expr tilde( Expr base )
{
	return accent( std::move( base ), U'˜' );
}

Expr bar( Expr base )
{
	return accent( std::move( base ), U'¯' );
}

Expr dot( Expr base )
{
	return accent( std::move( base ), U'˙' );
}

Expr vec( Expr base )
{
	return accent( std::move( base ), U'⃗' );
}

// ---- Dots ----------------------------------------------------------------------------------

Expr ldots()
{
	return text( "…" ); // …
}

Expr cdots()
{
	return text( "⋯" ); // ⋯
}

Expr vdots()
{
	return text( "⋮" ); // ⋮
}

Expr ddots()
{
	return text( "⋱" ); // ⋱
}

// ---- Phantom -------------------------------------------------------------------------------

namespace
{
// An empty box the size of `built` (or just its width/height), so it takes the same room
// without drawing anything
std::unique_ptr<Visual> make_phantom( std::unique_ptr<Visual> &&built, bool keep_width, bool keep_height )
{
	const AABB box = layout_extent( *built );
	auto placeholder = std::make_unique<Group>();
	placeholder->extent = AABB{
		{ keep_width ? box.min.x() : 0.F, keep_height ? box.min.y() : 0.F },
		{ keep_width ? box.max.x() : 0.F, keep_height ? box.max.y() : 0.F } };
	placeholder->baseline = keep_height ? built->baseline : 0.F;
	placeholder->math_axis = built->math_axis;
	return placeholder;
}
}

Expr phantom( Expr content )
{
	return Expr::from(
		[content = std::move( content )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{ return make_phantom( content.build( context, style ), true, true ); } );
}

Expr hphantom( Expr content )
{
	return Expr::from(
		[content = std::move( content )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{ return make_phantom( content.build( context, style ), true, false ); } );
}

Expr vphantom( Expr content )
{
	return Expr::from(
		[content = std::move( content )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{ return make_phantom( content.build( context, style ), false, true ); } );
}

// ---- Rows --------------------------------------------------------------------------------

namespace
{
Atom make_atom( AtomClass atom, Expr expr )
{
	Atom result;
	result.atom = atom;
	result.expr = std::move( expr );
	return result;
}
}

Atom ord( Expr expr ) { return make_atom( AtomClass::ORD, std::move( expr ) ); }
Atom op( Expr expr ) { return make_atom( AtomClass::OP, std::move( expr ) ); }
Atom bin( Expr expr ) { return make_atom( AtomClass::BIN, std::move( expr ) ); }
Atom rel( Expr expr ) { return make_atom( AtomClass::REL, std::move( expr ) ); }
Atom opening( Expr expr ) { return make_atom( AtomClass::OPEN, std::move( expr ) ); }
Atom closing( Expr expr ) { return make_atom( AtomClass::CLOSE, std::move( expr ) ); }
Atom punct( Expr expr ) { return make_atom( AtomClass::PUNCT, std::move( expr ) ); }
Atom inner( Expr expr ) { return make_atom( AtomClass::INNER, std::move( expr ) ); }

Atom ord( std::string characters ) { return ord( text( std::move( characters ) ) ); }
Atom op( std::string characters ) { return op( text( std::move( characters ) ) ); }
Atom bin( std::string characters ) { return bin( text( std::move( characters ) ) ); }
Atom rel( std::string characters ) { return rel( text( std::move( characters ) ) ); }
Atom opening( std::string characters ) { return opening( text( std::move( characters ) ) ); }
Atom closing( std::string characters ) { return closing( text( std::move( characters ) ) ); }
Atom punct( std::string characters ) { return punct( text( std::move( characters ) ) ); }
Atom inner( std::string characters ) { return inner( text( std::move( characters ) ) ); }

Atom space( float mu )
{
	Atom result;
	result.is_space = true;
	result.space = mu;
	return result;
}

Expr row( std::vector<Atom> atoms )
{
	return Expr::from(
		[atoms = std::move( atoms )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			auto result = std::make_unique<Row>( m.font, m.glyph_scale, style );
			for ( const Atom &atom: atoms )
			{
				if ( atom.is_space )
				{
					result->append_space( atom.space );
				}
				else
				{
					result->append( atom.atom, atom.expr.build( context, style ) );
				}
			}
			return std::unique_ptr<Visual>( std::move( result ) );
		} );
}

// ---- Grids -------------------------------------------------------------------------------

Expr matrix( std::vector<std::vector<Expr>> rows, MatrixOptions options )
{
	return Expr::from(
		[rows = std::move( rows ), options = std::move( options )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			auto grid = std::make_unique<Matrix>( m.font, m.glyph_scale, options );
			for ( const auto &row_cells: rows )
			{
				std::vector<std::unique_ptr<Visual>> cells;
				cells.reserve( row_cells.size() );
				for ( const auto &cell: row_cells )
				{
					if ( cell )
					{
						cells.push_back( cell.build( context, style.cell() ) );
					}
					else
					{
						// An empty cell takes no room
						auto empty = std::make_unique<Group>();
						AABB box;
						box.add( { 0.F, 0.F } );
						empty->extent = box;
						cells.push_back( std::move( empty ) );
					}
				}
				grid->append_row( std::move( cells ) );
			}
			return std::unique_ptr<Visual>( std::move( grid ) );
		} );
}

Expr pmatrix( std::vector<std::vector<Expr>> rows, MatrixOptions options )
{
	return parens( matrix( std::move( rows ), std::move( options ) ) );
}

Expr bmatrix( std::vector<std::vector<Expr>> rows, MatrixOptions options )
{
	return brackets( matrix( std::move( rows ), std::move( options ) ) );
}

Expr vmatrix( std::vector<std::vector<Expr>> rows, MatrixOptions options )
{
	return bars( matrix( std::move( rows ), std::move( options ) ) );
}

Expr cases( std::vector<std::vector<Expr>> rows, MatrixOptions options )
{
	if ( options.align.empty() )
	{
		options.align.assign( 16, ColumnAlign::LEFT );
	}
	return fenced( U'{', matrix( std::move( rows ), std::move( options ) ), 0 );
}

namespace
{
// A cell that is only room, `width` wide
std::unique_ptr<Visual> spacer_cell( float width )
{
	auto empty = std::make_unique<Group>();
	AABB box;
	box.add( { 0.F, 0.F } );
	box.add( { width, 0.F } );
	empty->extent = box;
	return empty;
}
}

Expr aligned( std::vector<std::vector<Expr>> rows, AlignedOptions options )
{
	return Expr::from(
		[rows = std::move( rows ), options = std::move( options )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			const auto &m = context.metrics( style );
			const float em = m.glyph_scale * static_cast<float>( m.font.face().face->units_per_EM );

			size_t pairs = 1;
			for ( const auto &row: rows )
			{
				pairs = std::max( pairs, ( row.size() + 1 ) / 2 );
			}
			const bool numbers = options.numbered || !options.tags.empty();

			// Columns: right, left, [gap, right, left]..., then (if numbered) a gap and the number at the right
			MatrixOptions grid_options;
			grid_options.column_sep_em = 0.F;
			grid_options.baseline_skip_em = options.baseline_skip_em;
			for ( size_t p = 0; p < pairs; ++p )
			{
				if ( p > 0 )
				{
					grid_options.align.push_back( ColumnAlign::LEFT ); // (the gap)
				}
				grid_options.align.push_back( ColumnAlign::RIGHT );
				grid_options.align.push_back( ColumnAlign::LEFT );
			}
			if ( numbers )
			{
				grid_options.align.push_back( ColumnAlign::LEFT ); // (the gap)
				grid_options.align.push_back( ColumnAlign::RIGHT );
			}

			auto grid = std::make_unique<Matrix>( m.font, m.glyph_scale, grid_options );
			int number = 0;
			for ( size_t r = 0; r < rows.size(); ++r )
			{
				std::vector<std::unique_ptr<Visual>> cells;
				auto add_cell = [&]( const Expr &cell )
				{
					cells.push_back( cell ? cell.build( context, style ) : spacer_cell( 0.F ) );
				};
				for ( size_t p = 0; p < pairs; ++p )
				{
					if ( p > 0 )
					{
						cells.push_back( spacer_cell( options.pair_gap_em * em ) );
					}
					for ( size_t k = 0; k < 2; ++k )
					{
						const size_t i = p * 2 + k;
						Expr cell = i < rows[r].size() ? rows[r][i] : Expr();
						if ( k == 1 && cell && options.relation_space )
						{
							cell = row( { space( Row::THICK ), ord( std::move( cell ) ) } );
						}
						add_cell( cell );
					}
				}
				if ( numbers )
				{
					std::string label;
					if ( r < options.tags.size() )
					{
						label = options.tags[r];
					}
					else if ( options.numbered )
					{
						label = "(" + std::to_string( ++number ) + ")";
					}
					cells.push_back( spacer_cell( options.number_gap_em * em ) );
					if ( label.empty() )
					{
						cells.push_back( spacer_cell( 0.F ) );
					}
					else
					{
						cells.push_back( upright( label ).build( context, style ) );
					}
				}
				grid->append_row( std::move( cells ) );
			}
			return std::unique_ptr<Visual>( std::move( grid ) );
		} );
}

// ---- Tags --------------------------------------------------------------------------------

Expr tag( std::string name, Expr content )
{
	if ( !content )
	{
		return Expr();
	}
	return Expr::from(
		[name = std::move( name ), content = std::move( content )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			auto visual = content.build( context, style );
			visual->tags.push_back( name );
			return visual;
		} );
}

// ---- Marking a part ------------------------------------------------------------------------

namespace
{
// A group that takes the room, and has the baseline and axis, of what it holds, whatever is drawn round it
std::unique_ptr<Group> wrap_like( std::unique_ptr<Visual> content, std::unique_ptr<Visual> &&mark_or_null, AABB extent )
{
	auto group = std::make_unique<Group>();
	group->baseline = content->baseline;
	group->math_axis = content->math_axis;
	group->extent = extent;
	group->add( std::move( content ) );
	if ( mark_or_null )
	{
		group->add( std::move( mark_or_null ) );
	}
	return group;
}

// A line as thick as body text's rules are: a twentieth of an em
float rule_thickness( const Context &context, MathStyle style )
{
	return 0.05F * context.theme().get_stack( context.font_style( style ) ).font( 0 ).scale;
}
}

Expr colored( Color color, Expr content )
{
	if ( !content )
	{
		return Expr();
	}
	return Expr::from(
		[color, content = std::move( content )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			auto visual = content.build( context, style );
			visual->color = color;
			visual->stroke_color = color;
			return visual;
		} );
}

Expr cancel( Expr content )
{
	if ( !content )
	{
		return Expr();
	}
	return Expr::from(
		[content = std::move( content )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			auto built = content.build( context, style );
			const AABB box = layout_extent( *built );
			auto line = std::make_unique<Line>();
			line->start = box.min;
			line->end = box.max;
			line->stroke_thickness = rule_thickness( context, style );
			return wrap_like( std::move( built ), std::move( line ), box );
		} );
}

Expr boxed( Expr content, float padding )
{
	if ( !content )
	{
		return Expr();
	}
	return Expr::from(
		[padding, content = std::move( content )]( const Context &context, MathStyle style ) -> std::unique_ptr<Visual>
		{
			auto built = content.build( context, style );
			const AABB box = layout_extent( *built );
			const float pad = padding * context.theme().get_stack( context.font_style( style ) ).font( 0 ).scale;
			AABB outer{ { box.min.x() - pad, box.min.y() - pad }, { box.max.x() + pad, box.max.y() + pad } };
			auto frame = std::make_unique<Rectangle>();
			frame->half_width = ( outer.max.x() - outer.min.x() ) / 2.F;
			frame->half_height = ( outer.max.y() - outer.min.y() ) / 2.F;
			frame->position = { ( outer.min.x() + outer.max.x() ) / 2.F, ( outer.min.y() + outer.max.y() ) / 2.F };
			frame->color = { 1.F, 1.F, 1.F, 0.F }; // an outline, not a filled box
			frame->stroke_thickness = rule_thickness( context, style );
			// The frame first: it does not cover what it is round
			auto group = std::make_unique<Group>();
			group->baseline = built->baseline;
			group->math_axis = built->math_axis;
			group->extent = outer;
			group->add( std::move( frame ) );
			group->add( std::move( built ) );
			return group;
		} );
}

// ---- Style -------------------------------------------------------------------------------

Expr with_style( MathStyle forced, Expr content )
{
	return Expr::from(
		[forced, content = std::move( content )]( const Context &context, MathStyle ) -> std::unique_ptr<Visual>
		{ return content.build( context, forced ); } );
}

}
