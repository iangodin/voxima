#pragma once

#include "Visual.h"
#include "VisualVisitor.h"

#include <algorithm>
#include <cstddef>
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

	/// Add a child at a place in the drawing order: 0 is the back, and an index past the end is the front
	template<typename Shape>
	Shape &insert( size_t index, std::unique_ptr<Shape> &&visual )
	{
		auto *shape = visual.get();
		_visuals.insert( _visuals.begin() + static_cast<std::ptrdiff_t>( std::min( index, _visuals.size() ) ), std::move( visual ) );
		return *shape;
	}

	/// Move a direct child to the front or the back of the drawing order (in front of or behind all the others),
	/// or one place forward or back. False, with nothing changed, if it is not a direct child of this group.
	bool to_front( const Visual *child )
	{
		return move_child( child, _visuals.size() );
	}

	bool to_back( const Visual *child )
	{
		return move_child( child, 0 );
	}

	bool raise( const Visual *child )
	{
		const size_t at = index_of( child );
		return move_child( child, at == _visuals.size() ? at : at + 1 );
	}

	bool lower( const Visual *child )
	{
		const size_t at = index_of( child );
		return move_child( child, at == 0 ? 0 : at - 1 );
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

private:
	// Where a direct child is, or size() if it is not one
	size_t index_of( const Visual *child ) const
	{
		size_t at = 0;
		while ( at < _visuals.size() && _visuals[at].get() != child )
		{
			++at;
		}
		return at;
	}

	bool move_child( const Visual *child, size_t to )
	{
		const size_t at = index_of( child );
		if ( at == _visuals.size() )
		{
			return false;
		}
		to = std::min( to, _visuals.size() - 1 );
		const auto begin = _visuals.begin();
		if ( to < at )
		{
			std::rotate( begin + static_cast<std::ptrdiff_t>( to ), begin + static_cast<std::ptrdiff_t>( at ), begin + static_cast<std::ptrdiff_t>( at + 1 ) );
		}
		else
		{
			std::rotate( begin + static_cast<std::ptrdiff_t>( at ), begin + static_cast<std::ptrdiff_t>( at + 1 ), begin + static_cast<std::ptrdiff_t>( to + 1 ) );
		}
		return true;
	}
};
