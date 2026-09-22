#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <math/Expr.h>
#include <themes/NewComputerModern.h>
#include <visuals/Accent.h>
#include <visuals/Bars.h>
#include <visuals/Delimited.h>
#include <visuals/Fraction.h>
#include <visuals/Group.h>
#include <visuals/Layout.h>
#include <visuals/Limits.h>
#include <visuals/MathGlyph.h>
#include <visuals/Radical.h>
#include <visuals/Scripts.h>
#include <visuals/Tags.h>

#include <algorithm>
#include <optional>
#include <stdexcept>

using Catch::Approx;
using namespace math;

namespace
{
struct Fixture
{
	Theme theme = NewComputerModern::create_theme();
	Context context{ theme };
	const Context::Metrics &body = context.metrics( MathStyle::text() );
	float script_percent = body.font.script_percent_scale_down() / 100.F;
	float script_script_percent = body.font.script_script_percent_scale_down() / 100.F;

	// A stand-in that records the style it was built in
	static Expr probe( std::optional<MathStyle> &slot )
	{
		return Expr::from(
			[&slot]( const Context &, MathStyle style ) -> std::unique_ptr<Visual>
			{
				slot = style;
				auto group = std::make_unique<Group>();
				AABB box;
				box.add( { 0.F, 0.F } );
				box.add( { 0.1F, 0.1F } );
				group->extent = box;
				return group;
			} );
	}
};

// The heights of the characters in a piece of text (not the delimiters, signs and other
// glyphs that come from the math font's large variants)
void collect_heights( const Visual &visual, std::vector<float> &heights )
{
	if ( dynamic_cast<const MathGlyph *>( &visual ) )
	{
		return;
	}
	if ( const auto *glyph = dynamic_cast<const Glyph *>( &visual ) )
	{
		if ( glyph->extent )
		{
			heights.push_back( glyph->extent->height() );
		}
		return;
	}
	if ( const auto *group = dynamic_cast<const Group *>( &visual ) )
	{
		for ( const auto &child: *group )
		{
			collect_heights( *child, heights );
		}
	}
}

std::vector<float> heights_of( const Visual &visual )
{
	std::vector<float> heights;
	collect_heights( visual, heights );
	std::sort( heights.begin(), heights.end() );
	return heights;
}

const Group &child_group( const Group &group, size_t i )
{
	return static_cast<const Group &>( **( group.begin() + static_cast<std::ptrdiff_t>( i ) ) );
}
}

TEST_CASE( "math::Context gives the font for each size", "[math]" )
{
	Fixture f;

	SECTION( "display and text share a size, so they share metrics" )
	{
		CHECK( &f.context.metrics( MathStyle::display() ) == &f.context.metrics( MathStyle::text() ) );
	}

	SECTION( "scripts are the font's percentages of the body size" )
	{
		CHECK( f.context.metrics( MathStyle::script() ).glyph_scale ==
		       Approx( f.body.glyph_scale * f.script_percent ) );
		CHECK( f.context.metrics( MathStyle::script_script() ).glyph_scale ==
		       Approx( f.body.glyph_scale * f.script_script_percent ) );
		CHECK( f.script_script_percent < f.script_percent );
	}

	SECTION( "metrics are kept, not rebuilt" )
	{
		CHECK( &f.context.metrics( MathStyle::script() ) == &f.context.metrics( MathStyle::script().cramp() ) );
	}
}

TEST_CASE( "math::Expr basics", "[math]" )
{
	Fixture f;

	SECTION( "an empty expression is false and can't be built" )
	{
		CHECK_FALSE( Expr() );
		CHECK( text( "x" ) );
		CHECK_THROWS_AS( Expr().build( f.context ), std::invalid_argument );
	}

	SECTION( "text is math italic, upright() is not" )
	{
		auto italic = text( "x" ).build( f.context );
		auto plain = upright( "x" ).build( f.context );
		auto glyph_index = []( const Visual &visual )
		{
			const Visual *v = &visual;
			while ( const auto *group = dynamic_cast<const Group *>( v ) )
			{
				v = group->begin()->get();
			}
			return static_cast<const Glyph *>( v )->index;
		};
		CHECK( glyph_index( *italic ) == f.body.font.get_glyph_index( 0x1D465 ) );
		CHECK( glyph_index( *plain ) == f.body.font.get_glyph_index( U'x' ) );
	}

	SECTION( "the other math alphabets: script, fraktur, double-struck, sans, mono" )
	{
		auto glyph_index = []( const Visual &visual )
		{
			const Visual *v = &visual;
			while ( const auto *group = dynamic_cast<const Group *>( v ) )
			{
				v = group->begin()->get();
			}
			return static_cast<const Glyph *>( v )->index;
		};
		CHECK( glyph_index( *script( "A" ).build( f.context ) ) ==
		       f.body.font.get_glyph_index( 0x1D49C ) );
		CHECK( glyph_index( *fraktur( "C" ).build( f.context ) ) ==
		       f.body.font.get_glyph_index( 0x212D ) );
		CHECK( glyph_index( *double_struck( "R" ).build( f.context ) ) ==
		       f.body.font.get_glyph_index( 0x211D ) );
		CHECK( glyph_index( *sans( "a" ).build( f.context ) ) ==
		       f.body.font.get_glyph_index( 0x1D5BA ) );
		CHECK( glyph_index( *mono( "a" ).build( f.context ) ) ==
		       f.body.font.get_glyph_index( 0x1D68A ) );
	}

	SECTION( "dots" )
	{
		auto glyph_index = []( const Visual &visual )
		{
			const Visual *v = &visual;
			while ( const auto *group = dynamic_cast<const Group *>( v ) )
			{
				v = group->begin()->get();
			}
			return static_cast<const Glyph *>( v )->index;
		};
		CHECK( glyph_index( *ldots().build( f.context ) ) == f.body.font.get_glyph_index( 0x2026 ) );
		CHECK( glyph_index( *cdots().build( f.context ) ) == f.body.font.get_glyph_index( 0x22EF ) );
		CHECK( glyph_index( *vdots().build( f.context ) ) == f.body.font.get_glyph_index( 0x22EE ) );
		CHECK( glyph_index( *ddots().build( f.context ) ) == f.body.font.get_glyph_index( 0x22F1 ) );
	}

	SECTION( "an expression can be built again, in any style" )
	{
		Expr formula = text( "22" );
		auto a = formula.build( f.context );
		auto b = formula.build( f.context );
		CHECK( a.get() != b.get() );
		CHECK( layout_extent( *a ).width() == Approx( layout_extent( *b ).width() ) );

		auto small = formula.build( f.context, MathStyle::script() );
		CHECK( layout_extent( *small ).width() ==
		       Approx( layout_extent( *a ).width() * f.script_percent ) );
	}
}

TEST_CASE( "math::Expr builds the right constructs", "[math]" )
{
	Fixture f;
	auto one = []() { return text( "1" ); };

	SECTION( "fractions and roots" )
	{
		CHECK( dynamic_cast<Fraction *>( fraction( one(), one() ).build( f.context ).get() ) != nullptr );

		auto radicand_in_radical = radical( fraction( one(), one() ) ).build( f.context );
		auto *rad = dynamic_cast<Radical *>( radicand_in_radical.get() );
		REQUIRE( rad != nullptr );
		CHECK( dynamic_cast<const Fraction *>( rad->begin()->get() ) != nullptr );

		auto with_index = root( one(), one() ).build( f.context );
		REQUIRE( dynamic_cast<Radical *>( with_index.get() ) != nullptr );
		CHECK( static_cast<Radical *>( with_index.get() )->size() == 4 ); // radicand, overline, sign, index
	}

	SECTION( "scripts" )
	{
		auto sup = superscript( one(), one() ).build( f.context );
		auto sub = subscript( one(), one() ).build( f.context );
		auto both = scripts( one(), one(), one() ).build( f.context );
		CHECK( static_cast<Scripts *>( sup.get() )->size() == 2 );
		CHECK( static_cast<Scripts *>( sub.get() )->size() == 2 );
		CHECK( static_cast<Scripts *>( both.get() )->size() == 3 );
		CHECK( dynamic_cast<Scripts *>( sup.get() ) != nullptr );
	}

	SECTION( "large operators: over and under in display, at the side in text and for integrals" )
	{
		CHECK( dynamic_cast<Limits *>( sum( one(), one() ).build( f.context ).get() ) != nullptr );
		CHECK( dynamic_cast<Limits *>( product( one(), one() ).build( f.context ).get() ) != nullptr );
		CHECK( dynamic_cast<Scripts *>( integral( one(), one() ).build( f.context ).get() ) != nullptr );
		CHECK( dynamic_cast<Scripts *>(
		           sum( one(), one() ).build( f.context, MathStyle::text() ).get() ) != nullptr );
		CHECK( dynamic_cast<Limits *>(
		           large_operator( U'∫', one(), one(), LimitPlacement::OVER_UNDER ).build( f.context ).get() ) != nullptr );
	}

	SECTION( "named operators: moving limits behave like large operators, others stay at the side" )
	{
		CHECK( dynamic_cast<Limits *>( lim( one() ).build( f.context ).get() ) != nullptr );
		CHECK( dynamic_cast<Scripts *>( lim( one() ).build( f.context, MathStyle::text() ).get() ) != nullptr );
		CHECK( dynamic_cast<Scripts *>( sin( one() ).build( f.context ).get() ) != nullptr );
		CHECK( dynamic_cast<Scripts *>( log( one() ).build( f.context ).get() ) != nullptr );
		CHECK( dynamic_cast<Scripts *>( named_operator( "argmax", one() ).build( f.context ).get() ) != nullptr );
		CHECK( dynamic_cast<Limits *>(
		           named_operator( "argmax", one(), Expr(), true ).build( f.context ).get() ) != nullptr );
	}

	SECTION( "delimiters, bars and accents" )
	{
		auto fenced_content = parens( one() ).build( f.context );
		REQUIRE( dynamic_cast<Delimited *>( fenced_content.get() ) != nullptr );
		CHECK( static_cast<Delimited *>( fenced_content.get() )->size() == 3 );

		auto binom = binomial( one(), one() ).build( f.context );
		auto *delimited = dynamic_cast<Delimited *>( binom.get() );
		REQUIRE( delimited != nullptr );
		CHECK( child_group( *delimited, 1 ).size() == 2 ); // numerator and denominator, no rule

		CHECK( dynamic_cast<Overline *>( overline( one() ).build( f.context ).get() ) != nullptr );
		CHECK( dynamic_cast<Underline *>( underline( one() ).build( f.context ).get() ) != nullptr );
		CHECK( dynamic_cast<Accent *>( hat( one() ).build( f.context ).get() ) != nullptr );
		CHECK( dynamic_cast<Accent *>( vec( text( "v" ) ).build( f.context ).get() ) != nullptr );
		CHECK( dynamic_cast<Overbrace *>( overbrace( one() ).build( f.context ).get() ) != nullptr );
		CHECK( dynamic_cast<Underbrace *>( underbrace( one() ).build( f.context ).get() ) != nullptr );
	}

	SECTION( "over/underbrace stretch to fit their content and stay centered on it" )
	{
		auto narrow = overbrace( text( "x" ) ).build( f.context );
		auto wide = overbrace( row( { ord( "x" ), bin( "+" ), ord( "y" ), bin( "+" ), ord( "z" ) } ) ).build( f.context );
		CHECK( layout_extent( *wide ).width() > layout_extent( *narrow ).width() );

		// The content's own width, and the brace centered under/over it
		auto over = static_cast<Overbrace *>( wide.get() );
		const AABB content_box = layout_extent( *find_tag( *over, "content" ) );
		const AABB brace_box = layout_extent( *find_tag( *over, "brace" ) );
		CHECK( brace_box.width() == Approx( content_box.width() ).margin( 0.05F ) );
		CHECK( ( brace_box.min.x() + brace_box.max.x() ) / 2.F ==
		       Approx( ( content_box.min.x() + content_box.max.x() ) / 2.F ).margin( 0.05F ) );

		// The brace sits above the content, an underbrace's below it
		CHECK( brace_box.min.y() >= content_box.max.y() );
		auto under = underbrace( text( "x" ) ).build( f.context );
		const AABB under_content = layout_extent( *find_tag( *under, "content" ) );
		const AABB under_brace = layout_extent( *find_tag( *under, "brace" ) );
		CHECK( under_brace.max.y() <= under_content.min.y() );
	}

	SECTION( "rows and grids" )
	{
		auto r = row( { ord( "x" ), bin( "+" ), ord( "y" ) } ).build( f.context );
		REQUIRE( dynamic_cast<Row *>( r.get() ) != nullptr );
		CHECK( static_cast<Row *>( r.get() )->size() == 3 );

		auto grid = pmatrix( { { text( "a" ), text( "b" ) }, { text( "c" ), text( "d" ) } } ).build( f.context );
		auto *fenced_grid = dynamic_cast<Delimited *>( grid.get() );
		REQUIRE( fenced_grid != nullptr );
		CHECK( child_group( *fenced_grid, 1 ).size() == 4 );

		auto det = vmatrix( { { text( "a" ) } } ).build( f.context );
		CHECK( static_cast<Delimited *>( det.get() )->size() == 3 );

		auto choice = cases( { { text( "1" ), text( "x" ) }, { text( "0" ), text( "y" ) } } ).build( f.context );
		CHECK( static_cast<Delimited *>( choice.get() )->size() == 2 ); // brace, grid: nothing on the right
	}
}

TEST_CASE( "math::Expr hands each part its style", "[math]" )
{
	Fixture f;
	const MathStyle D = MathStyle::display();
	std::optional<MathStyle> a;
	std::optional<MathStyle> b;

	SECTION( "fractions and binomials" )
	{
		(void) fraction( Fixture::probe( a ), Fixture::probe( b ) ).build( f.context );
		CHECK( *a == D.numerator() );
		CHECK( *b == D.denominator() );

		a.reset();
		b.reset();
		(void) binomial( Fixture::probe( a ), Fixture::probe( b ) ).build( f.context );
		CHECK( *a == D.numerator() );
		CHECK( *b == D.denominator() );
	}

	SECTION( "a fraction inside a fraction steps down again" )
	{
		(void) fraction( fraction( Fixture::probe( a ), Fixture::probe( b ) ), text( "1" ) ).build( f.context );
		CHECK( *a == D.numerator().numerator() );
		CHECK( *b == D.numerator().denominator() );
		CHECK( a->level == MathStyle::Level::SCRIPT );
	}

	SECTION( "radicands are cramped, indices are scriptscript" )
	{
		(void) radical( Fixture::probe( a ) ).build( f.context );
		CHECK( *a == D.radicand() );

		a.reset();
		(void) root( Fixture::probe( a ), Fixture::probe( b ) ).build( f.context );
		CHECK( *a == D.index() );
		CHECK( *b == D.radicand() );
	}

	SECTION( "scripts" )
	{
		(void) scripts( Fixture::probe( a ), Fixture::probe( b ), text( "1" ) ).build( f.context );
		CHECK( *a == D );
		CHECK( *b == D.superscript() );

		a.reset();
		b.reset();
		(void) scripts( text( "1" ), Expr(), Fixture::probe( a ) ).build( f.context );
		CHECK( *a == D.subscript() );
		CHECK( a->cramped );
	}

	SECTION( "limits of a large operator" )
	{
		(void) sum( Fixture::probe( a ), Fixture::probe( b ) ).build( f.context );
		CHECK( *a == D.subscript() );
		CHECK( *b == D.superscript() );
	}

	SECTION( "limits of a named operator" )
	{
		(void) lim( Fixture::probe( a ) ).build( f.context );
		CHECK( *a == D.subscript() );

		a.reset();
		(void) log( Fixture::probe( a ) ).build( f.context );
		CHECK( *a == D.subscript() );
	}

	SECTION( "overlines and accents cramp their content, underlines and delimiters do not" )
	{
		(void) overline( Fixture::probe( a ) ).build( f.context );
		CHECK( *a == D.cramp() );

		a.reset();
		(void) hat( Fixture::probe( a ) ).build( f.context );
		CHECK( *a == D.cramp() );

		a.reset();
		(void) underline( Fixture::probe( a ) ).build( f.context );
		CHECK( *a == D );

		a.reset();
		(void) parens( Fixture::probe( a ) ).build( f.context );
		CHECK( *a == D );

		a.reset();
		(void) overbrace( Fixture::probe( a ) ).build( f.context );
		CHECK( *a == D.cramp() );

		a.reset();
		(void) underbrace( Fixture::probe( a ) ).build( f.context );
		CHECK( *a == D );
	}

	SECTION( "matrix cells are text style in a displayed matrix" )
	{
		(void) matrix( { { Fixture::probe( a ), Fixture::probe( b ) } } ).build( f.context );
		CHECK( *a == D.cell() );
		CHECK( *b == D.cell() );
		CHECK( a->level == MathStyle::Level::TEXT );
	}

	SECTION( "row atoms keep the row's style" )
	{
		(void) row( { ord( Fixture::probe( a ) ), rel( "=" ), ord( Fixture::probe( b ) ) } ).build( f.context );
		CHECK( *a == D );
		CHECK( *b == D );
	}

	SECTION( "the style passed to build() is where it starts" )
	{
		(void) fraction( Fixture::probe( a ), Fixture::probe( b ) ).build( f.context, MathStyle::text() );
		CHECK( *a == MathStyle::text().numerator() );
		CHECK( *b == MathStyle::text().denominator() );
	}

	SECTION( "with_style overrides it for a part" )
	{
		(void) row( { ord( with_style( MathStyle::script(), Fixture::probe( a ) ) ), ord( Fixture::probe( b ) ) } )
			.build( f.context );
		CHECK( *a == MathStyle::script() );
		CHECK( *b == D );
	}
}

TEST_CASE( "math::Expr sizes and constants follow the style", "[math]" )
{
	Fixture f;
	const float display_scale = f.body.glyph_scale;

	SECTION( "each level of a nested fraction is smaller" )
	{
		auto nested = fraction( fraction( text( "2" ), text( "2" ) ), text( "2" ) ).build( f.context );
		auto heights = heights_of( *nested );
		REQUIRE( heights.size() == 3 );
		// the two inside the inner fraction are script size, the outer denominator is body size
		CHECK( heights[0] == Approx( heights[1] ) );
		CHECK( heights[0] / heights[2] == Approx( f.script_percent ) );
	}

	SECTION( "a superscript is script size" )
	{
		auto sup = superscript( text( "2" ), text( "2" ) ).build( f.context );
		auto heights = heights_of( *sup );
		REQUIRE( heights.size() == 2 );
		CHECK( heights[0] / heights[1] == Approx( f.script_percent ) );
	}

	SECTION( "the index of a root is scriptscript" )
	{
		auto with_index = root( text( "2" ), text( "2" ) ).build( f.context );
		auto heights = heights_of( *with_index );
		REQUIRE( heights.size() == 2 );
		CHECK( heights[0] / heights[1] == Approx( f.script_script_percent ) );
	}

	SECTION( "display fractions are more open than text ones" )
	{
		auto in_display = fraction( text( "1" ), text( "2" ) ).build( f.context );
		auto in_text = fraction( text( "1" ), text( "2" ) ).build( f.context, MathStyle::text() );
		const float display_up = ( *static_cast<Group &>( *in_display ).begin() )->position.y();
		const float text_up = ( *static_cast<Group &>( *in_text ).begin() )->position.y();

		CHECK( display_up == Approx( f.body.font.fraction_numerator_display_style_shift_up() * display_scale ) );
		CHECK( text_up == Approx( f.body.font.fraction_numerator_shift_up() * display_scale ) );
		CHECK( display_up > text_up );
	}

	SECTION( "superscripts under a radical sit lower (cramped)" )
	{
		auto plain = superscript( text( "y" ), text( "2" ) ).build( f.context );
		auto under_radical = radical( superscript( text( "y" ), text( "2" ) ) ).build( f.context );

		const float plain_up = ( *( static_cast<Group &>( *plain ).begin() + 1 ) )->position.y();
		const Group &inner_scripts = child_group( static_cast<Group &>( *under_radical ), 0 );
		const float cramped_up = ( *( inner_scripts.begin() + 1 ) )->position.y();

		CHECK( plain_up == Approx( f.body.font.superscript_shift() * display_scale ) );
		CHECK( cramped_up == Approx( f.body.font.superscript_shift_cramped() * display_scale ) );
		CHECK( cramped_up < plain_up );
	}

	SECTION( "row spacing follows the style: script rows have no medium space" )
	{
		auto gap = []( const Visual &visual )
		{
			const auto &group = static_cast<const Group &>( visual );
			const AABB first = layout_extent( **group.begin() );
			const AABB second = layout_extent( **( group.begin() + 1 ) );
			return second.min.x() - first.max.x();
		};

		const float mu = display_scale * static_cast<float>( f.body.font.face().face->units_per_EM ) / 18.F;
		auto in_display = row( { ord( "x" ), bin( "+" ), ord( "y" ) } ).build( f.context );
		CHECK( gap( *in_display ) == Approx( 4.F * mu ).margin( 1e-4 ) );

		auto in_script = row( { ord( "x" ), bin( "+" ), ord( "y" ) } ).build( f.context, MathStyle::script() );
		CHECK( gap( *in_script ) == Approx( 0.F ).margin( 1e-4 ) );
	}

	SECTION( "explicit space in a row" )
	{
		auto spaced = row( { ord( "x" ), space( Row::THIN ), ord( "y" ) } ).build( f.context );
		const auto &group = static_cast<const Group &>( *spaced );
		const AABB first = layout_extent( **group.begin() );
		const AABB second = layout_extent( **( group.begin() + 1 ) );
		const float mu = display_scale * static_cast<float>( f.body.font.face().face->units_per_EM ) / 18.F;
		CHECK( second.min.x() - first.max.x() == Approx( 3.F * mu ) );
	}
}
