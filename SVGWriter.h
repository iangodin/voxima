
#pragma once

#include "VisualVisitor.h"
#include "Circle.h"
#include "Rectangle.h"
#include "Visuals.h"
#include "Color.h"

#include <ostream>
#include <iomanip>
#include <string>

class SVGWriter : public VisualVisitor
{
public:
	SVGWriter( std::ostream &out, int indent_level = 2 ) 
		: _out( out ), _indent( indent_level ) 
	{
		_out << std::fixed << std::setprecision( 4 );
	}

	void visit( const Circle &circle ) override
	{
		_out << indent() << "<g transform=\"translate(" << circle.position.x 
		     << "," << circle.position.y << ") rotate(" << circle.rotation << ")\">\n";
		
		_indent += 2;
		Color final_color = _current_color * circle.color;
		_out << indent() << "<circle cx=\"0\" cy=\"0\" r=\"" << circle.radius << "\" "
		     << color_attributes( final_color ) << " />\n";
		_indent -= 2;

		_out << indent() << "</g>\n";
	}

	void visit( const Rectangle &rectangle ) override
	{
		_out << indent() << "<g transform=\"translate(" << rectangle.position.x 
		     << "," << rectangle.position.y << ") rotate(" << rectangle.rotation << ")\">\n";
		
		_indent += 2;
		Color final_color = _current_color * rectangle.color;
		float x = -rectangle.half_width;
		float y = -rectangle.half_height;
		float w = rectangle.half_width * 2.0f;
		float h = rectangle.half_height * 2.0f;

		_out << indent() << "<rect x=\"" << x 
		     << "\" y=\"" << y 
		     << "\" width=\"" << w 
		     << "\" height=\"" << h << "\" "
		     << color_attributes( final_color ) << " />\n";
		_indent -= 2;

		_out << indent() << "</g>\n";
	}

	void visit( const Visuals &visuals ) override
	{
		Color old_color = _current_color;
		_current_color = _current_color * visuals.color;

		_out << indent() << "<g transform=\"translate(" << visuals.position.x 
		     << "," << visuals.position.y << ") rotate(" << visuals.rotation << ")\">\n";
		
		_indent += 2;
		for ( const auto &v : visuals )
		{
			v->accept( *this );
		}
		_indent -= 2;

		_out << indent() << "</g>\n";

		_current_color = old_color;
	}

	static std::string color_attributes( const Color &c )
	{
		std::ostringstream oss;
		oss << "fill=\"rgb(" 
		    << static_cast<int>( c.r * 255 ) << "," 
		    << static_cast<int>( c.g * 255 ) << "," 
		    << static_cast<int>( c.b * 255 ) << ")\" "
		    << "fill-opacity=\"" << c.a << "\"";
		return oss.str();
	}

private:
	std::string indent() const
	{
		return std::string( _indent, ' ' );
	}

	std::ostream &_out;
	int _indent;
	Color _current_color = { 1.F, 1.F, 1.F, 1.F };
};
