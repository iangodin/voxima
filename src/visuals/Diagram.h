#pragma once

#include "core/Theme.h"
#include "visuals/Connector.h"
#include "visuals/CreateText.h"
#include "visuals/Group.h"
#include "visuals/Panel.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

// Which way a layered layout runs
enum class Direction
{
	DOWN,
	RIGHT
};

struct DiagramLayout
{
	Direction direction = Direction::DOWN;

	// Between one layer and the next, and between neighbours in a layer (stage units, between the boxes)
	float layer_gap = 1.2F;
	float node_gap = 0.6F;
};

// Boxes joined by arrows: a flowchart, a tree, a graph, the parts of a system. The nodes are Panels (so they are boxes of
// what is in them, in the palette's tint) in one group, the edges Connectors in another behind it; both are in this group,
// so the whole diagram moves, fades and draws as one, or a node or an edge alone (node( id ), edge( i )).
//
//     Diagram flow;
//     flow.add_text_node( theme, "in", "Input", style );
//     flow.add_text_node( theme, "run", "Process", style );
//     flow.add_edge( "in", "run" );
//     flow.layout_layers();                 // the nodes in rows, the arrows following
//
// Nodes can also be put anywhere by hand (node( id ).position, then update()).
class Diagram : public Group
{
public:
	Diagram();

	// A node with anything in it (a Panel is made round it), at `at`; the id is how it is found again. Adding one with an id
	// that is there already replaces nothing: it throws.
	Panel &add_node( const std::string &id, std::unique_ptr<Visual> content, Point at = { 0.F, 0.F }, const PanelOptions &options = {} );

	// A node holding text
	Panel &add_text_node( Theme &theme, const std::string &id, const std::string &text, const FontStyle &style, Point at = { 0.F, 0.F }, const PanelOptions &options = {} );

	// An arrow from one node to another (by id). Returns the Connector, which is also edge( index ).
	Connector &add_edge( const std::string &from, const std::string &to, const ConnectorOptions &options = {} );

	Panel &node( const std::string &id ) const;
	bool has_node( const std::string &id ) const;
	size_t node_count() const;
	Connector &edge( size_t index ) const;
	size_t edge_count() const;

	// The nodes in layers: a node is one layer further along than every node with an arrow into it (following the arrows, so
	// a node with none in is in the first), the layers one after another in the direction, the nodes of a layer side by side,
	// each layer centered on the origin. Nodes in a layer keep the order they were added in. (Arrows that loop back are not
	// followed.) Then the arrows are laid out again.
	void layout_layers( const DiagramLayout &layout = {} );

	// Lay the arrows out again, from where the nodes are now
	void update();

private:
	struct Edge
	{
		std::string from;
		std::string to;
		Connector *connector;
	};

	Group *_edges;
	Group *_nodes;
	std::vector<std::string> _order; // the ids, as added
	std::map<std::string, Panel *> _by_id;
	std::vector<Edge> _links;
};

// Keep a diagram's arrows joining its nodes at every frame, until the handle goes
Updaters::Handle follow( Stage &stage, Diagram &diagram );

std::unique_ptr<Diagram> make_diagram();
