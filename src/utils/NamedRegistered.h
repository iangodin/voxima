
#pragma once

#include <string>
#include <unordered_map>

template <typename T>
class NamedRegistered
{
public:
	explicit NamedRegistered( std::string name ) :
		m_name( std::move( name ) )
	{
		register_instance();
	}

	NamedRegistered( NamedRegistered &&other ) noexcept :
		m_name( std::move( other.m_name ) )
	{
		register_instance();
	}

	virtual ~NamedRegistered()
	{
		// Only if it is still this one: another (from a plugin loaded since, say) may have taken the name
		auto &registry = get_registry();
		if ( auto it = registry.find( m_name ); it != registry.end() && it->second == this )
		{
			registry.erase( it );
		}
	}

	NamedRegistered( const NamedRegistered & ) = delete;
	NamedRegistered &operator=( const NamedRegistered & ) = delete;

	static T *get_by_name( const std::string &name )
	{
		auto &registry = get_registry();
		auto it = registry.find( name );
		if ( it == registry.end() )
			return nullptr;
		return it->second;
	}

	const std::string &get_name() const
	{
		return m_name;
	}

	// Defined in one place in the core library for each T (with an explicit instantiation there), not here:
	// a header's static would be a separate one in each plugin, and their entries would not be seen.
	static std::unordered_map<std::string, T *> &get_registry();
private:
	std::string m_name;

	void register_instance()
	{
		if ( m_name.empty() )
			return;
		get_registry()[m_name] = static_cast<T *>( this );
	}

};
