
#pragma once

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <optional>

//====================================//

template <typename Key, typename Value>
struct Interval
{
	Key start;
	Key end;
	Value value;

	Interval( Key s, Key e, const Value &v ) :
		start( s ), end( e ), value( v )
	{
	}

	bool operator<( const Interval &other ) const
	{
		return start < other.start;
	}

	bool contains( Key key ) const
	{
		return key >= start && key < end;
	}

	bool overlaps( const Interval &other ) const
	{
		return start < other.end && other.start < end;
	}

	bool is_adjacent( const Interval &other ) const
	{
		return end == other.start || other.end == start;
	}

	// Required by structured bindings alongside std::tuple_size/element below.
	template <std::size_t N>
	auto get() const
	{
		if constexpr ( N == 0 )
			return start;
		else if constexpr ( N == 1 )
			return end;
		else if constexpr ( N == 2 )
			return value;
	}
};

//====================================//

namespace std
{
template <typename Key, typename Value>
struct tuple_size<Interval<Key, Value>> : integral_constant<size_t, 3>
{
};

template <typename Key, typename Value>
struct tuple_element<0, Interval<Key, Value>>
{
	using type = Key;
};

template <typename Key, typename Value>
struct tuple_element<1, Interval<Key, Value>>
{
	using type = Key;
};

template <typename Key, typename Value>
struct tuple_element<2, Interval<Key, Value>>
{
	using type = Value;
};
}

//====================================//

template <typename Key, typename Value>
class IntervalMap
{
public:
	using Interval = ::Interval<Key, Value>;

	// Iterator types
	using iterator = typename std::vector<Interval>::iterator;
	using const_iterator = typename std::vector<Interval>::const_iterator;

	IntervalMap() = default;

	// Insert [start, end) -> value.
	// Overlapping existing intervals are replaced by the new value over the
	// overlapping region; their non-overlapping tails are preserved.
	void insert( Key start, Key end, const Value &value )
	{
		if ( start >= end )
			throw std::invalid_argument( "start must be less than end" );

		Interval new_interval( start, end, value );

		auto it = std::lower_bound( _intervals.begin(), _intervals.end(), new_interval );

		// Step back if the previous interval overlaps
		if ( it != _intervals.begin() )
		{
			auto prev = std::prev( it );
			if ( prev->end > start )
				--it;
		}

		Key actual_start = start;
		Key actual_end = end;

		// Consume all intervals that overlap [start, end)
		while ( it != _intervals.end() && it->start < end )
		{
			actual_start = std::min( actual_start, it->start );
			actual_end = std::max( actual_end, it->end );
			it = _intervals.erase( it );
		}

		_intervals.insert(
			std::lower_bound(
				_intervals.begin(),
				_intervals.end(),
				Interval( actual_start, actual_end, value ) ),
			Interval( actual_start, actual_end, value ) );
	}

	// Return the value at key, or std::nullopt if unmapped.
	std::optional<Value> find( Key key ) const
	{
		auto it = std::lower_bound(
			_intervals.begin(), _intervals.end(), Interval( key, key + 1, Value{} ) );
		if ( it != _intervals.begin() )
		{
			--it;
			if ( it->contains( key ) )
				return it->value;
		}
		return std::nullopt;
	}

	bool contains( Key key ) const
	{
		return find( key ).has_value();
	}

	// Remove the range [start, end) from the map.
	// Intervals that partially overlap are trimmed; their surviving portions remain.
	void erase( Key start, Key end )
	{
		if ( start >= end )
			return;

		std::vector<Interval> result;
		result.reserve( _intervals.size() );

		for ( const auto &iv: _intervals )
		{
			if ( iv.end <= start || iv.start >= end )
			{
				// No overlap — keep as-is
				result.push_back( iv );
			}
			else
			{
				// Left remnant
				if ( iv.start < start )
					result.emplace_back( iv.start, start, iv.value );
				// Right remnant
				if ( iv.end > end )
					result.emplace_back( end, iv.end, iv.value );
			}
		}

		_intervals = std::move( result );
	}

	void clear()
	{
		_intervals.clear();
	}

	bool empty() const
	{
		return _intervals.empty();
	}

	size_t size() const
	{
		return _intervals.size();
	}

	// Iterators
	iterator begin()
	{
		return _intervals.begin();
	}

	const_iterator begin() const
	{
		return _intervals.begin();
	}

	const_iterator cbegin() const
	{
		return _intervals.cbegin();
	}

	iterator end()
	{
		return _intervals.end();
	}

	const_iterator end() const
	{
		return _intervals.end();
	}

	const_iterator cend() const
	{
		return _intervals.cend();
	}

	// Direct access to the underlying sorted vector
	const std::vector<Interval> &intervals() const
	{
		return _intervals;
	}

	// Overall [min, max) span, or nullopt if empty
	std::optional<std::pair<Key, Key>> span() const
	{
		if ( _intervals.empty() )
			return std::nullopt;
		return std::make_pair( _intervals.front().start, _intervals.back().end );
	}

private:
	std::vector<Interval> _intervals;
};

//====================================//

