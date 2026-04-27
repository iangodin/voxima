#pragma once

#include "VisualVisitor.h"
#include "Circle.h"
#include "Rectangle.h"
#include "RoundRectangle.h"
#include "Path.h"
#include "Visuals.h"
#include <functional>

class CallbackVisitor : public VisualVisitor
{
public:
	CallbackVisitor( std::function<void( Visual & )> callback ) 
		: _callback( callback ) {}

	void visit( const Circle &circle ) override { _callback( const_cast<Circle &>( circle ) ); }
	void visit( const Rectangle &rect ) override { _callback( const_cast<Rectangle &>( rect ) ); }
	void visit( const RoundRectangle &rect ) override { _callback( const_cast<RoundRectangle &>( rect ) ); }
	void visit( const Path &path ) override { _callback( const_cast<Path &>( path ) ); }
	
	void visit( const Visuals &visuals ) override
	{
		_callback( const_cast<Visuals &>( visuals ) );
		for ( const auto &v : visuals )
		{
			v->accept( *this );
		}
	}

private:
	std::function<void( Visual & )> _callback;
};
