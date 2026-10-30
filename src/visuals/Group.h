#pragma once

#include "Visual.h"
#include "VisualVisitor.h"

#include <algorithm>
#include <memory>
#include <vector>

class Group : public Visual
{
public:
	Group() = default;
	Group( const Group & ) = delete;

	virtual ~Group() = default;

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

	/// Swap the direct child `old` for `with`, keeping its place in the drawing order. Returns
	/// the old child (so the caller decides when it dies), or null - leaving everything as it
	/// was - if `old` is not a direct child of this group.
	std::unique_ptr<Visual> replace( const Visual *old, std::unique_ptr<Visual> with )
	{
		for ( auto &visual: _visuals )
		{
			if ( visual.get() == old )
			{
				std::swap( visual, with );
				return with;
			}
		}
		return nullptr;
	}

	/// Take the direct child `old` out of this group and hand it back, or null if it is not one.
	std::unique_ptr<Visual> remove( const Visual *old )
	{
		for ( auto it = _visuals.begin(); it != _visuals.end(); ++it )
		{
			if ( it->get() == old )
			{
				auto taken = std::move( *it );
				_visuals.erase( it );
				return taken;
			}
		}
		return nullptr;
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
		scale_common( scale );
		return *this;
	}

protected:
	std::vector<std::unique_ptr<Visual>> _visuals;
};
