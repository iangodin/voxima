#include "Font.h"
#include "MFLFontFace.h"
#include "Path.h"
#include "Bounds.h"
#include "mfl/layout.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include <hb.h>
#include <hb-ft.h>
#include <vector>
#include <iostream>

Font::Font( const std::string &path ) : _path(path), _face( std::make_unique<MFLFontFace>(path) )
{
}

Font::~Font()
{
}

std::unique_ptr<Group> Font::create_text( const std::string &text, float font_size ) const
{
    auto group = std::make_unique<Group>();

    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8( buffer, text.c_str(), -1, 0, -1 );
    hb_buffer_guess_segment_properties( buffer );
    int upem = _face->get_ft_face()->units_per_EM;
    hb_font_set_scale(_face->get_hb_font(), upem, upem);
    hb_shape( _face->get_hb_font(), buffer, NULL, 0 );

    unsigned int glyph_count;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos( buffer, &glyph_count );
    hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions( buffer, &glyph_count );

    float scale = font_size / static_cast<float>(upem);
    float current_x = 0.0f;
    float current_y = 0.0f;

    for ( unsigned int i = 0; i < glyph_count; ++i )
    {
        hb_codepoint_t glyph_id = glyph_info[i].codepoint;
        hb_glyph_position_t pos = glyph_pos[i];

        auto path = get_glyph_path(glyph_id, scale);
        path->position = Point{ current_x + (static_cast<float>(pos.x_offset) * scale), current_y + (static_cast<float>(pos.y_offset) * scale) };
        group->add( std::move( path ) );

        current_x += static_cast<float>(pos.x_advance) * scale;
        current_y += static_cast<float>(pos.y_advance) * scale;
    }

    hb_buffer_destroy( buffer );
    center_group(group.get());
    return group;
}

std::unique_ptr<Group> Font::create_math( const std::string &formula, float font_size ) const
{
	const float internal_font_size = 100.F;

    auto group = std::make_unique<Group>();

    auto create_font = [this](const mfl::font_family /*family*/) -> std::unique_ptr<mfl::abstract_font_face> {
        return std::make_unique<MFLFontFace>(_path);
    };

    auto layout = mfl::layout(formula, mfl::points{static_cast<double>(internal_font_size)}, create_font);

    if (layout.error) {
        std::cerr << "MFL Error: " << *layout.error << std::endl;
        return group;
    }

    std::cout << "Formula: " << formula << " returned " << layout.glyphs.size() << " glyphs." << std::endl;
    for (const auto& g : layout.glyphs) {
        std::cout << "Glyph index: " << g.index << ", x: " << g.x.value() << ", y: " << g.y.value() << ", size: " << g.size.value() << std::endl;
    }

    int upem = _face->get_ft_face()->units_per_EM;
    //float base_upem = static_cast<float>(_face->get_ft_face()->units_per_EM);
    for (const auto& g : layout.glyphs) {
        // g.size is the size of the font for this specific glyph (e.g. script size)
        // We need to scale the glyph path by this size.
        float glyph_scale = static_cast<float>(g.size.value()) * font_size / ( internal_font_size * static_cast<float>( upem ) );

        auto path = get_glyph_path(static_cast<unsigned int>(g.index), glyph_scale);
        // mfl already provided the absolute position in points.
        path->position = Point{
			static_cast<float>(g.x.value()) * font_size / ( 64.F * static_cast<float>( upem ) ),
			static_cast<float>(g.y.value()) * font_size / ( 64.F * static_cast<float>( upem ) ) };
        group->add(std::move(path));
    }

    center_group(group.get());
    return group;
}

std::unique_ptr<Path> Font::get_glyph_path(unsigned int glyph_id, float scale) const
{
    FT_Load_Glyph( _face->get_ft_face(), glyph_id, FT_LOAD_NO_SCALE );

    auto path = std::make_unique<Path>();
    struct DecomposeContext { Path* path; float scale; };
    DecomposeContext ctx = { path.get(), scale };

    FT_Outline_Funcs funcs = {
        [](const FT_Vector* to, void* user) -> int {
            auto* d_ctx = static_cast<DecomposeContext*>(user);
            d_ctx->path->commands.push_back({ PathCommand::MoveTo, { static_cast<float>(to->x) * d_ctx->scale, static_cast<float>(to->y) * d_ctx->scale } });
            return 0;
        },
        [](const FT_Vector* to, void* user) -> int {
            auto* d_ctx = static_cast<DecomposeContext*>(user);
            d_ctx->path->commands.push_back({ PathCommand::LineTo, { static_cast<float>(to->x) * d_ctx->scale, static_cast<float>(to->y) * d_ctx->scale } });
            return 0;
        },
        [](const FT_Vector* control, const FT_Vector* to, void* user) -> int {
            auto* d_ctx = static_cast<DecomposeContext*>(user);
            d_ctx->path->commands.push_back({ PathCommand::QuadTo, { static_cast<float>(to->x) * d_ctx->scale, static_cast<float>(to->y) * d_ctx->scale }, { static_cast<float>(control->x) * d_ctx->scale, static_cast<float>(control->y) * d_ctx->scale } });
            return 0;
        },
        [](const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user) -> int {
            auto* d_ctx = static_cast<DecomposeContext*>(user);
            d_ctx->path->commands.push_back({ PathCommand::CubicTo, { static_cast<float>(to->x) * d_ctx->scale, static_cast<float>(to->y) * d_ctx->scale }, { static_cast<float>(control1->x) * d_ctx->scale, static_cast<float>(control1->y) * d_ctx->scale }, { static_cast<float>(control2->x) * d_ctx->scale, static_cast<float>(control2->y) * d_ctx->scale } });
            return 0;
        }, 0, 0 };
    FT_Outline_Decompose( &_face->get_ft_face()->glyph->outline, &funcs, &ctx );
    return path;
}

void Font::center_group(Group* group) const
{
    BoundingBoxVisitor visitor;
    group->accept( visitor );
    Point center = { ( visitor.bounds.min.x + visitor.bounds.max.x ) / 2.0f,
                     ( visitor.bounds.min.y + visitor.bounds.max.y ) / 2.0f };

    for ( auto &v : *group ) {
        v->position.x -= center.x;
        v->position.y -= center.y;
    }
    group->position = { 0.0f, 0.0f };
}
