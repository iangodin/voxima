#pragma once

#include <string>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <lunasvg/lunasvg.h>

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

    void encode( const lunasvg::Bitmap &bitmap )
    {
        if ( bitmap.isNull() )
            return;

        std::stringstream filename;
        filename << m_prefix << std::setw( m_digits ) << std::setfill( '0' ) << m_frame_count << m_suffix;

        bitmap.writeToPng( filename.str().c_str() );
        m_frame_count++;
    }

    void close()
    {
    }

private:
    std::string m_prefix;
    std::string m_suffix;
    int m_digits;
    size_t m_frame_count;
};
