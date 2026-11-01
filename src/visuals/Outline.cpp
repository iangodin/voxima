#include "visuals/Outline.h"

#include "visuals/ArrowTip.h"
#include "visuals/Circle.h"
#include "visuals/Ellipse.h"
#include "visuals/Glyph.h"
#include "visuals/Group.h"
#include "visuals/Image.h"
#include "visuals/Line.h"
#include "visuals/MathAssembly.h"
#include "visuals/Path.h"
#include "visuals/Rectangle.h"
#include "visuals/RoundRectangle.h"
#include "visuals/Tags.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>
#include <type_traits>
#include <variant>

namespace
{

// Control-point distance that makes a cubic follow a quarter circle
constexpr float KAPPA = 0.5522847498F;

// x' = a x + c y + tx, y' = b x + d y + ty
struct Affine
{
	float a = 1.F, b = 0.F, c = 0.F, d = 1.F, tx = 0.F, ty = 0.F;

	Point operator()( Point p ) const
	{
		return { a * p.x() + c * p.y() + tx, b * p.x() + d * p.y() + ty };
	}

	// The transform that applies `local` first, then this one
	Affine operator*( const Affine &local ) const
	{
		return {
			a * local.a + c * local.b,
			b * local.a + d * local.b,
			a * local.c + c * local.d,
			b * local.c + d * local.d,
			a * local.tx + c * local.ty + tx,
			b * local.tx + d * local.ty + ty };
	}

	// How much areas grow, for scaling stroke widths
	float scale() const
	{
		return std::sqrt( std::abs( a * d - b * c ) );
	}
};

// translate * rotate * scale, the order SVGWriter::open_g writes them in
Affine local_transform( const Visual &v )
{
	const float radians = v.rotation * std::numbers::pi_v<float> / 180.F;
	const float cs = std::cos( radians );
	const float sn = std::sin( radians );
	return { cs * v.scale_x, sn * v.scale_x, -sn * v.scale_y, cs * v.scale_y, v.position.x(), v.position.y() };
}

Point lerp( Point a, Point b, float t )
{
	return a + ( b - a ) * t;
}

// FNV-1a over what describes a leaf's geometry
class Hasher
{
public:
	Hasher &add( uint64_t value )
	{
		for ( int i = 0; i < 8; ++i )
		{
			_hash ^= ( value >> ( 8 * i ) ) & 0xFF;
			_hash *= 1099511628211ULL;
		}
		return *this;
	}

	Hasher &add( float value )
	{
		uint32_t bits = 0;
		std::memcpy( &bits, &value, sizeof( bits ) );
		return add( static_cast<uint64_t>( bits ) );
	}

	Hasher &add( Point p )
	{
		return add( p.x() ).add( p.y() );
	}

	uint64_t result() const
	{
		return _hash == 0 ? 1 : _hash;
	}

private:
	uint64_t _hash = 14695981039346656037ULL;
};

uint64_t hash_commands( const std::vector<PathCommand> &list )
{
	Hasher h;
	h.add( static_cast<uint64_t>( 100 ) );
	for ( const auto &command: list )
	{
		h.add( static_cast<uint64_t>( command.index() ) );
		std::visit(
			[&]( const auto &cmd )
			{
				using T = std::decay_t<decltype( cmd )>;
				if constexpr ( std::is_same_v<T, MoveTo> || std::is_same_v<T, LineTo> )
				{
					h.add( cmd.to );
				}
				else if constexpr ( std::is_same_v<T, QuadTo> )
				{
					h.add( cmd.to ).add( cmd.ctrl1 );
				}
				else if constexpr ( std::is_same_v<T, CubicTo> )
				{
					h.add( cmd.to ).add( cmd.ctrl1 ).add( cmd.ctrl2 );
				}
			},
			command );
	}
	return h.result();
}

class ContourBuilder
{
public:
	// `open_strokes`: a contour with no close stays open, as what is only stroked is drawn. Otherwise it is closed, as a
	// fill closes it.
	explicit ContourBuilder( const Affine &xf, bool open_strokes = false ) :
		_xf( xf ),
		_open_strokes( open_strokes )
	{
	}

	std::vector<Contour> &contours()
	{
		finish( !_open_strokes );
		return _contours;
	}

	void move_to( Point p )
	{
		finish( !_open_strokes );
		_current.start = _xf( p );
		_cur = _current.start;
		_open = true;
	}

	void line_to( Point p )
	{
		const Point to = _xf( p );
		push( { lerp( _cur, to, 1.F / 3.F ), lerp( _cur, to, 2.F / 3.F ), to } );
	}

	void quad_to( Point ctrl, Point p )
	{
		const Point q = _xf( ctrl );
		const Point to = _xf( p );
		push( { _cur + ( q - _cur ) * ( 2.F / 3.F ), to + ( q - to ) * ( 2.F / 3.F ), to } );
	}

	void cubic_to( Point c1, Point c2, Point p )
	{
		push( { _xf( c1 ), _xf( c2 ), _xf( p ) } );
	}

	// Quarter circle from p to q around center (p - center and q - center perpendicular, equal length)
	void arc_to( Point p, Point q, Point center )
	{
		const Point u = p - center;
		const Point v = q - center;
		cubic_to( p + v * KAPPA, q + u * KAPPA, q );
	}

	void close()
	{
		finish( true );
	}

	// End the contour where it is, without closing it back to its start (a stroke)
	void end_open()
	{
		finish( false );
	}

private:
	void push( const CubicSeg &seg )
	{
		_current.segs.push_back( seg );
		_cur = seg.to;
	}

	// End the contour being built: close it back to its start (unless it is a stroke) and keep it if it has
	// any length
	void finish( bool closed )
	{
		if ( _open && !_current.segs.empty() )
		{
			if ( closed && !( _cur == _current.start ) )
			{
				push( { lerp( _cur, _current.start, 1.F / 3.F ), lerp( _cur, _current.start, 2.F / 3.F ), _current.start } );
			}
			_current.closed = closed;
			_contours.push_back( std::move( _current ) );
		}
		_current = {};
		_open = false;
	}

	Affine _xf;
	bool _open_strokes = false;
	Contour _current;
	Point _cur;
	bool _open = false;
	std::vector<Contour> _contours;
};

class OutlineBuilder : public VisualVisitor
{
public:
	std::vector<OutlineShape> shapes;
	size_t skipped = 0;
	Color foreground = { 1.F, 1.F, 1.F, 1.F }; // what white is drawn as (see Color::resolved)

	void visit( const Circle &circle ) override
	{
		ellipse( circle, circle.radius, circle.radius, LeafKind::CIRCLE, Hasher().add( static_cast<uint64_t>( 1 ) ).add( circle.radius ).result() );
	}

	void visit( const Ellipse &e ) override
	{
		ellipse( e, e.half_width, e.half_height, LeafKind::ELLIPSE, Hasher().add( static_cast<uint64_t>( 2 ) ).add( e.half_width ).add( e.half_height ).result() );
	}

	void visit( const Rectangle &rect ) override
	{
		const float hw = rect.half_width;
		const float hh = rect.half_height;
		ContourBuilder b( leaf_transform( rect ) );
		b.move_to( { -hw, -hh } );
		b.line_to( { hw, -hh } );
		b.line_to( { hw, hh } );
		b.line_to( { -hw, hh } );
		emit( rect, b, LeafKind::RECTANGLE, Hasher().add( static_cast<uint64_t>( 3 ) ).add( hw ).add( hh ).result() );
	}

	void visit( const RoundRectangle &rect ) override
	{
		const float hw = rect.half_width;
		const float hh = rect.half_height;
		const float tl = rect.top_left_radius;
		const float tr = rect.top_right_radius;
		const float bl = rect.bottom_left_radius;
		const float br = rect.bottom_right_radius;

		// Same outline, corner order and direction as SVGWriter::visit( RoundRectangle )
		ContourBuilder b( leaf_transform( rect ) );
		b.move_to( { -hw + tl, hh } );
		b.line_to( { hw - tr, hh } );
		if ( tr > 0.F ) b.arc_to( { hw - tr, hh }, { hw, hh - tr }, { hw - tr, hh - tr } );
		b.line_to( { hw, -hh + br } );
		if ( br > 0.F ) b.arc_to( { hw, -hh + br }, { hw - br, -hh }, { hw - br, -hh + br } );
		b.line_to( { -hw + bl, -hh } );
		if ( bl > 0.F ) b.arc_to( { -hw + bl, -hh }, { -hw, -hh + bl }, { -hw + bl, -hh + bl } );
		b.line_to( { -hw, hh - tl } );
		if ( tl > 0.F ) b.arc_to( { -hw, hh - tl }, { -hw + tl, hh }, { -hw + tl, hh - tl } );
		emit( rect, b, LeafKind::ROUND_RECTANGLE, Hasher().add( static_cast<uint64_t>( 4 ) ).add( hw ).add( hh ).add( tl ).add( tr ).add( bl ).add( br ).result() );
	}

	void visit( const Path &path ) override
	{
		commands( path, path.commands, LeafKind::PATH );
	}

	void visit( const Glyph &glyph ) override
	{
		commands( glyph, glyph.commands, LeafKind::GLYPH );
	}

	void visit( const Line &line ) override
	{
		// Stroke only: an open contour, and nothing to fill
		ContourBuilder b( leaf_transform( line ) );
		b.move_to( line.start );
		b.line_to( line.end );
		b.end_open();
		emit( line, b, LeafKind::LINE, Hasher().add( static_cast<uint64_t>( 5 ) ).add( line.start ).add( line.end ).result(), true );
	}

	void visit( const ArrowTip &arrow ) override
	{
		float dx = arrow.end.x() - arrow.start.x();
		float dy = arrow.end.y() - arrow.start.y();
		const float len = std::sqrt( dx * dx + dy * dy );
		if ( len <= 0.0001F )
		{
			return; // the SVG writer draws nothing either
		}
		dx /= len;
		dy /= len;
		const float px = -dy;
		const float py = dx;

		// The tip is at `end`, the other two corners are back along the arrow, either side (as in SVGWriter)
		ContourBuilder b( leaf_transform( arrow ) );
		b.move_to( arrow.end );
		b.line_to( { arrow.end.x() - dx * arrow.size * 2.F + px * arrow.size, arrow.end.y() - dy * arrow.size * 2.F + py * arrow.size } );
		b.line_to( { arrow.end.x() - dx * arrow.size * 2.F - px * arrow.size, arrow.end.y() - dy * arrow.size * 2.F - py * arrow.size } );
		emit( arrow, b, LeafKind::ARROW_TIP, Hasher().add( static_cast<uint64_t>( 6 ) ).add( arrow.start ).add( arrow.end ).add( arrow.size ).result() );
	}

	void visit( const Image &image ) override
	{
		// An Image has no outline, but it can be told where it has got to: undo the transforms into a
		// position, rotation and scale (which a rotated, unevenly scaled group makes impossible)
		const Affine m = leaf_transform( image );
		const float sx = std::hypot( m.a, m.b );
		if ( sx < 1e-9F )
		{
			++skipped;
			return;
		}
		const float sy = ( m.a * m.d - m.b * m.c ) / sx;
		const float shear = ( m.a * m.c + m.b * m.d ) / ( sx * std::max( std::abs( sy ), 1e-9F ) );
		if ( std::abs( shear ) > 1e-3F )
		{
			++skipped;
			return;
		}

		OutlineShape shape;
		shape.image = ImageInfo{
			image.path,
			image.half_width,
			image.half_height,
			{ m.tx, m.ty },
			std::atan2( m.b, m.a ) * 180.F / std::numbers::pi_v<float>,
			sx,
			sy };
		shape.fill = ( _color * image.color ).resolved( foreground );
		shape.stroke = ( _stroke * image.stroke_color ).resolved( foreground );
		shape.word = _word;
		shape.tags = _tags;
		shape.tags.insert( shape.tags.end(), image.tags.begin(), image.tags.end() );
		shape.kind = LeafKind::IMAGE;
		Hasher h;
		h.add( static_cast<uint64_t>( 7 ) );
		for ( char c: image.path )
		{
			h.add( static_cast<uint64_t>( static_cast<unsigned char>( c ) ) );
		}
		shape.key = h.add( image.half_width ).add( image.half_height ).result();
		shapes.push_back( std::move( shape ) );
	}

	void visit( const Group &group ) override
	{
		enter( group );
	}

	void visit( const MathAssembly &assembly ) override
	{
		enter( assembly );
	}

private:
	Affine leaf_transform( const Visual &v ) const
	{
		return _xf * local_transform( v );
	}

	void ellipse( const Visual &v, float rx, float ry, LeafKind kind, uint64_t key )
	{
		// Starts at the right and runs the way SVG draws circles
		ContourBuilder b( leaf_transform( v ) );
		b.move_to( { rx, 0.F } );
		b.cubic_to( { rx, ry * KAPPA }, { rx * KAPPA, ry }, { 0.F, ry } );
		b.cubic_to( { -rx * KAPPA, ry }, { -rx, ry * KAPPA }, { -rx, 0.F } );
		b.cubic_to( { -rx, -ry * KAPPA }, { -rx * KAPPA, -ry }, { 0.F, -ry } );
		b.cubic_to( { rx * KAPPA, -ry }, { rx, -ry * KAPPA }, { rx, 0.F } );
		emit( v, b, kind, key );
	}

	void commands( const Visual &v, const std::vector<PathCommand> &list, LeafKind kind )
	{
		ContourBuilder b( leaf_transform( v ), v.color.a <= 0.F && v.stroke_thickness > 0.F );
		for ( const auto &command: list )
		{
			std::visit(
				[&]( const auto &cmd )
				{
					using T = std::decay_t<decltype( cmd )>;
					if constexpr ( std::is_same_v<T, MoveTo> )
					{
						b.move_to( cmd.to );
					}
					else if constexpr ( std::is_same_v<T, LineTo> )
					{
						b.line_to( cmd.to );
					}
					else if constexpr ( std::is_same_v<T, QuadTo> )
					{
						b.quad_to( cmd.ctrl1, cmd.to );
					}
					else if constexpr ( std::is_same_v<T, CubicTo> )
					{
						b.cubic_to( cmd.ctrl1, cmd.ctrl2, cmd.to );
					}
					else
					{
						b.close();
					}
				},
				command );
		}
		emit( v, b, kind, hash_commands( list ) );
	}

	void emit( const Visual &v, ContourBuilder &b, LeafKind kind, uint64_t key, bool stroke_only = false )
	{
		OutlineShape shape;
		shape.contours = std::move( b.contours() );
		if ( shape.contours.empty() )
		{
			return;
		}
		shape.fill = ( _color * v.color ).resolved( foreground );
		if ( stroke_only )
		{
			shape.fill.a = 0.F;
		}
		else if ( v.fill_gradient && !v.fill_gradient->stops.empty() )
		{
			const Color seen = _color * v.color;
			const Color tint{ seen.r, seen.g, seen.b, 1.F };
			const Affine m = leaf_transform( v );
			Gradient gradient = *v.fill_gradient;
			gradient.transform = GradientTransform{ m.a, m.b, m.c, m.d, m.tx, m.ty } * gradient.transform;
			for ( GradientStop &stop: gradient.stops )
			{
				stop.color = ( stop.color * tint ).resolved( foreground );
				stop.color.a *= seen.a;
			}
			shape.fill = gradient.average();
			shape.gradient = std::move( gradient );
		}
		shape.stroke = ( _stroke * v.stroke_color ).resolved( foreground );
		const float stroke_scale = leaf_transform( v ).scale();
		shape.stroke_thickness = v.stroke_thickness * stroke_scale;
		shape.dash = v.stroke_dash;
		for ( float &length: shape.dash.pattern )
		{
			length *= stroke_scale;
		}
		shape.dash.offset *= stroke_scale;
		shape.word = _word;
		shape.tags = _tags;
		shape.tags.insert( shape.tags.end(), v.tags.begin(), v.tags.end() );
		shape.kind = kind;
		shape.key = key;
		shapes.push_back( std::move( shape ) );
	}

	void enter( const Group &group )
	{
		const Affine old_xf = _xf;
		const Color old_color = _color;
		const Color old_stroke = _stroke;
		const size_t old_tags = _tags.size();
		const int old_word = _word;

		_tags.insert( _tags.end(), group.tags.begin(), group.tags.end() );
		if ( std::any_of( group.tags.begin(), group.tags.end(), []( const std::string &t ) { return is_word_tag( t ); } ) )
		{
			_word = _next_word++;
		}
		_xf = _xf * local_transform( group );
		_color = _color * group.color;
		_stroke = _stroke * group.stroke_color;
		for ( const auto &child: group )
		{
			child->accept( *this );
		}

		_xf = old_xf;
		_color = old_color;
		_stroke = old_stroke;
		_tags.resize( old_tags );
		_word = old_word;
	}

	int _word = -1;
	int _next_word = 0;
	std::vector<std::string> _tags;
	Affine _xf;
	Color _color;
	Color _stroke;
};

}

std::vector<OutlineShape> to_outline( const Visual &visual, size_t *skipped, const Color &foreground )
{
	OutlineBuilder builder;
	builder.foreground = foreground;
	visual.accept( builder );
	if ( skipped )
	{
		*skipped = builder.skipped;
	}
	return std::move( builder.shapes );
}

Point place_in_parent( const Visual &visual, Point p )
{
	return local_transform( visual )( p );
}

double signed_area( const Contour &contour )
{
	// Green's theorem: half the integral of x dy - y dx. On a cubic the integrand is a degree-5
	// polynomial, which 3-point Gauss-Legendre integrates exactly.
	constexpr double NODES[3] = { 0.5 - 0.5 * 0.7745966692414834, 0.5, 0.5 + 0.5 * 0.7745966692414834 };
	constexpr double WEIGHTS[3] = { 5.0 / 18.0, 8.0 / 18.0, 5.0 / 18.0 };

	double area = 0.0;
	Point p0 = contour.start;
	for ( const auto &seg: contour.segs )
	{
		const double x0 = p0.x(), y0 = p0.y();
		const double x1 = seg.c1.x(), y1 = seg.c1.y();
		const double x2 = seg.c2.x(), y2 = seg.c2.y();
		const double x3 = seg.to.x(), y3 = seg.to.y();
		for ( int i = 0; i < 3; ++i )
		{
			const double t = NODES[i];
			const double u = 1.0 - t;
			const double x = u * u * u * x0 + 3 * u * u * t * x1 + 3 * u * t * t * x2 + t * t * t * x3;
			const double y = u * u * u * y0 + 3 * u * u * t * y1 + 3 * u * t * t * y2 + t * t * t * y3;
			const double dx = 3 * ( u * u * ( x1 - x0 ) + 2 * u * t * ( x2 - x1 ) + t * t * ( x3 - x2 ) );
			const double dy = 3 * ( u * u * ( y1 - y0 ) + 2 * u * t * ( y2 - y1 ) + t * t * ( y3 - y2 ) );
			area += WEIGHTS[i] * ( x * dy - y * dx ) / 2.0;
		}
		p0 = seg.to;
	}

	// An open contour encloses what it would with a straight line back to its start
	if ( !contour.closed && !contour.segs.empty() )
	{
		const Point end = contour.segs.back().to;
		area += ( static_cast<double>( end.x() ) * contour.start.y() - static_cast<double>( contour.start.x() ) * end.y() ) / 2.0;
	}
	return area;
}

AABB on_curve_bounds( const Contour &contour )
{
	AABB box;
	box.add( contour.start );
	for ( const auto &seg: contour.segs )
	{
		box.add( seg.to );
	}
	return box;
}

Contour reversed( const Contour &contour )
{
	Contour out;
	out.closed = contour.closed;
	if ( contour.segs.empty() )
	{
		out.start = contour.start;
		return out;
	}

	// Segment k runs from prev(k) to segs[k].to; walked backwards it runs to prev(k) with the control
	// points swapped. A loop is walked back from its start (which is also the end of its last segment); an
	// open contour from its end.
	out.start = contour.closed ? contour.start : contour.segs.back().to;
	for ( size_t k = contour.segs.size(); k-- > 0; )
	{
		const Point end = k == 0 ? contour.start : contour.segs[k - 1].to;
		out.segs.push_back( { contour.segs[k].c2, contour.segs[k].c1, end } );
	}
	return out;
}

Contour closed_loop( const Contour &contour )
{
	if ( contour.closed || contour.segs.empty() )
	{
		return contour;
	}
	Contour out = contour;
	out.closed = true;
	const Contour back = reversed( contour );
	out.segs.insert( out.segs.end(), back.segs.begin(), back.segs.end() );
	return out;
}
