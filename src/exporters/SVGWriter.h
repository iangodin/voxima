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
	// `flatness` (in the drawing's units; 0 for none): how close to its true shape a curve must be drawn. The
	// rasteriser (plutovg, FreeType's) only gets a curve to within about an eighth of a pixel, and where it lands
	// depends on how the curve happens to be cut into pieces, so the same shape drawn as a glyph and as a morph's
	// many-piece copy of it differ at the edges, and flicker when one replaces the other. Curves are written cut
	// into as many equal pieces as it takes for each to be within `flatness` of a straight line, which the
	// rasteriser then draws as it is. Stage::save_svg gives a twenty-fifth of a pixel.
	SVGWriter( std::ostream &out, int indent_level = 2, float flatness = 0.F );

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
	void visit( const MathAssembly &assembly ) override;

	static std::string color_attributes( const Color &c );
	static std::string stroke_attributes( float thickness, const Color &c );

private:
	void emit_path_data( const std::vector<PathCommand> &commands, std::ostream &out ) const;

	std::string indent() const;
	bool open_g( const Visual &v, bool force_g );
	void close_g();

	std::ostream &_out;
	int _indent;
	float _flatness = 0.F;
	Color _current_color = { 1.F, 1.F, 1.F, 1.F };
	Color _current_stroke_color = { 1.F, 1.F, 1.F, 1.F };
	// Glyph outlines already written, keyed by the path data itself (which has the font size
	// baked in), so identical outlines share one definition and different ones never do.
	std::map<std::string, size_t> _glyph_ids;
	std::vector<std::string> _glyph_paths;
	std::vector<bool> _open_g_count;
};
