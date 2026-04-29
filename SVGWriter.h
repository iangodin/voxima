#pragma once

#include "VisualVisitor.h"
#include "Circle.h"
#include "Rectangle.h"
#include "RoundRectangle.h"
#include "Ellipse.h"
#include "Path.h"
#include "Line.h"
#include "ArrowTip.h"
#include "Image.h"
#include "Group.h"
#include "Glyph.h"
#include "Color.h"

#include <ostream>
#include <iomanip>
#include <string>
#include <map>

class SVGWriter : public VisualVisitor
{
public:
	SVGWriter( std::ostream &out, int indent_level = 2 )
		: _out( out ), _indent( indent_level )
	{
		_out << std::fixed << std::setprecision( 4 );
	}

	void write_defs( std::ostream &out )
	{
		if ( _glyphs.empty() ) return;

		out << "  <defs>\n";
		for ( const auto &[key, commands] : _glyphs )
		{
			out << "    <path id=\"glyph_" << key.first << "_" << key.second << "\" d=\"";
			emit_path_data( commands, out );
			out << " Z\" />\n";
		}
		out << "  </defs>\n";
	}

	void visit( const Circle &circle ) override
	{
		open_g( circle );

		Color final_color = _current_color * circle.color;
		Color final_stroke_color = _current_stroke_color * circle.stroke_color;

		_out << indent() << "<circle cx=\"0\" cy=\"0\" r=\"" << circle.radius << "\" "
		     << color_attributes( final_color ) << " "
		     << stroke_attributes( circle.stroke_thickness, final_stroke_color ) << " />\n";

		close_g();
	}

	void visit( const Rectangle &rectangle ) override
	{
		open_g( rectangle );

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

		close_g();
	}

	void visit( const RoundRectangle &rect ) override
	{
		open_g( rect );

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

		close_g();
	}

	void visit( const Ellipse &ellipse ) override
	{
		open_g( ellipse );

		Color final_color = _current_color * ellipse.color;
		Color final_stroke_color = _current_stroke_color * ellipse.stroke_color;

		_out << indent() << "<ellipse cx=\"0\" cy=\"0\" rx=\"" << ellipse.half_width
		     << "\" ry=\"" << ellipse.half_height << "\" "
		     << color_attributes( final_color ) << " "
		     << stroke_attributes( ellipse.stroke_thickness, final_stroke_color ) << " />\n";

		close_g();
	}

	void visit( const Path &path ) override
	{
		open_g( path );

		Color final_color = _current_color * path.color;
		Color final_stroke_color = _current_stroke_color * path.stroke_color;

		_out << indent() << "<path d=\"";
		emit_path_data( path.commands, _out );
		_out << " Z\" " << color_attributes( final_color ) << " "
		     << stroke_attributes( path.stroke_thickness, final_stroke_color ) << " />\n";

		close_g();
	}

	void visit( const Glyph &glyph ) override
	{
		auto key = std::make_pair( glyph.font_face, glyph.index );
		if ( !_glyphs.contains( key ) )
		{
			_glyphs[key] = glyph.commands;
		}

		open_g( glyph );

		Color final_color = _current_color * glyph.color;
		Color final_stroke_color = _current_stroke_color * glyph.stroke_color;

		_out << indent() << "<use href=\"#glyph_" << glyph.font_face << "_" << glyph.index << "\" "
		     << color_attributes( final_color ) << " "
		     << stroke_attributes( glyph.stroke_thickness, final_stroke_color ) << " />\n";

		close_g();
	}

	void visit( const Line &line ) override
	{
		open_g( line );

		Color final_stroke_color = _current_stroke_color * line.stroke_color;

		_out << indent() << "<line x1=\"" << line.start.x << "\" y1=\"" << line.start.y
		     << "\" x2=\"" << line.end.x << "\" y2=\"" << line.end.y << "\" "
		     << stroke_attributes( line.stroke_thickness, final_stroke_color ) << " />\n";

		close_g();
	}

	void visit( const ArrowTip &arrow ) override
	{
		open_g( arrow );

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

		close_g();
	}

	void visit( const Image &image ) override
	{
		open_g( image );

		float x = -image.half_width;
		float y = -image.half_height;
		float w = image.half_width * 2.0f;
		float h = image.half_height * 2.0f;

		// Note: We need a transform="scale(1, -1)" on the image itself
		// so it's not flipped upside down by the global scale(1, -1)
		_out << indent() << "<image href=\"" << image.path << "\" x=\"" << x
		     << "\" y=\"" << -y - h << "\" width=\"" << w
		     << "\" height=\"" << h << "\" transform=\"scale(1, -1)\" />\n";

		close_g();
	}

	void visit( const Group &group ) override
	{
		Color old_color = _current_color;
		Color old_stroke_color = _current_stroke_color;

		_current_color = _current_color * group.color;
		_current_stroke_color = _current_stroke_color * group.stroke_color;

		open_g( group );
		for ( const auto &v : group )
		{
			v->accept( *this );
		}
		close_g();

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
	void emit_path_data( const std::vector<PathCommand> &commands, std::ostream &out ) const
	{
		for ( size_t i = 0; i < commands.size(); ++i )
		{
			const auto &cmd = commands[i];
			if ( i > 0 ) out << " ";

			switch ( cmd.type )
			{
				case PathCommand::MoveTo:
					out << "M " << cmd.to.x << "," << cmd.to.y;
					break;
				case PathCommand::LineTo:
					out << "L " << cmd.to.x << "," << cmd.to.y;
					break;
				case PathCommand::QuadTo:
					out << "Q " << cmd.ctrl1.x << "," << cmd.ctrl1.y
					     << " " << cmd.to.x << "," << cmd.to.y;
					break;
				case PathCommand::CubicTo:
					out << "C " << cmd.ctrl1.x << "," << cmd.ctrl1.y
					     << " " << cmd.ctrl2.x << "," << cmd.ctrl2.y
					     << " " << cmd.to.x << "," << cmd.to.y;
					break;
			}
		}
	}

	std::string indent() const
	{
		return std::string( static_cast<std::size_t>(_indent), ' ' );
	}

	void open_g( const Visual &v )
	{
		_out << indent() << "<g transform=\"translate(" << v.position.x
		     << "," << v.position.y << ") rotate(" << v.rotation
		     << ") scale(" << v.scale_x << "," << v.scale_y << ")\">\n";
		_indent += 2;
	}

	void close_g()
	{
		_indent -= 2;
		_out << indent() << "</g>\n";
	}

	std::ostream &_out;
	int _indent;
	Color _current_color = { 1.F, 1.F, 1.F, 1.F };
	Color _current_stroke_color = { 1.F, 1.F, 1.F, 1.F };
	std::map<std::pair<const FontFace *, unsigned int>, std::vector<PathCommand>> _glyphs;
};
