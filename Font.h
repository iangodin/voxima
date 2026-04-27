#pragma once

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "Visuals.h"
#include "Path.h"
#include <string>
#include <vector>
#include <fstream>
#include <memory>

class Font
{
public:
	Font( const std::string &path )
	{
		std::ifstream file( path, std::ios::binary | std::ios::ate );
		if ( !file ) return;

		std::streamsize size = file.tellg();
		file.seekg( 0, std::ios::beg );
		_buffer.resize( size );
		if ( !file.read( (char*)_buffer.data(), size ) ) return;

		if ( !stbtt_InitFont( &_font, _buffer.data(), 0 ) )
		{
			_buffer.clear();
		}
	}

	std::unique_ptr<Visuals> create_text( const std::string &text, float font_size ) const
	{
		auto group = std::make_unique<Visuals>();
		if ( _buffer.empty() ) return group;

		// We'll scale relative to the EM box
		float scale = stbtt_ScaleForMappingEmToPixels( &_font, font_size );
		
		float x_pos = 0.0f;
		for ( char c : text )
		{
			auto path = std::make_unique<Path>();
			
			stbtt_vertex *vertices;
			int num_verts = stbtt_GetCodepointShape( &_font, c, &vertices );
			
			for ( int i = 0; i < num_verts; ++i )
			{
				stbtt_vertex v = vertices[i];
				// Font space has Y-up, but STB often uses Y-down internally depending on the font.
				// We map to our Y-up system.
				switch ( v.type )
				{
					case STBTT_vmove:
						path->commands.push_back({ PathCommand::MoveTo, { v.x * scale, -v.y * scale } });
						break;
					case STBTT_vline:
						path->commands.push_back({ PathCommand::LineTo, { v.x * scale, -v.y * scale } });
						break;
					case STBTT_vcurve:
						path->commands.push_back({ PathCommand::QuadTo, { v.x * scale, -v.y * scale }, { v.cx * scale, -v.cy * scale } });
						break;
					case STBTT_vcubic:
						path->commands.push_back({ PathCommand::CubicTo, { v.x * scale, -v.y * scale }, { v.cx * scale, -v.cy * scale }, { v.cx1 * scale, -v.cy1 * scale } });
						break;
				}
			}
			stbtt_FreeShape( &_font, vertices );

			int advance, lsb;
			stbtt_GetCodepointHMetrics( &_font, c, &advance, &lsb );
			
			path->position = { x_pos + (lsb * scale), 0.0f };
			group->add( std::move( path ) );

			x_pos += advance * scale;
		}

		return group;
	}

private:
	std::vector<unsigned char> _buffer;
	stbtt_fontinfo _font;
};
