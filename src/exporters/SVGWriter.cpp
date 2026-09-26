
//#define CLIPPER_UNION_GLYPHS

#include "exporters/SVGWriter.h"
#include "visuals/MathAssembly.h"
#include "visuals/Polyline.h"

#ifdef CLIPPER_UNION_GLYPHS
#include <clipper2/clipper.h>
#endif

#include <algorithm>
#include <cmath>
#include <sstream>

namespace
{
class PolylineCollector : public VisualVisitor
{
public:
	std::vector<Polyline> polylines;
	Point offset = { 0, 0 };

	void add_commands( const std::vector<PathCommand> &commands, Point p, float sx, float sy )
	{
		Polyline *current = nullptr;
		for ( const auto &cmd: commands )
		{
			std::visit(
				[&]( auto &&arg )
				{
					using T = std::decay_t<decltype( arg )>;
					if constexpr ( std::is_same_v<T, MoveTo> )
					{
						polylines.emplace_back();
						current = &polylines.back();
						current->addPoint( { ( p.x() + arg.to.x() ) * sx, ( p.y() + arg.to.y() ) * sy } );
					}
					else if constexpr ( std::is_same_v<T, LineTo> )
					{
						if ( current )
							current->addPoint( { ( p.x() + arg.to.x() ) * sx, ( p.y() + arg.to.y() ) * sy } );
					}
					else if constexpr ( std::is_same_v<T, QuadTo> )
					{
						if ( current )
						{
							Point p0 = current->back();
							Point p1 = { ( p.x() + arg.ctrl1.x() ) * sx, ( p.y() + arg.ctrl1.y() ) * sy };
							Point p2 = { ( p.x() + arg.to.x() ) * sx, ( p.y() + arg.to.y() ) * sy };
							current->addQuadratic( p0, p1, p2 );
						}
					}
					else if constexpr ( std::is_same_v<T, CubicTo> )
					{
						if ( current )
						{
							Point p0 = current->back();
							Point p1 = { ( p.x() + arg.ctrl1.x() ) * sx, ( p.y() + arg.ctrl1.y() ) * sy };
							Point p2 = { ( p.x() + arg.ctrl2.x() ) * sx, ( p.y() + arg.ctrl2.y() ) * sy };
							Point p3 = { ( p.x() + arg.to.x() ) * sx, ( p.y() + arg.to.y() ) * sy };
							current->addCubic( p0, p1, p2, p3 );
						}
					}
					else if constexpr ( std::is_same_v<T, Close> )
					{
						if ( current )
						{
							current->close();
							current = nullptr;
						}
					}
				},
				cmd );
		}
	}

	void visit( const Glyph &glyph ) override { add_commands( glyph.commands, offset + glyph.position, glyph.scale_x, glyph.scale_y ); }
	void visit( const Path &path ) override { add_commands( path.commands, offset + path.position, path.scale_x, path.scale_y ); }
	void visit( const Group &group ) override
	{
		Point old_offset = offset;
		offset = offset + group.position;
		for ( const auto &v: group ) v->accept( *this );
		offset = old_offset;
	}
	void visit( const MathAssembly &assembly ) override { visit( static_cast<const Group &>( assembly ) ); }

	void visit( const Circle & ) override {}
	void visit( const Rectangle & ) override {}
	void visit( const RoundRectangle & ) override {}
	void visit( const Ellipse & ) override {}
	void visit( const Line & ) override {}
	void visit( const ArrowTip & ) override {}
	void visit( const Image & ) override {}
};
}

SVGWriter::SVGWriter( std::ostream &out, int indent_level, float flatness )
    : _out( out ), _indent( indent_level ), _flatness( flatness )
{
	_out << std::fixed << std::setprecision( 4 );
}

void SVGWriter::write_defs( std::ostream &out )
{
	if ( _glyph_paths.empty() ) return;

	out << "  <defs>\n";
	for ( size_t id = 0; id < _glyph_paths.size(); ++id )
	{
		out << "    <path id=\"glyph_" << id << "\" d=\"" << _glyph_paths[id] << "\" />\n";
	}
	out << "  </defs>\n";
}

void SVGWriter::visit( const Circle &circle )
{
	if ( open_g( circle, false ) )
	{
		Color final_color = _current_color * circle.color;
		Color final_stroke_color = _current_stroke_color * circle.stroke_color;

		_out << indent() << "<circle cx=\"0\" cy=\"0\" r=\"" << circle.radius << "\" "
		     << color_attributes( final_color ) << " "
		     << stroke_attributes( circle.stroke_thickness, final_stroke_color ) << " />\n";

		close_g();
	}
}

void SVGWriter::visit( const Rectangle &rectangle )
{
	if ( open_g( rectangle, false ) )
	{
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
}

void SVGWriter::visit( const RoundRectangle &rect )
{
	if ( open_g( rect, false ) )
	{
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
}

void SVGWriter::visit( const Ellipse &ellipse )
{
	if ( open_g( ellipse, false ) )
	{
		Color final_color = _current_color * ellipse.color;
		Color final_stroke_color = _current_stroke_color * ellipse.stroke_color;

		_out << indent() << "<ellipse cx=\"0\" cy=\"0\" rx=\"" << ellipse.half_width
		     << "\" ry=\"" << ellipse.half_height << "\" "
		     << color_attributes( final_color ) << " "
		     << stroke_attributes( ellipse.stroke_thickness, final_stroke_color ) << " />\n";

		close_g();
	}
}

void SVGWriter::visit( const Path &path )
{
	if ( open_g( path, false ) )
	{
		Color final_color = _current_color * path.color;
		Color final_stroke_color = _current_stroke_color * path.stroke_color;

		_out << indent() << "<path d=\"";
		emit_path_data( path.commands, _out );
		_out << "\" " << color_attributes( final_color ) << " "
		     << stroke_attributes( path.stroke_thickness, final_stroke_color );
		if ( path.stroke_thickness > 0.F && path.join != LineJoin::MITER )
		{
			_out << " stroke-linejoin=\"" << ( path.join == LineJoin::BEVEL ? "bevel" : "round" ) << "\"";
		}
		_out << " />\n";

		close_g();
	}
}

void SVGWriter::visit( const Glyph &glyph )
{
	std::ostringstream path;
	emit_path_data( glyph.commands, path );
	const auto [entry, is_new] = _glyph_ids.emplace( path.str(), _glyph_ids.size() );
	if ( is_new )
	{
		_glyph_paths.push_back( entry->first );
	}
	const size_t id = entry->second;

	if ( open_g( glyph, false ) )
	{
		Color final_color = _current_color * glyph.color;
		Color final_stroke_color = _current_stroke_color * glyph.stroke_color;

		_out << indent() << "<use href=\"#glyph_" << id << "\" "
		     << color_attributes( final_color ) << " "
		     << stroke_attributes( glyph.stroke_thickness, final_stroke_color ) << " />\n";

		close_g();
	}
}

void SVGWriter::visit( const Line &line )
{
	if ( open_g( line, false ) )
	{
		Color final_stroke_color = _current_stroke_color * line.stroke_color;

		_out << indent() << "<line x1=\"" << line.start.x() << "\" y1=\"" << line.start.y()
		     << "\" x2=\"" << line.end.x() << "\" y2=\"" << line.end.y() << "\" "
		     << stroke_attributes( line.stroke_thickness, final_stroke_color ) << " />\n";

		close_g();
	}
}

void SVGWriter::visit( const ArrowTip &arrow )
{
	if ( open_g( arrow, false ) )
	{
		Color final_color = _current_color * arrow.color;
		Color final_stroke_color = _current_stroke_color * arrow.stroke_color;

		float dx = arrow.end.x() - arrow.start.x();
		float dy = arrow.end.y() - arrow.start.y();
		float len = std::sqrt( dx * dx + dy * dy );

		if ( len > 0.0001f )
		{
			dx /= len;
			dy /= len;

			// Perpendicular vector
			float px = -dy;
			float py = dx;

			// Tip is at 'end', two other corners are offset by size
			float p1x = arrow.end.x();
			float p1y = arrow.end.y();
			float p2x = arrow.end.x() - dx * arrow.size * 2.0f + px * arrow.size;
			float p2y = arrow.end.y() - dy * arrow.size * 2.0f + py * arrow.size;
			float p3x = arrow.end.x() - dx * arrow.size * 2.0f - px * arrow.size;
			float p3y = arrow.end.y() - dy * arrow.size * 2.0f - py * arrow.size;

			_out << indent() << "<path d=\"M " << p1x << "," << p1y
			     << " L " << p2x << "," << p2y
			     << " L " << p3x << "," << p3y << " Z\" "
			     << color_attributes( final_color ) << " "
			     << stroke_attributes( arrow.stroke_thickness, final_stroke_color ) << " />\n";
		}

		close_g();
	}
}

void SVGWriter::visit( const Image &image )
{
	if ( open_g( image, false ) )
	{
		float x = -image.half_width;
		float y = -image.half_height;
		float w = image.half_width * 2.0f;
		float h = image.half_height * 2.0f;

		// Note: We need a transform="scale(1, -1)" on the image itself
		// so it's not flipped upside down by the global scale(1, -1)
		// The alpha of the color (the image's own times its groups') fades the picture
		const float opacity = ( _current_color * image.color ).a;

		_out << indent() << "<image href=\"" << image.path << "\" x=\"" << x
		     << "\" y=\"" << -y - h << "\" width=\"" << w
		     << "\" height=\"" << h << "\" transform=\"scale(1, -1)\"";
		// Nearly opaque is written as opaque: the rasteriser draws a picture with any opacity another way (a
		// pixel off), so 0.9999 would jump the picture for a difference no one can see
		if ( opacity < 0.999F )
		{
			_out << " opacity=\"" << opacity << "\"";
		}
		_out << " />\n";

		close_g();
	}
}

void SVGWriter::visit( const Group &group )
{
	Color old_color = _current_color;
	Color old_stroke_color = _current_stroke_color;

	_current_color = _current_color * group.color;
	_current_stroke_color = _current_stroke_color * group.stroke_color;

	open_g( group, true );
	for ( const auto &v: group )
	{
		v->accept( *this );
	}
	close_g();

	_current_color = old_color;
	_current_stroke_color = old_stroke_color;
}

void SVGWriter::visit( const MathAssembly &assembly )
{
#ifdef CLIPPER_UNION_GLYPHS
	PolylineCollector collector;
	for ( const auto &v: assembly ) v->accept( collector );

	Clipper2Lib::PathsD subject;
	for ( const auto &poly: collector.polylines )
	{
		Clipper2Lib::PathD path;
		for ( const auto &p: poly ) path.push_back( { (double)p.x(), (double)p.y() } );
		subject.push_back( path );
	}

	Clipper2Lib::PathsD unioned = Clipper2Lib::Union( subject, Clipper2Lib::FillRule::NonZero );

	if ( open_g( assembly, false ) )
	{
		Color final_color = _current_color * assembly.color;
		Color final_stroke_color = _current_stroke_color * assembly.stroke_color;

		_out << indent() << "<path d=\"";
		for ( size_t i = 0; i < unioned.size(); ++i )
		{
			if ( i > 0 ) _out << " ";
			const auto &path = unioned[i];
			if ( path.empty() ) continue;
			_out << "M " << path[0].x << "," << path[0].y;
			for ( size_t j = 1; j < path.size(); ++j ) _out << " L " << path[j].x << "," << path[j].y;
			_out << " Z";
		}
		_out << "\" " << color_attributes( final_color ) << " "
		     << stroke_attributes( assembly.stroke_thickness, final_stroke_color ) << " />\n";
		close_g();
	}
#else
	Color old_color = _current_color;
	Color old_stroke_color = _current_stroke_color;

	_current_color = _current_color * assembly.color;
	_current_stroke_color = _current_stroke_color * assembly.stroke_color;

	open_g( assembly, true );
	for ( const auto &v : assembly )
	{
		v->accept( *this );
	}
	close_g();

	_current_color = old_color;
	_current_stroke_color = old_stroke_color;
#endif
}

std::string SVGWriter::color_attributes( const Color &c )
{
	std::ostringstream oss;
	oss << "fill=\"rgb("
	    << static_cast<int>( c.r * 255 ) << ","
	    << static_cast<int>( c.g * 255 ) << ","
	    << static_cast<int>( c.b * 255 ) << ")\" "
	    << "fill-opacity=\"" << c.a << "\"";
	return oss.str();
}

std::string SVGWriter::stroke_attributes( float thickness, const Color &c )
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

namespace
{
// Distance from p to the line through a and b (or to a, if they are the same point)
float off_line( Point p, Point a, Point b )
{
	const Point ab = b - a;
	const float len = std::sqrt( ab.lengthSquared() );
	if ( len < 1e-12F )
	{
		return std::sqrt( ( p - a ).lengthSquared() );
	}
	return std::abs( ( p - a ).x() * ab.y() - ( p - a ).y() * ab.x() ) / len;
}

Point bezier( Point p0, Point c1, Point c2, Point p3, float t )
{
	const float u = 1.F - t;
	return p0 * ( u * u * u ) + c1 * ( 3.F * u * u * t ) + c2 * ( 3.F * u * t * t ) + p3 * ( t * t * t );
}

Point bezier_derivative( Point p0, Point c1, Point c2, Point p3, float t )
{
	const float u = 1.F - t;
	return ( c1 - p0 ) * ( 3.F * u * u ) + ( c2 - c1 ) * ( 6.F * u * t ) + ( p3 - c2 ) * ( 3.F * t * t );
}
}

void SVGWriter::emit_path_data( const std::vector<PathCommand> &commands, std::ostream &out ) const
{
	Point current;
	Point start;

	// A cubic from `current`, cut into equal pieces until each is within the flatness of a straight line (a
	// cubic strays from its chord by at most 3/4 of its control points' distance from it, and halving it cuts
	// that by four). Each piece is exact: its ends and tangents are the curve's own.
	auto cubic = [&]( Point c1, Point c2, Point to )
	{
		const Point p0 = current;
		size_t pieces = 1;
		if ( _flatness > 0.F )
		{
			const float stray = 0.75F * std::max( off_line( c1, p0, to ), off_line( c2, p0, to ) );
			pieces = static_cast<size_t>( std::clamp( std::ceil( std::sqrt( stray / _flatness ) ), 1.F, 64.F ) );
		}
		for ( size_t k = 0; k < pieces; ++k )
		{
			const float t0 = static_cast<float>( k ) / static_cast<float>( pieces );
			const float t1 = static_cast<float>( k + 1 ) / static_cast<float>( pieces );
			const float third = ( t1 - t0 ) / 3.F;
			const Point a = k == 0 ? p0 : bezier( p0, c1, c2, to, t0 );
			const Point b = k + 1 == pieces ? to : bezier( p0, c1, c2, to, t1 );
			const Point q1 = pieces == 1 ? c1 : a + bezier_derivative( p0, c1, c2, to, t0 ) * third;
			const Point q2 = pieces == 1 ? c2 : b - bezier_derivative( p0, c1, c2, to, t1 ) * third;
			out << ( k > 0 ? " " : "" ) << "C " << q1.x() << "," << q1.y() << " " << q2.x() << "," << q2.y() << " " << b.x() << "," << b.y();
		}
		current = to;
	};

	for ( size_t i = 0; i < commands.size(); ++i )
	{
		const auto &cmd = commands[i];
		if ( i > 0 ) out << " ";

		std::visit(
			[&]( auto &&arg )
			{
				using T = std::decay_t<decltype( arg )>;
				if constexpr ( std::is_same_v<T, MoveTo> )
				{
					out << "M " << arg.to.x() << "," << arg.to.y();
					current = start = arg.to;
				}
				else if constexpr ( std::is_same_v<T, LineTo> )
				{
					out << "L " << arg.to.x() << "," << arg.to.y();
					current = arg.to;
				}
				else if constexpr ( std::is_same_v<T, QuadTo> )
				{
					if ( _flatness > 0.F )
					{
						// As the cubic it is exactly, so it is cut the same way
						cubic( current + ( arg.ctrl1 - current ) * ( 2.F / 3.F ), arg.to + ( arg.ctrl1 - arg.to ) * ( 2.F / 3.F ), arg.to );
					}
					else
					{
						out << "Q " << arg.ctrl1.x() << "," << arg.ctrl1.y()
						    << " " << arg.to.x() << "," << arg.to.y();
						current = arg.to;
					}
				}
				else if constexpr ( std::is_same_v<T, CubicTo> )
				{
					cubic( arg.ctrl1, arg.ctrl2, arg.to );
				}
				else if constexpr ( std::is_same_v<T, Close> )
				{
					out << "Z";
					current = start;
				}
			},
			cmd );
	}
}

std::string SVGWriter::indent() const
{
	return std::string( static_cast<std::size_t>( _indent ), ' ' );
}

bool SVGWriter::open_g( const Visual &v, bool force_g )
{
	std::vector<std::string> parts;
	if ( v.position.x() != 0.F || v.position.y() != 0.F )
	{
		std::ostringstream oss;
		oss << "translate(" << v.position.x() << "," << v.position.y() << ")";
		parts.push_back( oss.str() );
	}

	// A rotation or scale too small to see is left out. Besides saving space, a picture is drawn a pixel off
	// under any transform but none (the rasteriser samples it another way), so a scale of 0.99998 on the way
	// to 1 would make it jump.
	if ( std::abs( v.rotation ) >= 1e-3F )
	{
		std::ostringstream oss;
		oss << "rotate(" << v.rotation << ")";
		parts.push_back( oss.str() );
	}

	if ( std::abs( v.scale_x - 1.F ) >= 1e-4F || std::abs( v.scale_y - 1.F ) >= 1e-4F )
	{
		std::ostringstream oss;
		oss << "scale(" << v.scale_x << "," << v.scale_y << ")";
		parts.push_back( oss.str() );
	}

	if ( parts.empty() && !force_g )
	{
		_open_g_count.push_back( false );
		return true;
	}

	_out << indent() << "<g";
	if ( !parts.empty() )
	{
		_out << " transform=\"";
		for ( size_t i = 0; i < parts.size(); ++i )
		{
			if ( i > 0 ) _out << " ";
			_out << parts[i];
		}
		_out << "\"";
	}
	_out << ">\n";
	_indent += 2;
	_open_g_count.push_back( true );
	return true;
}

void SVGWriter::close_g()
{
	if ( _open_g_count.back() )
	{
		_indent -= 2;
		_out << indent() << "</g>\n";
	}
	_open_g_count.pop_back();
}
