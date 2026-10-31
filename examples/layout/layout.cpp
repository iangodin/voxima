// Things placed against each other, not on coordinates: panels with a list and a table, a row of shapes, and tables with wrapped columns, formulas and tinted rows.
//
// Build it (see CMakeLists.txt), then
//
//     voxima --plugin build/layout --script-dir scripts --lang en --list-scenes
//     voxima --plugin build/layout --script-dir scripts --lang en --scene layout --preview
//
// With --preview, rebuilding it while the window is open plays the scene again with the change.

#include <core/Animate.h>
#include <core/Plugin.h>
#include <core/SceneRegistry.h>
#include <core/Script.h>
#include <core/Stage.h>
#include <core/Tween.h>
#include <core/Wait.h>
#include <math/Operators.h>
#include <visuals/CreateList.h>
#include <visuals/CreateTable.h>
#include <visuals/CreateText.h>
#include <visuals/Panel.h>

#include <chrono>
#include <cmath>

// Times in the scenes are written in seconds (0.5s, 1.7s), so they last as long at any --fps
using namespace std::chrono_literals;

VOXIMA_PLUGIN();

namespace
{
// Things placed against each other, not on coordinates: a list and a table in panels, and a row of labeled shapes
Scene show_layout( Stage &stage, Script & )
{
	Theme &theme = *stage.theme;
	const Palette &palette = theme.palette();

	// A title, at the top
	auto &title = stage.visuals.add( make_text( theme, "Layout", FontStyle{ Intent::TITLE }.large() ) );
	move_to( title, { 0.F, 4.1F }, Anchor::TOP );

	// A list in a panel, and a table in another, side by side
	// (a panel with a shadow has an opaque fill: the background lifted a little toward the text color)
	const Color lifted{ palette.background.r * 0.88F + palette.text.r * 0.12F, palette.background.g * 0.88F + palette.text.g * 0.12F, palette.background.b * 0.88F + palette.text.b * 0.12F, 1.F };
	auto list_panel = make_panel(
		make_list( theme, { "Rows and columns", "Grids of any size", "Panels that fit", "Numbered lists too" } ),
		{ .fill = lifted, .shadow = ShadowOptions{} } );
	auto table_panel = make_panel(
		make_table( theme,
		{ { "Palette", "Type", "Background" },
		  { "paper", "light", "warm white" },
		  { "one-dark", "dark", "slate" },
		  { "blueprint", "dark", "navy" } },
		{ .striped = true, .alignment = { Align::START, Align::CENTER, Align::END } } ) );
	auto &left = stage.visuals.add( std::move( list_panel ) );
	auto &right = stage.visuals.add( std::move( table_panel ) );
	move_to( left, { -3.6F, 1.2F }, Anchor::CENTER );
	next_to( right, left, Anchor::RIGHT, 0.7F );

	// A row of shapes, each with its label under it, the row centered under the panels
	Group shapes;
	auto shape_with = [&]( std::unique_ptr<Visual> shape, const std::string &name )
	{
		auto column = std::make_unique<Group>();
		column->add( std::move( shape ) );
		// The label's baseline at the same depth in every column (a text's box depends on the letters in it, so lining
		// up the boxes would not line up the words), centered under the shape
		auto &label = column->add( make_text( theme, name, FontStyle{ Intent::LABEL } ) );
		const AABB box = box_of( label );
		label.position = { -( box.min.x() + box.max.x() ) / 2.F, -1.05F };
		return column;
	};
	auto circle = std::make_unique<Circle>( 0.5F, palette.red );
	auto square = std::make_unique<Rectangle>( 0.5F, 0.5F, palette.blue );
	auto bar = std::make_unique<Rectangle>( 1.1F, 0.4F, palette.green );
	auto &row = stage.visuals.add( std::make_unique<Group>() );
	row.add( shape_with( std::move( circle ), "circle" ) );
	row.add( shape_with( std::move( square ), "square" ) );
	row.add( shape_with( std::move( bar ), "a longer bar" ) );
	arrange_row( row, { .gap = 1.2F, .align = Align::BASELINE } ); // the columns' origins level: shapes centered, labels on one line
	next_to( row, left, Anchor::BOTTOM, 0.9F );
	move_to( row, { 0.F, point_at( row, Anchor::CENTER ).y() }, Anchor::CENTER );

	co_await wait_for( stage, 3s );
}

// Tables: columns that wrap to a width, formulas as cells, tinted rows, and a grid
Scene show_tables( Stage &stage, Script & )
{
	using namespace math::literals;
	Theme &theme = *stage.theme;
	const Palette &palette = theme.palette();
	const FontStyle style{ Intent::PROSE };

	auto &title = stage.visuals.add( make_text( theme, "Tables", FontStyle{ Intent::TITLE } ) );
	move_to( title, { 0.F, 4.1F }, Anchor::TOP );

	// A table too wide for its place: the widest column is wrapped, and the rows tinted
	const std::vector<std::vector<std::string>> steps = {
		{ "Step", "What happens" },
		{ "read", "The input is read a line at a time and split into words, keeping the position of each" },
		{ "check", "Every word is looked up, and the ones that are not known are collected for the report" },
		{ "run", "Done" } };
	auto &notes = stage.visuals.add( make_panel( make_table(
		theme,
		steps,
		{ .striped = true, .header_fill = Color{ palette.blue.r, palette.blue.g, palette.blue.b, 0.25F }, .max_width = 6.6F } ) ) );
	move_to( notes, { -3.6F, 0.9F }, Anchor::CENTER );

	// Formulas in the cells, the columns lined up on their edges
	math::Context context( theme, FontStyle{ Intent::MATH } );
	auto cell = [&]( const math::Expr &formula )
	{
		auto visual = formula.build( context );
		return std::unique_ptr<Visual>( std::move( visual ) );
	};
	std::vector<std::vector<std::unique_ptr<Visual>>> rows( 4 );
	rows[0].push_back( make_text( theme, "function", FontStyle{ Intent::PROSE }.bold() ) );
	rows[0].push_back( make_text( theme, "derivative", FontStyle{ Intent::PROSE }.bold() ) );
	using namespace math;
	rows[1].push_back( cell( superscript( "x"_m, "2"_m ) ) );
	rows[1].push_back( cell( "2"_m * "x"_m ) );
	rows[2].push_back( cell( row( { op( sin() ), ord( "x"_m ) } ) ) );
	rows[2].push_back( cell( row( { op( cos() ), ord( "x"_m ) } ) ) );
	rows[3].push_back( cell( radical( "x"_m ) ) );
	rows[3].push_back( cell( "1"_m / ( "2"_m * radical( "x"_m ) ) ) );
	auto &formulas = stage.visuals.add( make_panel( make_table( theme, std::move( rows ), { .grid = true, .alignment = { Align::CENTER } } ) ) );
	move_to( formulas, { 4.2F, 0.6F }, Anchor::CENTER );

	co_await wait_for( stage, 3s );
}
}

REGISTER_SCENE( "layout", "Panels, a list and a table, and a row of shapes, placed against each other", show_layout );
REGISTER_SCENE( "tables", "Tables that wrap, hold formulas, and tint their rows", show_tables );
