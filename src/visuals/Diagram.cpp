#include "visuals/Diagram.h"

#include "core/Stage.h"
#include "visuals/Placement.h"

#include <algorithm>
#include <stdexcept>

Diagram::Diagram()
{
	_edges = &add( std::make_unique<Group>() );
	_nodes = &add( std::make_unique<Group>() );
}

Panel &Diagram::add_node( const std::string &id, std::unique_ptr<Visual> content, Point at, const PanelOptions &options )
{
	if ( _by_id.contains( id ) )
	{
		throw std::invalid_argument( "Diagram: there is a node called '" + id + "' already" );
	}
	auto &panel = _nodes->add( make_panel( std::move( content ), options ) );
	// Its center at `at`
	move_to( panel, at, Anchor::CENTER );
	_by_id[id] = &panel;
	_order.push_back( id );
	return panel;
}

Panel &Diagram::add_text_node( Theme &theme, const std::string &id, const std::string &text, const FontStyle &style, Point at, const PanelOptions &options )
{
	return add_node( id, make_text( theme, text, style ), at, options );
}

Connector &Diagram::add_edge( const std::string &from, const std::string &to, const ConnectorOptions &options )
{
	const auto a = _by_id.find( from );
	const auto b = _by_id.find( to );
	if ( a == _by_id.end() || b == _by_id.end() )
	{
		throw std::invalid_argument( "Diagram: an edge from '" + from + "' to '" + to + "', and one of them is not a node" );
	}
	auto &connector = _edges->add( make_connector( *a->second, *b->second, options ) );
	_links.push_back( { from, to, &connector } );
	return connector;
}

Panel &Diagram::node( const std::string &id ) const
{
	const auto it = _by_id.find( id );
	if ( it == _by_id.end() )
	{
		throw std::invalid_argument( "Diagram: there is no node called '" + id + "'" );
	}
	return *it->second;
}

bool Diagram::has_node( const std::string &id ) const
{
	return _by_id.contains( id );
}

size_t Diagram::node_count() const
{
	return _order.size();
}

Connector &Diagram::edge( size_t index ) const
{
	return *_links.at( index ).connector;
}

size_t Diagram::edge_count() const
{
	return _links.size();
}

void Diagram::update()
{
	for ( const Edge &link: _links )
	{
		link.connector->update();
	}
}

void Diagram::layout_layers( const DiagramLayout &layout )
{
	const size_t n = _order.size();
	if ( n == 0 )
	{
		return;
	}
	std::map<std::string, size_t> index;
	for ( size_t i = 0; i < n; ++i )
	{
		index[_order[i]] = i;
	}

	// Longest path from the start: relax along the arrows, at most n rounds (an arrow that loops back would never settle)
	std::vector<size_t> depth( n, 0 );
	for ( size_t round = 0; round < n; ++round )
	{
		bool changed = false;
		for ( const Edge &link: _links )
		{
			const size_t from = index[link.from];
			const size_t to = index[link.to];
			if ( depth[from] + 1 > depth[to] && depth[from] + 1 < n )
			{
				depth[to] = depth[from] + 1;
				changed = true;
			}
		}
		if ( !changed )
		{
			break;
		}
	}

	const size_t layers = *std::max_element( depth.begin(), depth.end() ) + 1;
	std::vector<std::vector<Panel *>> in_layer( layers );
	for ( size_t i = 0; i < n; ++i )
	{
		in_layer[depth[i]].push_back( _by_id[_order[i]] );
	}

	const bool down = layout.direction == Direction::DOWN;
	// Along a layer: the size across (width for DOWN); through the layers: the size along (height for DOWN)
	auto across = [&]( const Panel &p ) { const AABB b = box_of( p ); return down ? b.max.x() - b.min.x() : b.max.y() - b.min.y(); };
	auto along = [&]( const Panel &p ) { const AABB b = box_of( p ); return down ? b.max.y() - b.min.y() : b.max.x() - b.min.x(); };

	float position = 0.F; // how far through the layers the next one starts
	std::vector<float> starts;
	std::vector<float> thickness;
	for ( const auto &layer: in_layer )
	{
		float thick = 0.F;
		for ( const Panel *p: layer )
		{
			thick = std::max( thick, along( *p ) );
		}
		starts.push_back( position );
		thickness.push_back( thick );
		position += thick + layout.layer_gap;
	}
	const float total = position - layout.layer_gap;

	for ( size_t l = 0; l < layers; ++l )
	{
		float width = -layout.node_gap;
		for ( const Panel *p: in_layer[l] )
		{
			width += across( *p ) + layout.node_gap;
		}
		float at = -width / 2.F;
		// The middle of the layer's band, centered on the origin overall
		const float middle = starts[l] + thickness[l] / 2.F - total / 2.F;
		for ( Panel *p: in_layer[l] )
		{
			const float size = across( *p );
			const float x = at + size / 2.F;
			move_to( *p, down ? Point{ x, -middle } : Point{ middle, -x }, Anchor::CENTER );
			at += size + layout.node_gap;
		}
	}
	update();
}

Updaters::Handle follow( Stage &stage, Diagram &diagram )
{
	return stage.updaters.add( [&diagram] { diagram.update(); } );
}

std::unique_ptr<Diagram> make_diagram()
{
	return std::make_unique<Diagram>();
}
