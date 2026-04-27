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
#include "Point.h"
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

    void visit( const Group &group ) override {
        for ( const auto &v : group ) {
            v->accept( *this );
        }
    }
};
