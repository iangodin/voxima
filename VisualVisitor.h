
#pragma once

struct Circle;
struct Rectangle;
struct RoundRectangle;
struct Path;
struct Line;
struct ArrowTip;
struct Text;
struct Visuals;

struct VisualVisitor
{
	virtual ~VisualVisitor() = default;
	virtual void visit( const Circle &circle ) = 0;
	virtual void visit( const Rectangle &rectangle ) = 0;
	virtual void visit( const RoundRectangle &rectangle ) = 0;
	virtual void visit( const Path &path ) = 0;
	virtual void visit( const Line &line ) = 0;
	virtual void visit( const ArrowTip &arrow ) = 0;
	virtual void visit( const Visuals &visuals ) = 0;
};
