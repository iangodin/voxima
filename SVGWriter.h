#pragma once

#include "VisualVisitor.h"
#include "Circle.h"
#include "Rectangle.h"
#include "RoundRectangle.h"
#include "Path.h"
#include "Line.h"
#include "ArrowTip.h"
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
		     << "," << circle.position.y << ") rotate(" << circle.rotation 
		     << ") scale(" << circle.scale_x << "," << circle.scale_y << ")\">\n";
		
		_indent += 2;
		Color final_color = _current_color * circle.color;
		Color final_stroke_color = _current_stroke_color * circle.stroke_color;

		_out << indent() << "<circle cx=\"0\" cy=\"0\" r=\"" << circle.radius << "\" "
		     << color_attributes( final_color ) << " "
		     << stroke_attributes( circle.stroke_thickness, final_stroke_color ) << " />\n";
		_indent -= 2;

		_out << indent() << "</g>\n";
	}

	void visit( const Rectangle &rectangle ) override
	{
		_out << indent() << "<g transform=\"translate(" << rectangle.position.x 
		     << "," << rectangle.position.y << ") rotate(" << rectangle.rotation 
		     << ") scale(" << rectangle.scale_x << "," << rectangle.scale_y << ")\">\n";
		
		_indent += 2;
		Color final_color = _current_color * rectangle.color;
		Color final_stroke_color = _current_stroke_color * rectangle.stroke_color;
		
		float hw = rectangle.half_width;
		float hh = rectangle.half_height;

		_out << indent() << "<rect x=\"" << -hw 
		     << "\" y=\"" << -hh 
		     << "\" width=\"" << hw * 2.0f 
		     << "\" height=\"" << hh * 2.0f << "\" "
		     << color_attributes( final_color ) << " "
		     << stroke_attributes( rectangle.stroke_thickness, final_stroke_color ) << " />\n";
		_indent -= 2;

		_out << indent() << "</g>\n";
	}

	void visit( const RoundRectangle &rect ) override
	{
		_out << indent() << "<g transform=\"translate(" << rect.position.x 
		     << "," << rect.position.y << ") rotate(" << rect.rotation 
		     << ") scale(" << rect.scale_x << "," << rect.scale_y << ")\">\n";
		
		_indent += 2;
		Color final_color = _current_color * rect.color;
		Color final_stroke_color = _current_stroke_color * rect.stroke_color;

		float hw = rect.half_width;
		float hh = rect.half_height;
		float tl = rect.top_left_radius;
		float tr = rect.top_right_radius;
		float bl = rect.bottom_left_radius;
		float br = rect.bottom_right_radius;

		if ( tl == 0 && tr == 0 && bl == 0 && br == 0 )
		{
			_out << indent() << "<rect x=\"" << -hw << "\" y=\"" << -hh 
			     << "\" width=\"" << hw * 2.0f << "\" height=\"" << hh * 2.0f << "\" "
			     << color_attributes( final_color ) << " "
			     << stroke_attributes( rect.stroke_thickness, final_stroke_color ) << " />\n";
		}
		else
		{
			_out << indent() << "<path d=\"M " << -hw + tl << "," << hh
			     << " L " << hw - tr << "," << hh
			     << " A " << tr << "," << tr << " 0 0 0 " << hw << "," << hh - tr
			     << " L " << hw << "," << -hh + br
			     << " A " << br << "," << br << " 0 0 0 " << hw - br << "," << -hh
			     << " L " << -hw + bl << "," << -hh
			     << " A " << bl << "," << bl << " 0 0 0 " << -hw << "," << -hh + bl
			     << " L " << -hw << "," << hh - tl
			     << " A " << tl << "," << tl << " 0 0 0 " << -hw + tl << "," << hh
			     << " Z\" " << color_attributes( final_color ) << " "
			     << stroke_attributes( rect.stroke_thickness, final_stroke_color ) << " />\n";
		}
		_indent -= 2;

		_out << indent() << "</g>\n";
	}

	void visit( const Path &path ) override
	{
		_out << indent() << "<g transform=\"translate(" << path.position.x 
		     << "," << path.position.y << ") rotate(" << path.rotation 
		     << ") scale(" << path.scale_x << "," << path.scale_y << ")\">\n";
		
		_indent += 2;
		Color final_color = _current_color * path.color;
		Color final_stroke_color = _current_stroke_color * path.stroke_color;

		_out << indent() << "<path d=\"";
		for ( size_t i = 0; i < path.commands.size(); ++i )
		{
			const auto &cmd = path.commands[i];
			if ( i > 0 ) _out << " ";
			
			switch ( cmd.type )
			{
				case PathCommand::MoveTo:
					_out << "M " << cmd.to.x << "," << cmd.to.y;
					break;
				case PathCommand::LineTo:
					_out << "L " << cmd.to.x << "," << cmd.to.y;
					break;
				case PathCommand::QuadTo:
					_out << "Q " << cmd.ctrl1.x << "," << cmd.ctrl1.y 
					     << " " << cmd.to.x << "," << cmd.to.y;
					break;
				case PathCommand::CubicTo:
					_out << "C " << cmd.ctrl1.x << "," << cmd.ctrl1.y 
					     << " " << cmd.ctrl2.x << "," << cmd.ctrl2.y 
					     << " " << cmd.to.x << "," << cmd.to.y;
					break;
			}
		}
		_out << " Z\" " << color_attributes( final_color ) << " "
		     << stroke_attributes( path.stroke_thickness, final_stroke_color ) << " />\n";
		
		_indent -= 2;
		_out << indent() << "</g>\n";
	}

	void visit( const Line &line ) override
	{
		_out << indent() << "<g transform=\"translate(" << line.position.x 
		     << "," << line.position.y << ") rotate(" << line.rotation 
		     << ") scale(" << line.scale_x << "," << line.scale_y << ")\">\n";
		
		_indent += 2;
		Color final_stroke_color = _current_stroke_color * line.stroke_color;

		_out << indent() << "<line x1=\"" << line.start.x << "\" y1=\"" << line.start.y 
		     << "\" x2=\"" << line.end.x << "\" y2=\"" << line.end.y << "\" "
		     << stroke_attributes( line.stroke_thickness, final_stroke_color ) << " />\n";
		_indent -= 2;

		_out << indent() << "</g>\n";
	}

	void visit( const ArrowTip &arrow ) override
	{
		_out << indent() << "<g transform=\"translate(" << arrow.position.x 
		     << "," << arrow.position.y << ") rotate(" << arrow.rotation 
		     << ") scale(" << arrow.scale_x << "," << arrow.scale_y << ")\">\n";
		
		_indent += 2;
		Color final_color = _current_color * arrow.color;
		Color final_stroke_color = _current_stroke_color * arrow.stroke_color;

		float dx = arrow.end.x - arrow.start.x;
		float dy = arrow.end.y - arrow.start.y;
		float len = std::sqrt( dx * dx + dy * dy );
		
		if ( len > 0.0001f )
		{
			dx /= len;
			dy /= len;

			// Perpendicular vector
			float px = -dy;
			float py = dx;

			// Tip is at 'end', two other corners are offset by size
			float p1x = arrow.end.x;
			float p1y = arrow.end.y;
			float p2x = arrow.end.x - dx * arrow.size * 2.0f + px * arrow.size;
			float p2y = arrow.end.y - dy * arrow.size * 2.0f + py * arrow.size;
			float p3x = arrow.end.x - dx * arrow.size * 2.0f - px * arrow.size;
			float p3y = arrow.end.y - dy * arrow.size * 2.0f - py * arrow.size;

			_out << indent() << "<path d=\"M " << p1x << "," << p1y 
			     << " L " << p2x << "," << p2y 
			     << " L " << p3x << "," << p3y << " Z\" "
			     << color_attributes( final_color ) << " "
			     << stroke_attributes( arrow.stroke_thickness, final_stroke_color ) << " />\n";
		}
		_indent -= 2;

		_out << indent() << "</g>\n";
	}

	void visit( const Visuals &visuals ) override
	{
		Color old_color = _current_color;
		Color old_stroke_color = _current_stroke_color;
		
		_current_color = _current_color * visuals.color;
		_current_stroke_color = _current_stroke_color * visuals.stroke_color;

		_out << indent() << "<g transform=\"translate(" << visuals.position.x 
		     << "," << visuals.position.y << ") rotate(" << visuals.rotation 
		     << ") scale(" << visuals.scale_x << "," << visuals.scale_y << ")\">\n";
		
		_indent += 2;
		for ( const auto &v : visuals )
		{
			v->accept( *this );
		}
		_indent -= 2;

		_out << indent() << "</g>\n";

		_current_color = old_color;
		_current_stroke_color = old_stroke_color;
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

	static std::string stroke_attributes( float thickness, const Color &c )
	{
		if ( thickness <= 0.0f ) return "";

		std::ostringstream oss;
		oss << "stroke=\"rgb(" 
		    << static_cast<int>( c.r * 255 ) << "," 
		    << static_cast<int>( c.g * 255 ) << "," 
		    << static_cast<int>( c.b * 255 ) << ")\" "
		    << "stroke-width=\"" << thickness << "\" "
		    << "stroke-opacity=\"" << c.a << "\"";
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
	Color _current_stroke_color = { 1.F, 1.F, 1.F, 1.F };
};
