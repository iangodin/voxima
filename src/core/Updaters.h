#pragma once

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

// Things done every frame that keep one thing tied to another: a label that follows a moving point, a line between two
// boxes that are moving, a value shown that changes as something else does. Scenes run in steps; an updater is what
// makes "this follows that" true at every frame without the scene having to say it every time.
//
//     auto follow = stage.updaters.add( [&] { label.position = dot.position + Point{ 0.F, 0.4F }; } );
//     co_await move_along( stage, dot, path, 2s );      // the label goes with the dot
//
// They run each frame, after the scene has done what it does in that frame (so they see where everything is now), in the
// order they were added, and before the picture is made. The handle stops the updater when it is destroyed, so keep it for
// as long as the tie should hold; `release()` keeps it going for the rest of the scene instead. All are cleared when a scene
// starts (with the stage's visuals: an updater must not outlive what it refers to, so take one off before removing a visual
// it uses if the scene goes on).
class Updaters
{
public:
	// Ends its updater when destroyed
	class Handle
	{
	public:
		Handle() = default;

		Handle( Updaters *owner, size_t id ) :
			_owner( owner ),
			_id( id )
		{
		}

		Handle( Handle &&other ) noexcept :
			_owner( std::exchange( other._owner, nullptr ) ),
			_id( other._id )
		{
		}

		Handle &operator=( Handle &&other ) noexcept
		{
			if ( this != &other )
			{
				stop();
				_owner = std::exchange( other._owner, nullptr );
				_id = other._id;
			}
			return *this;
		}

		Handle( const Handle & ) = delete;
		Handle &operator=( const Handle & ) = delete;

		~Handle()
		{
			stop();
		}

		// End it now
		void stop()
		{
			if ( _owner )
			{
				_owner->remove( _id );
				_owner = nullptr;
			}
		}

		// Let it go on until the scene ends, whatever happens to this handle
		void release()
		{
			_owner = nullptr;
		}

		bool active() const
		{
			return _owner != nullptr;
		}

	private:
		Updaters *_owner = nullptr;
		size_t _id = 0;
	};

	Handle add( std::function<void()> update )
	{
		const size_t id = _next++;
		_entries.push_back( { id, std::move( update ) } );
		return Handle( this, id );
	}

	// Run them all once, in the order they were added. One may add or end others (or itself) while it runs: the ones added
	// run from the next frame, the ones ended do not run again.
	void run()
	{
		std::vector<size_t> ids;
		ids.reserve( _entries.size() );
		for ( const Entry &entry: _entries )
		{
			ids.push_back( entry.id );
		}
		for ( size_t id: ids )
		{
			for ( Entry &entry: _entries )
			{
				if ( entry.id == id )
				{
					const std::function<void()> update = entry.update; // (it may end itself)
					update();
					break;
				}
			}
		}
	}

	void remove( size_t id )
	{
		std::erase_if( _entries, [id]( const Entry &entry ) { return entry.id == id; } );
	}

	void clear()
	{
		_entries.clear();
	}

	size_t size() const
	{
		return _entries.size();
	}

private:
	struct Entry
	{
		size_t id;
		std::function<void()> update;
	};

	std::vector<Entry> _entries;
	size_t _next = 1;
};
