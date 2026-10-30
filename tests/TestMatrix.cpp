#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <themes/AcademicClassic.h>
#include <visuals/Circle.h>
#include <visuals/Delimited.h>
#include <visuals/Layout.h>
#include <visuals/Matrix.h>

#include <algorithm>

using Catch::Approx;

namespace
{
struct Fixture
{
	Theme theme = AcademicClassic::create_theme();
	MathFonts math_fonts = theme.get_math( FontStyle{ Intent::MATH } );
	MathFont font{ math_fonts.roman() };
	float s = math_fonts.roman().scale / static_cast<float>( math_fonts.roman().face->units_per_EM );
	float em = math_fonts.roman().scale;
	float axis = font.axis_height() * s;

	static std::unique_ptr<Circle> circle( float radius )
	{
		auto c = std::make_unique<Circle>();
		c->radius = radius;
		return c;
	}
};

// The layout box of the i-th cell, in the order the cells were added
AABB cell_box( const Matrix &matrix, size_t i )
{
	return layout_extent( **( matrix.begin() + static_cast<std::ptrdiff_t>( i ) ) );
}

float center_x( const AABB &box )
{
	return ( box.min.x() + box.max.x() ) / 2.F;
}
}

TEST_CASE( "Matrix: columns", "[visuals]" )
{
	Fixture f;
	const float sep = f.em; // one em between columns

	SECTION( "cells sit in their columns, a column gap apart" )
	{
		Matrix m( f.font, f.s );
		m.append_row( make_cells( Fixture::circle( 0.1F ), Fixture::circle( 0.1F ) ) );
		m.append_row( make_cells( Fixture::circle( 0.1F ), Fixture::circle( 0.1F ) ) );
		REQUIRE( m.size() == 4 );

		CHECK( center_x( cell_box( m, 0 ) ) == Approx( 0.1F ) );
		CHECK( center_x( cell_box( m, 1 ) ) == Approx( 0.2F + sep + 0.1F ) );
		CHECK( center_x( cell_box( m, 2 ) ) == Approx( 0.1F ) );
		CHECK( center_x( cell_box( m, 3 ) ) == Approx( 0.2F + sep + 0.1F ) );

		REQUIRE( m.extent.has_value() );
		CHECK( m.extent->min.x() == Approx( 0.F ) );
		CHECK( m.extent->max.x() == Approx( 0.4F + sep ) );
	}

	SECTION( "a column is as wide as its widest cell; cells are centered by default" )
	{
		Matrix m( f.font, f.s );
		m.append_row( make_cells( Fixture::circle( 0.1F ) ) );
		m.append_row( make_cells( Fixture::circle( 0.3F ) ) );
		CHECK( center_x( cell_box( m, 0 ) ) == Approx( 0.3F ) );
		CHECK( center_x( cell_box( m, 1 ) ) == Approx( 0.3F ) );
	}

	SECTION( "left and right alignment" )
	{
		Matrix left( f.font, f.s, MatrixOptions{ .align = { ColumnAlign::LEFT } } );
		left.append_row( make_cells( Fixture::circle( 0.1F ) ) );
		left.append_row( make_cells( Fixture::circle( 0.3F ) ) );
		CHECK( cell_box( left, 0 ).min.x() == Approx( 0.F ) );

		Matrix right( f.font, f.s, MatrixOptions{ .align = { ColumnAlign::RIGHT } } );
		right.append_row( make_cells( Fixture::circle( 0.1F ) ) );
		right.append_row( make_cells( Fixture::circle( 0.3F ) ) );
		CHECK( cell_box( right, 0 ).max.x() == Approx( 0.6F ) );
	}

	SECTION( "each column has its own alignment" )
	{
		Matrix m( f.font, f.s, MatrixOptions{ .align = { ColumnAlign::LEFT, ColumnAlign::RIGHT } } );
		m.append_row( make_cells( Fixture::circle( 0.1F ), Fixture::circle( 0.1F ) ) );
		m.append_row( make_cells( Fixture::circle( 0.3F ), Fixture::circle( 0.3F ) ) );
		CHECK( cell_box( m, 0 ).min.x() == Approx( 0.F ) );
		CHECK( cell_box( m, 1 ).max.x() == Approx( 0.6F + sep + 0.6F ) );
	}

	SECTION( "a short row leaves its missing cells empty" )
	{
		Matrix m( f.font, f.s );
		m.append_row( make_cells( Fixture::circle( 0.1F ), Fixture::circle( 0.1F ) ) );
		m.append_row( make_cells( Fixture::circle( 0.1F ) ) );
		REQUIRE( m.size() == 3 );
		CHECK( center_x( cell_box( m, 2 ) ) == Approx( center_x( cell_box( m, 0 ) ) ) );
	}
}

TEST_CASE( "Matrix: rows", "[visuals]" )
{
	Fixture f;
	const float baseline_skip = 1.2F * f.em;
	const float lineskip = 0.1F * f.em;

	auto row_gap = []( const Matrix &m )
	{
		// cells are circles centered on their baselines, so position.y is the row baseline
		return ( *m.begin() )->position.y() - ( *( m.begin() + 1 ) )->position.y();
	};

	SECTION( "short rows are a baseline skip apart" )
	{
		Matrix m( f.font, f.s );
		m.append_row( make_cells( Fixture::circle( 0.05F ) ) );
		m.append_row( make_cells( Fixture::circle( 0.05F ) ) );
		CHECK( row_gap( m ) == Approx( baseline_skip ) );
	}

	SECTION( "tall rows are pushed apart by the smallest gap instead" )
	{
		Matrix m( f.font, f.s );
		m.append_row( make_cells( Fixture::circle( 0.3F ) ) );
		m.append_row( make_cells( Fixture::circle( 0.3F ) ) );
		const float reach = 0.3F + 0.3F;
		const float expected = baseline_skip >= reach ? baseline_skip : reach + lineskip;
		CHECK( row_gap( m ) == Approx( expected ) );
		CHECK( row_gap( m ) > baseline_skip );
	}

	SECTION( "the grid is centered on the math axis" )
	{
		Matrix one( f.font, f.s );
		one.append_row( make_cells( Fixture::circle( 0.2F ) ) );
		CHECK( ( one.extent->min.y() + one.extent->max.y() ) / 2.F == Approx( f.axis ).margin( 1e-4 ) );

		Matrix three( f.font, f.s );
		for ( int i = 0; i < 3; ++i )
		{
			three.append_row( make_cells( Fixture::circle( 0.05F + 0.1F * static_cast<float>( i ) ) ) );
		}
		CHECK( ( three.extent->min.y() + three.extent->max.y() ) / 2.F == Approx( f.axis ).margin( 1e-4 ) );
	}

	SECTION( "appending lays out again, so the middle row stays between its neighbours" )
	{
		Matrix m( f.font, f.s );
		for ( int i = 0; i < 3; ++i )
		{
			m.append_row( make_cells( Fixture::circle( 0.05F ) ) );
		}
		const float y0 = ( *m.begin() )->position.y();
		const float y1 = ( *( m.begin() + 1 ) )->position.y();
		const float y2 = ( *( m.begin() + 2 ) )->position.y();
		CHECK( y0 - y1 == Approx( baseline_skip ) );
		CHECK( y1 - y2 == Approx( baseline_skip ) );
		// The box (with its 0.7 / 0.3 struts) is what sits on the axis, so the middle of the
		// baselines is 0.2 baseline-skips below it
		CHECK( ( y0 + y2 ) / 2.F == Approx( f.axis - 0.2F * baseline_skip ).margin( 1e-4 ) );
	}
}

TEST_CASE( "Matrix: box, scaling and delimiters", "[visuals]" )
{
	Fixture f;

	SECTION( "an empty matrix has no box" )
	{
		Matrix m( f.font, f.s );
		CHECK_FALSE( m.extent.has_value() );
	}

	SECTION( "it scales as a unit" )
	{
		Matrix m( f.font, f.s );
		m.append_row( make_cells( Fixture::circle( 0.1F ), Fixture::circle( 0.1F ) ) );
		const AABB before = *m.extent;
		m *= 2.F;
		CHECK( m.extent->max.x() == Approx( before.max.x() * 2.F ) );
		CHECK( m.extent->max.y() == Approx( before.max.y() * 2.F ) );
	}

	SECTION( "delimiters around it stretch to its height" )
	{
		auto grid = std::make_unique<Matrix>( f.font, f.s );
		for ( int i = 0; i < 3; ++i )
		{
			grid->append_row( make_cells( Fixture::circle( 0.2F ), Fixture::circle( 0.2F ) ) );
		}
		const float height = grid->extent->height();

		Delimited fenced( f.font, f.s, U'(', std::move( grid ), U')' );
		REQUIRE( fenced.size() == 3 );
		const AABB open = layout_extent( **fenced.begin() );
		CHECK( open.height() >= 0.85F * height );
	}
}
