#pragma once

#include "visuals/VisualVisitor.h"
#include "visuals/Circle.h"
#include "visuals/Rectangle.h"
#include "visuals/RoundRectangle.h"
#include "visuals/Ellipse.h"
#include "visuals/Path.h"
#include "visuals/Line.h"
#include "visuals/ArrowTip.h"
#include "visuals/Image.h"
#include "visuals/Group.h"
#include "visuals/Glyph.h"
#include "visuals/Color.h"

#include <ostream>
#include <iomanip>
#include <string>
#include <map>
#include <vector>

class SVGWriter : public VisualVisitor
{
public:
	SVGWriter( std::ostream &out, int indent_level = 2 );

	void write_defs( std::ostream &out );

	void visit( const Circle &circle ) override;
	void visit( const Rectangle &rectangle ) override;
	void visit( const RoundRectangle &rectangle ) override;
	void visit( const Ellipse &ellipse ) override;
	void visit( const Path &path ) override;
	void visit( const Glyph &glyph ) override;
	void visit( const Line &line ) override;
	void visit( const ArrowTip &arrow ) override;
	void visit( const Image &image ) override;
	void visit( const Group &group ) override;

	static std::string color_attributes( const Color &c );
	static std::string stroke_attributes( float thickness, const Color &c );

private:
	void emit_path_data( const std::vector<PathCommand> &commands, std::ostream &out ) const;

	std::string indent() const;
	bool open_g( const Visual &v, bool force_g );
	void close_g();

	std::ostream &_out;
	int _indent;
	Color _current_color = { 1.F, 1.F, 1.F, 1.F };
	Color _current_stroke_color = { 1.F, 1.F, 1.F, 1.F };
	std::map<std::pair<const FontFace *, unsigned int>, std::vector<PathCommand>> _glyphs;
	std::vector<bool> _open_g_count;
};
