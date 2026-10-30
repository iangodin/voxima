#pragma once

#include <visuals/AABB.h>
#include <visuals/Point.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>

struct Camera
{
    int32_t width = 1280;  // of the picture, in pixels
    int32_t height = 720;
    int64_t frame_rate = 24;

    // What part of the stage is seen: the point in the middle of the picture (in stage units, y up), and how far in it
    // is. At zoom 1 the picture is BASE_VIEW_WIDTH stage units wide; at 2 half that, so things look twice as big
    // (and their lines twice as thick: it is a closer look, not a change to the drawing).
    Point center = { 0.F, 0.F };
    float zoom = 1.F;

    static constexpr float BASE_VIEW_WIDTH = 16.F;

    // The stage units across and up the picture, at this zoom
    float view_width() const
    {
        return BASE_VIEW_WIDTH / zoom;
    }

    float view_height() const
    {
        return view_width() * static_cast<float>( height ) / static_cast<float>( width );
    }

    // The part of the stage that is seen
    AABB visible() const
    {
        AABB box;
        box.add( { center.x() - view_width() / 2.F, center.y() - view_height() / 2.F } );
        box.add( { center.x() + view_width() / 2.F, center.y() + view_height() / 2.F } );
        return box;
    }

    // How many frames a time takes at this frame rate, to the nearest whole frame (nothing takes a negative
    // number). This is how animations are written in seconds and still run for as long whatever --fps is: a
    // time that is not a whole number of frames (half a second at 25 fps) is a frame more or less.
    int64_t frames( std::chrono::duration<double> duration ) const
    {
        return std::max<int64_t>( 0, std::llround( duration.count() * static_cast<double>( frame_rate ) ) );
    }

    // How long a number of frames lasts at this frame rate
    std::chrono::duration<double> duration( int64_t frames ) const
    {
        return std::chrono::duration<double>( static_cast<double>( frames ) / static_cast<double>( frame_rate ) );
    }
};
