
#pragma once

#include "Visual.h"
#include "VisualVisitor.h"

#include <vector>
#include <memory>

class Visuals : public Visual
{
public:
	template<typename Shape>
	Shape &add( std::unique_ptr<Shape> &&visual )
	{
		auto *shape = visual.get();
		_visuals.push_back( std::move( visual ) );
		return *shape;
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

private:
	std::vector<std::unique_ptr<Visual>> _visuals;
};

