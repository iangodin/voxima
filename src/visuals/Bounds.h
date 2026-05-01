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
#include "visuals/Point.h"
#include <algorithm>
#include <limits>

struct AABB {
    Point min = { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };
    Point max = { -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity() };

    void add( Point p ) {
        min.x = std::min( min.x, p.x );
        min.y = std::min( min.y, p.y );
        max.x = std::max( max.x, p.x );
        max.y = std::max( max.y, p.y );
    }
};

class BoundingBoxVisitor : public VisualVisitor {
public:
    AABB bounds;

    void visit( const Circle &circle ) override {
        bounds.add( { circle.position.x - circle.radius, circle.position.y - circle.radius } );
        bounds.add( { circle.position.x + circle.radius, circle.position.y + circle.radius } );
    }

    void visit( const Rectangle &rect ) override {
        bounds.add( { rect.position.x - rect.half_width, rect.position.y - rect.half_height } );
        bounds.add( { rect.position.x + rect.half_width, rect.position.y + rect.half_height } );
    }

    void visit( const RoundRectangle &rect ) override {
        bounds.add( { rect.position.x - rect.half_width, rect.position.y - rect.half_height } );
        bounds.add( { rect.position.x + rect.half_width, rect.position.y + rect.half_height } );
    }

    void visit( const Ellipse &ellipse ) override {
        bounds.add( { ellipse.position.x - ellipse.half_width, ellipse.position.y - ellipse.half_height } );
        bounds.add( { ellipse.position.x + ellipse.half_width, ellipse.position.y + ellipse.half_height } );
    }

    void visit( const Path &path ) override {
        for ( const auto &cmd : path.commands ) {
            bounds.add( { path.position.x + cmd.to.x, path.position.y + cmd.to.y } );
            if ( cmd.type == PathCommand::QuadTo || cmd.type == PathCommand::CubicTo )
                bounds.add( { path.position.x + cmd.ctrl1.x, path.position.y + cmd.ctrl1.y } );
            if ( cmd.type == PathCommand::CubicTo )
                bounds.add( { path.position.x + cmd.ctrl2.x, path.position.y + cmd.ctrl2.y } );
        }
    }

    void visit( const Line &line ) override {
        bounds.add( { line.position.x + line.start.x, line.position.y + line.start.y } );
        bounds.add( { line.position.x + line.end.x, line.position.y + line.end.y } );
    }

    void visit( const ArrowTip &arrow ) override {
        bounds.add( { arrow.position.x + arrow.end.x, arrow.position.y + arrow.end.y } );
        float s = arrow.size * 2.0f;
        bounds.add( { arrow.position.x + arrow.end.x - s, arrow.position.y + arrow.end.y - s } );
        bounds.add( { arrow.position.x + arrow.end.x + s, arrow.position.y + arrow.end.y + s } );
    }

    void visit( const Image &image ) override {
        bounds.add( { image.position.x - image.half_width, image.position.y - image.half_height } );
        bounds.add( { image.position.x + image.half_width, image.position.y + image.half_height } );
    }

    void visit( const Glyph &glyph ) override {
        for ( const auto &cmd : glyph.commands ) {
            bounds.add( { ( glyph.position.x + cmd.to.x ) * glyph.scale_x, ( glyph.position.y + cmd.to.y ) * glyph.scale_y } );
            if ( cmd.type == PathCommand::QuadTo || cmd.type == PathCommand::CubicTo )
                bounds.add( { ( glyph.position.x + cmd.ctrl1.x ) * glyph.scale_x, ( glyph.position.y + cmd.ctrl1.y ) * glyph.scale_y } );
            if ( cmd.type == PathCommand::CubicTo )
                bounds.add( { ( glyph.position.x + cmd.ctrl2.x ) * glyph.scale_x, ( glyph.position.y + cmd.ctrl2.y ) * glyph.scale_y } );
        }
    }

    void visit( const Group &group ) override {
        for ( const auto &v : group ) {
            v->accept( *this );
        }
    }
};
