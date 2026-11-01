#include "PngRender.h"

#include <lunasvg/lunasvg.h>

#include <iomanip>
#include <sstream>

void PngRender::encode( const lunasvg::Bitmap &bitmap )
{
    if ( bitmap.isNull() )
        return;

    std::stringstream filename;
    filename << m_prefix << std::setw( m_digits ) << std::setfill( '0' ) << m_frame_count << m_suffix;

    bitmap.writeToPng( filename.str().c_str() );
    m_frame_count++;
}
