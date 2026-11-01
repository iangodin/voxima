#pragma once

#include "VisualVisitor.h"
#include "Circle.h"
#include "Rectangle.h"
#include "RoundRectangle.h"
#include "Path.h"
#include "Line.h"
#include "ArrowTip.h"
#include "Glyph.h"
#include "Ellipse.h"
#include "Image.h"
#include "Group.h"
#include "MathAssembly.h"
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
	void visit( const Line &line ) override { _callback( const_cast<Line &>( line ) ); }
	void visit( const ArrowTip &arrow ) override { _callback( const_cast<ArrowTip &>( arrow ) ); }
	void visit( const Glyph &glyph ) override { _callback( const_cast<Glyph &>( glyph ) ); }
	
	void visit( const Ellipse &ellipse ) override { _callback( const_cast<Ellipse &>( ellipse ) ); }
	void visit( const Image &image ) override { _callback( const_cast<Image &>( image ) ); }
	void visit( const Group &visuals ) override
	{
		_callback( const_cast<Group &>( visuals ) );
		for ( const auto &v : visuals )
		{
			v->accept( *this );
		}
	}
	void visit( const MathAssembly &assembly ) override { visit( static_cast<const Group &>( assembly ) ); }

private:
	std::function<void( Visual & )> _callback;
};
