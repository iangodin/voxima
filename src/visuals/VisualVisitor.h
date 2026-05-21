
#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOGDI
#define NOGDI
#endif
#endif

struct Circle;
struct Rectangle;
struct RoundRectangle;
struct Ellipse;
struct Path;
struct Line;
struct ArrowTip;
struct Text;
struct Image;
class Group;
struct Glyph;

struct VisualVisitor
{
	virtual ~VisualVisitor() = default;
	virtual void visit( const Circle &circle ) = 0;
	virtual void visit( const Rectangle &rectangle ) = 0;
	virtual void visit( const RoundRectangle &rectangle ) = 0;
	virtual void visit( const Ellipse &ellipse ) = 0;
	virtual void visit( const Path &path ) = 0;
	virtual void visit( const Line &line ) = 0;
	virtual void visit( const ArrowTip &arrow ) = 0;
	virtual void visit( const Image &image ) = 0;
	virtual void visit( const Group &group ) = 0;
	virtual void visit( const Glyph &glyph ) = 0;
};
