#pragma once

#include "Visual.h"
#include "VisualVisitor.h"

#include <memory>
#include <vector>

class Group : public Visual
{
public:
	Group() = default;
	Group( const Group & ) = delete;

	virutal ~Group() = default;

	Group &operator=( const Group & ) = delete;

	template<typename Shape>
	Shape &add( std::unique_ptr<Shape> &&visual )
	{
		auto *shape = visual.get();
		_visuals.push_back( std::move( visual ) );
		return *shape;
	}

	template<typename Shape>
	Shape &set( size_t i, std::unique_ptr<Shape> &&visual )
	{
		auto *shape = visual.get();
		_visuals.at( i ) = std::move( visual );
		return *shape;
	}

	void clear( void )
	{
		_visuals.clear();
	}

	size_t size( void ) const
	{
		return _visuals.size();
	}

	auto begin( void ) const
	{
		return _visuals.begin();
	}

	auto end( void ) const
	{
		return _visuals.end();
	}

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}

	Group &operator*=( float scale ) override
	{
		for ( auto &visual: _visuals )
		{
			*visual *= scale;
		}
		position *= scale;
		return *this;
	}

protected:
	std::vector<std::unique_ptr<Visual>> _visuals;
};
