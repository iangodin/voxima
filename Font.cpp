
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "Font.h"
#include "Path.h"
#include "Bounds.h"
#include <fstream>
#include <vector>

struct Font::Impl
{
	std::vector<unsigned char> buffer;
	stbtt_fontinfo font;
};

Font::Font( const std::string &path ) : _impl( std::make_unique<Impl>() )
{
	std::ifstream file( path, std::ios::binary | std::ios::ate );
	if ( !file ) return;

	std::streamsize size = file.tellg();
	file.seekg( 0, std::ios::beg );
	_impl->buffer.resize( size );
	if ( !file.read( (char*)_impl->buffer.data(), size ) ) return;

	if ( !stbtt_InitFont( &_impl->font, _impl->buffer.data(), 0 ) )
	{
		_impl->buffer.clear();
	}
}

Font::~Font() = default;

std::unique_ptr<Visuals> Font::create_text( const std::string &text, float font_size ) const
{
	auto group = std::make_unique<Visuals>();
	if ( _impl->buffer.empty() ) return group;

	float scale = stbtt_ScaleForMappingEmToPixels( &_impl->font, font_size );

	float x_pos = 0.0f;
	for ( char c : text )
	{
		auto path = std::make_unique<Path>();

		stbtt_vertex *vertices;
		int num_verts = stbtt_GetCodepointShape( &_impl->font, c, &vertices );

		for ( int i = 0; i < num_verts; ++i )
		{
			stbtt_vertex v = vertices[i];
			switch ( v.type )
			{
				case STBTT_vmove:
					path->commands.push_back({ PathCommand::MoveTo, { v.x * scale, v.y * scale } });
					break;
				case STBTT_vline:
					path->commands.push_back({ PathCommand::LineTo, { v.x * scale, v.y * scale } });
					break;
				case STBTT_vcurve:
					path->commands.push_back({ PathCommand::QuadTo, { v.x * scale, v.y * scale }, { v.cx * scale, v.cy * scale } });
					break;
				case STBTT_vcubic:
					path->commands.push_back({ PathCommand::CubicTo, { v.x * scale, v.y * scale }, { v.cx * scale, v.cy * scale }, { v.cx1 * scale, v.cy1 * scale } });
					break;
			}
		}
		stbtt_FreeShape( &_impl->font, vertices );

		int advance, lsb;
		stbtt_GetCodepointHMetrics( &_impl->font, c, &advance, &lsb );

		path->position = { x_pos + (lsb * scale), 0.0f };
		group->add( std::move( path ) );

		x_pos += advance * scale;
	}

	BoundingBoxVisitor visitor;
	group->accept( visitor );
	Point center = { ( visitor.bounds.min.x + visitor.bounds.max.x ) / 2.0f,
					 ( visitor.bounds.min.y + visitor.bounds.max.y ) / 2.0f };

	for ( auto &v : *group ) {
		v->position.x -= center.x;
		v->position.y -= center.y;
	}

	return group;
}
