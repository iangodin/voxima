
#pragma once

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <vector>

//====================================//

template<typename Key, typename Value>
struct Interval
{
	Key start;
	Key end;
	Value value;

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
};

//====================================//

template<typename Key, typename Value>
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
	// Adjacent intervals with the same value are merged.
	void insert( Key start, Key end, const Value &value )
	{
		if ( start >= end )
		{
			throw std::invalid_argument( "start must be less than end" );
		}

		// Coalesce with left neighbor
		auto it_low = std::lower_bound(
			_intervals.begin(),
			_intervals.end(),
			start,
			[]( const Interval &iv, Key k )
			{
				return iv.start < k;
			} );

		if ( it_low != _intervals.begin() )
		{
			auto it_prev = std::prev( it_low );
			if ( it_prev->end >= start && it_prev->value == value )
			{
				start = it_prev->start;
			}
		}

		// Coalesce with right neighbor
		auto it_high = std::lower_bound(
			_intervals.begin(),
			_intervals.end(),
			end,
			[]( const Interval &iv, Key k )
			{
				return iv.start < k;
			} );

		if ( it_high != _intervals.end() )
		{
			if ( it_high->start <= end && it_high->value == value )
			{
				end = it_high->end;
			}
		}

		erase( start, end );

		Interval new_interval( start, end, value );
		auto it = std::lower_bound(
			_intervals.begin(),
			_intervals.end(),
			start,
			[]( const Interval &iv, Key k )
			{
				return iv.start < k;
			} );
		_intervals.insert( it, new_interval );
	}

	// Return the value at key, or std::nullopt if unmapped.
	std::optional<Value> find( Key key ) const
	{
		auto it = std::lower_bound(
			_intervals.begin(),
			_intervals.end(),
			key,
			[]( const Interval &iv, Key k )
			{
				return iv.start < k;
			} );

		if ( it != _intervals.end() && it->contains( key ) )
		{
			return it->value;
		}

		if ( it != _intervals.begin() )
		{
			auto it_prev = std::prev( it );
			if ( it_prev->contains( key ) )
			{
				return it_prev->value;
			}
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
		{
			return;
		}

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
				{
					result.emplace_back( iv.start, start, iv.value );
				}
				// Right remnant
				if ( iv.end > end )
				{
					result.emplace_back( end, iv.end, iv.value );
				}
			}
		}

		_intervals = std::move( result );
	}

	// Apply a transformation function to all existing intervals within [start, end).
	// Supports two signatures:
	//   1. void(Value&)
	//   2. void(Key start, Key end, Value&)
	// Gaps (unmapped regions) are NOT transformed.
	// Adjacent intervals with the same value are merged after the transformation.
	template<typename F>
	void apply( Key start, Key end, F &&transform )
	{
		if ( start >= end )
		{
			return;
		}

		split( start );
		split( end );

		auto it = std::lower_bound(
			_intervals.begin(),
			_intervals.end(),
			start,
			[]( const Interval &iv, Key k )
			{
				return iv.start < k;
			} );

		for ( ; it != _intervals.end() && it->start < end; ++it )
		{
			if constexpr ( std::is_invocable_v<F, Value &> )
			{
				transform( it->value );
			}
			else if constexpr ( std::is_invocable_v<F, Key, Key, Value &> )
			{
				transform( it->start, it->end, it->value );
			}
			else
			{
				static_assert(
					std::is_invocable_v<F, Value &> || std::is_invocable_v<F, Key, Key, Value &>,
					"Transform function must accept either (Value&) or (Key, Key, Value&)" );
			}
		}

		coalesce();
	}

	// Remove intervals within [start, end) if they satisfy the predicate.
	// The predicate should have the signature bool(const Interval&).
	// Adjacent intervals with the same value are merged after the removal.
	template<typename P>
	void erase_if( Key start, Key end, P &&predicate )
	{
		if ( start >= end )
		{
			return;
		}

		split( start );
		split( end );

		auto it_start = std::lower_bound(
			_intervals.begin(),
			_intervals.end(),
			start,
			[]( const Interval &iv, Key k )
			{
				return iv.start < k;
			} );

		auto it_end = std::lower_bound(
			it_start,
			_intervals.end(),
			end,
			[]( const Interval &iv, Key k )
			{
				return iv.start < k;
			} );

		auto new_end = std::remove_if( it_start, it_end, predicate );
		_intervals.erase( new_end, it_end );

		coalesce();
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
		{
			return std::nullopt;
		}
		return std::make_pair( _intervals.front().start, _intervals.back().end );
	}

private:
	std::vector<Interval> _intervals;

	// Ensure an interval boundary exists at k.
	// If k falls inside an interval [s, e), it is split into [s, k) and [k, e).
	void split( Key k )
	{
		auto it = std::lower_bound(
			_intervals.begin(),
			_intervals.end(),
			k,
			[]( const Interval &iv, Key key )
			{
				return iv.start < key;
			} );

		if ( it != _intervals.begin() )
		{
			auto it_prev = std::prev( it );
			if ( it_prev->start < k && it_prev->end > k )
			{
				Interval right( k, it_prev->end, it_prev->value );
				it_prev->end = k;
				_intervals.insert( it, right );
			}
		}
	}

	// Merge adjacent intervals with the same value.
	void coalesce()
	{
		if ( _intervals.size() < 2 )
		{
			return;
		}

		auto write_it = _intervals.begin();
		for ( auto read_it = std::next( _intervals.begin() ); read_it != _intervals.end();
		      ++read_it )
		{
			if ( write_it->end == read_it->start && write_it->value == read_it->value )
			{
				write_it->end = read_it->end;
			}
			else
			{
				++write_it;
				if ( write_it != read_it )
				{
					*write_it = std::move( *read_it );
				}
			}
		}
		_intervals.erase( std::next( write_it ), _intervals.end() );
	}
};

//====================================//
