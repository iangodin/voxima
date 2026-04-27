#include "Font.h"
#include "Path.h"
#include "Bounds.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include <hb.h>
#include <hb-ft.h>
#include <vector>
#include <iostream>

struct Font::Impl
{
    FT_Library ft_library;
    FT_Face ft_face;
    hb_font_t* hb_font;
    std::vector<unsigned char> buffer;
};

Font::Font( const std::string &path ) : _impl( std::make_unique<Impl>() )
{
    if ( FT_Init_FreeType( &_impl->ft_library ) != 0 ) return;
    if ( FT_New_Face( _impl->ft_library, path.c_str(), 0, &_impl->ft_face ) != 0 ) return;

    _impl->hb_font = hb_ft_font_create( _impl->ft_face, NULL );
}

Font::~Font()
{
    hb_font_destroy( _impl->hb_font );
    FT_Done_Face( _impl->ft_face );
    FT_Done_FreeType( _impl->ft_library );
}

std::unique_ptr<Group> Font::create_text( const std::string &text, float font_size ) const
{
    auto group = std::make_unique<Group>();

    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8( buffer, text.c_str(), -1, 0, -1 );
    hb_buffer_guess_segment_properties( buffer );
    int upem = _impl->ft_face->units_per_EM;
    hb_font_set_scale(_impl->hb_font, upem, upem);
    hb_shape( _impl->hb_font, buffer, NULL, 0 );

    unsigned int glyph_count;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos( buffer, &glyph_count );
    hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions( buffer, &glyph_count );

    float units_per_em = static_cast<float>(_impl->ft_face->units_per_EM);
    float scale = font_size / units_per_em;
    float current_x = 0.0f;
    float current_y = 0.0f;

    struct DecomposeContext { Path* path; float scale; };

    for ( unsigned int i = 0; i < glyph_count; ++i )
    {
        hb_codepoint_t glyph_id = glyph_info[i].codepoint;
        hb_glyph_position_t pos = glyph_pos[i];

        FT_Load_Glyph( _impl->ft_face, glyph_id, FT_LOAD_DEFAULT );
        
        auto path = std::make_unique<Path>();
        DecomposeContext ctx = { path.get(), scale };

        FT_Outline_Funcs funcs = { 
            [](const FT_Vector* to, void* user) -> int {
                auto* ctx = static_cast<DecomposeContext*>(user);
                ctx->path->commands.push_back({ PathCommand::MoveTo, { (float)to->x * ctx->scale, (float)to->y * ctx->scale } });
                return 0;
            },
            [](const FT_Vector* to, void* user) -> int {
                auto* ctx = static_cast<DecomposeContext*>(user);
                ctx->path->commands.push_back({ PathCommand::LineTo, { (float)to->x * ctx->scale, (float)to->y * ctx->scale } });
                return 0;
            },
            [](const FT_Vector* control, const FT_Vector* to, void* user) -> int {
                auto* ctx = static_cast<DecomposeContext*>(user);
                ctx->path->commands.push_back({ PathCommand::QuadTo, { (float)to->x * ctx->scale, (float)to->y * ctx->scale }, { (float)control->x * ctx->scale, (float)control->y * ctx->scale } });
                return 0;
            },
            [](const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user) -> int {
                auto* ctx = static_cast<DecomposeContext*>(user);
                ctx->path->commands.push_back({ PathCommand::CubicTo, { (float)to->x * ctx->scale, (float)to->y * ctx->scale }, { (float)control1->x * ctx->scale, (float)control1->y * ctx->scale }, { (float)control2->x * ctx->scale, (float)control2->y * ctx->scale } });
                return 0;
            }, 0, 0 };
        FT_Outline_Decompose( &_impl->ft_face->glyph->outline, &funcs, &ctx );

        path->position = Point{ current_x + (pos.x_offset * scale), current_y + (pos.y_offset * scale) };
        group->add( std::move( path ) );

        current_x += pos.x_advance * scale;
        current_y += pos.y_advance * scale;
    }

    hb_buffer_destroy( buffer );
// Center the glyphs by offsetting each path
BoundingBoxVisitor visitor;
group->accept( visitor );
Point center = { ( visitor.bounds.min.x + visitor.bounds.max.x ) / 2.0f, 
                 ( visitor.bounds.min.y + visitor.bounds.max.y ) / 2.0f };

for ( auto &v : *group ) {
    v->position.x -= center.x;
    v->position.y -= center.y;
}

group->position = { 0.0f, 0.0f };

return group;
}
