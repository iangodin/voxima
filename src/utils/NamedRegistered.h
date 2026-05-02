
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
		if ( !m_name.empty() )
		{
			get_registry().erase( m_name );
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

private:
	std::string m_name;

	void register_instance()
	{
		if ( m_name.empty() )
			return;
		get_registry()[m_name] = static_cast<T *>( this );
	}

	static std::unordered_map<std::string, T *> &get_registry()
	{
		static std::unordered_map<std::string, T *> registry;
		return registry;
	}
};
