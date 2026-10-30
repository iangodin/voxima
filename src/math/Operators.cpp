#include "math/Operators.h"

namespace math
{

Expr operator+( Expr a, Expr b )
{
	return row( { ord( std::move( a ) ), bin( "+" ), ord( std::move( b ) ) } );
}

Expr operator-( Expr a, Expr b )
{
	return row( { ord( std::move( a ) ), bin( "−" ), ord( std::move( b ) ) } );
}

Expr operator-( Expr a )
{
	return row( { ord( "−" ), ord( std::move( a ) ) } );
}

Expr operator*( Expr a, Expr b )
{
	return row( { ord( std::move( a ) ), ord( std::move( b ) ) } );
}

Expr times( Expr a, Expr b )
{
	return row( { ord( std::move( a ) ), bin( sym::times ), ord( std::move( b ) ) } );
}

Expr operator/( Expr a, Expr b )
{
	return fraction( std::move( a ), std::move( b ) );
}

Expr relation( Expr a, const char *symbol, Expr b )
{
	return row( { ord( std::move( a ) ), rel( std::string( symbol ) ), ord( std::move( b ) ) } );
}

Expr eq( Expr a, Expr b )
{
	return relation( std::move( a ), "=", std::move( b ) );
}

Expr lt( Expr a, Expr b )
{
	return relation( std::move( a ), "<", std::move( b ) );
}

Expr gt( Expr a, Expr b )
{
	return relation( std::move( a ), ">", std::move( b ) );
}

Expr leq( Expr a, Expr b )
{
	return relation( std::move( a ), sym::leq, std::move( b ) );
}

Expr geq( Expr a, Expr b )
{
	return relation( std::move( a ), sym::geq, std::move( b ) );
}

Expr neq( Expr a, Expr b )
{
	return relation( std::move( a ), sym::neq, std::move( b ) );
}

namespace literals
{

Expr operator""_m( const char *characters, size_t size )
{
	return text( std::string( characters, size ) );
}

Expr operator""_u( const char *characters, size_t size )
{
	return upright( std::string( characters, size ) );
}

}

}
