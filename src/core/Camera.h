#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>

struct Camera
{
    int32_t width = 1280;
    int32_t height = 720;
    int64_t frame_rate = 24;

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
