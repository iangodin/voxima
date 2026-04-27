#pragma once

#include "VisualVisitor.h"
#include "Circle.h"
#include "Rectangle.h"
#include "RoundRectangle.h"
#include "Path.h"
#include "Visuals.h"
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

    void visit( const Path &path ) override {
        for ( const auto &cmd : path.commands ) {
            bounds.add( { path.position.x + cmd.to.x, path.position.y + cmd.to.y } );
        }
    }

    void visit( const Visuals &visuals ) override {
        for ( const auto &v : visuals ) {
            v->accept( *this );
        }
    }
};
