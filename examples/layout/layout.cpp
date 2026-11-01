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
#include <visuals/Tags.h>

#include <chrono>
#include <cmath>

// Times in the scenes are written in seconds (0.5s, 1.7s), so they last as long at any --fps
using namespace std::chrono_literals;

VOXIMA_PLUGIN();

// The basics of the guide: shapes, groups, placement against each other, rows and grids, panels, lists, tables and tags
Scene show_basics( Stage &stage, Script & )
{
	Theme &theme = *stage.theme;
	const FontStyle style{ Intent::LABEL };

	// --8<-- [start:shapes]
	auto circle = std::make_unique<Circle>( 0.7F, theme.palette().blue );   // radius, color
	auto &dot = stage.visuals.add( std::move( circle ) );
	dot.position = { -3.F, 0.F };

	auto &box = stage.visuals.add( std::make_unique<Rectangle>( 1.F, 0.5F, theme.palette().green ) );  // half width, half height
	// --8<-- [end:shapes]
	box.position = { -5.F, 0.F };

	// --8<-- [start:groups]
	auto &row = stage.visuals.add( std::make_unique<Group>() );
	row.add( std::make_unique<Circle>( 0.4F ) );
	row.add( std::make_unique<Circle>( 0.4F ) );
	// --8<-- [end:groups]
	row.position = { 0.F, -3.F };

	auto &title = stage.visuals.add( make_text( theme, "Placing things", FontStyle{ Intent::TITLE } ) );
	auto &label = stage.visuals.add( make_text( theme, "label", style ) );
	auto &other = stage.visuals.add( std::make_unique<Circle>( 0.5F, theme.palette().red ) );
	other.position = { 1.F, 1.F };
	// --8<-- [start:panel]
	auto &panel = stage.visuals.add( make_panel( make_text( theme, "Theorem", style ), { .padding = 0.5F } ) );
	// --8<-- [end:panel]
	// --8<-- [start:placement]
	move_to( title, { 0.F, 3.5F }, Anchor::TOP );          // its top edge at y = 3.5
	next_to( label, dot, Anchor::BOTTOM );                 // under it, a gap between
	next_to( panel, other, Anchor::RIGHT, 1.0F );          // to its right, 1 unit apart
	// --8<-- [end:placement]

	auto &group = stage.visuals.add( std::make_unique<Group>() );
	for ( int i = 0; i < 7; ++i )
	{
		group.add( std::make_unique<Rectangle>( 0.2F + 0.05F * static_cast<float>( i ), 0.2F, theme.palette().orange ) );
	}
	// --8<-- [start:arrange]
	arrange_row( group, { .gap = 0.5F } );                     // left to right, centered on the origin
	arrange_column( group, { .gap = 0.3F, .align = Align::START } );
	arrange_grid( group, 3, { .column_gap = 0.4F, .row_gap = 0.2F } );   // rows of 3
	distribute_row( group );                                   // equal spaces between, outer two stay put
	align_edges( group, Anchor::LEFT );                        // line up on an edge
	// --8<-- [end:arrange]
	group.position = { 4.F, -2.F };

	// --8<-- [start:keep-inside]
	keep_inside( group, safe_area( stage.camera ) );   // moved the least it takes
	fit_inside( group, safe_area( stage.camera ) );    // and scaled down if it is too big
	// --8<-- [end:keep-inside]

	// --8<-- [start:list]
	auto &list = stage.visuals.add( make_list( theme, { "first", "second", "third" }, { .numbered = true } ) );
	list.row( 1 );    // each row is a visual of its own, to show or light on its own
	// --8<-- [end:list]
	move_to( list, { -5.F, 2.F }, Anchor::LEFT );

	// --8<-- [start:table]
	const std::vector<std::vector<std::string>> rows = {
		{ "Name", "Scale", "Use" },
		{ "tiny", "0.5", "footnotes" },
		{ "huge", "2.0", "titles" } };
	auto &table = stage.visuals.add( make_table( theme, rows, { .striped = true, .max_width = 10.F } ) );
	table.cell( 1, 2 );   // a cell is a visual
	// --8<-- [end:table]
	move_to( table, { 3.F, 1.5F }, Anchor::LEFT );

	// --8<-- [start:tags]
	tag_visual( dot, "hero" );
	Visual *found = find_tag( stage.visuals, "hero" );
	// --8<-- [end:tags]
	found->color = theme.palette().yellow;

	co_await wait_for( stage, 3s );
}

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

// What is drawn in front of what: a group draws its children in order
Scene show_order( Stage &stage, Script & )
{
	const Palette &palette = stage.theme->palette();
	auto &back = stage.visuals.add( std::make_unique<Rectangle>( 2.F, 1.2F, palette.blue ) );
	back.position = { -1.F, 0.F };
	auto &front = stage.visuals.add( std::make_unique<Circle>( 1.4F, palette.orange ) );
	front.position = { 1.F, 0.F };
	co_await wait_for( stage, 1s );

	// --8<-- [start:order]
	stage.visuals.to_front( &back );                                            // now the rectangle hides the circle
	auto &behind = stage.visuals.insert( 0, std::make_unique<Circle>( 2.F, palette.green ) ); // behind them both
	// --8<-- [end:order]
	behind.position = { -3.F, 0.F };
	co_await wait_for( stage, 1s );

	stage.visuals.to_back( &back );
	co_await wait_for( stage, 1s );
}

REGISTER_SCENE( "layout-basics", "Shapes, groups, placement, rows and grids, panels, lists, tables and tags", show_basics );
REGISTER_SCENE( "layout", "Panels, a list and a table, and a row of shapes, placed against each other", show_layout );
REGISTER_SCENE( "tables", "Tables that wrap, hold formulas, and tint their rows", show_tables );
REGISTER_SCENE( "drawing-order", "Putting a shape in front of or behind the others", show_order );
