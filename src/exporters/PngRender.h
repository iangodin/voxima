#pragma once

#include <cstddef>
#include <string>

namespace lunasvg
{
class Bitmap;
}

class PngRender
{
public:
    PngRender( const std::string &prefix, const std::string &suffix, int digits = 4 )
        : m_prefix( prefix ), m_suffix( suffix ), m_digits( digits ), m_frame_count( 0 )
    {
    }

    ~PngRender()
    {
    }

    void encode( const lunasvg::Bitmap &bitmap );

    void close()
    {
    }

private:
    std::string m_prefix;
    std::string m_suffix;
    int m_digits;
    size_t m_frame_count;
};
