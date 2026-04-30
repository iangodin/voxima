#include <iostream>
#include <vector>
#include <string_view>
#include <memory>
#include <format>
#include <stdexcept>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_FONT_FORMATS_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H
#include FT_TRUETYPE_IDS_H
#include FT_SFNT_NAMES_H
#include FT_MULTIPLE_MASTERS_H

// RAII Deleter for FreeType resources
struct FT_Deleter {
    void operator()(FT_Library lib) const { FT_Done_FreeType(lib); }
    void operator()(FT_Face face) const { FT_Done_Face(face); }
};

using UniqueLibrary = std::unique_ptr<FT_LibraryRec_, FT_Deleter>;
using UniqueFace = std::unique_ptr<FT_FaceRec_, FT_Deleter>;

class FontInspector {
public:
    explicit FontInspector(std::string_view path) : font_path(path) {
        FT_Library lib_raw;
        if (FT_Init_FreeType(&lib_raw)) {
            throw std::runtime_error("Failed to initialize FreeType library.");
        }
        library.reset(lib_raw);
    }

    void inspect_all_faces() {
        FT_Face face_raw;
        if (FT_New_Face(library.get(), font_path.data(), -1, &face_raw)) {
            std::cerr << std::format("Error: Could not open font file '{}'\n", font_path);
            return;
        }
        int num_faces = face_raw->num_faces;
        FT_Done_Face(face_raw);

        for (int i = 0; i < num_faces; ++i) {
            inspect_face(i);
        }
    }

private:
    std::string_view font_path;
    UniqueLibrary library;

    std::string get_font_version(FT_Face face) const {
        FT_SfntName name;
        FT_UInt count = FT_Get_Sfnt_Name_Count(face);

        for (FT_UInt i = 0; i < count; ++i) {
            if (FT_Get_Sfnt_Name(face, i, &name) == 0) {
                // Name ID 5 is the Version string
                if (name.name_id == 5) {
                    std::string result;
                    // Basic check: If it's UTF-16BE (common for Windows platform),
                    // every second byte is often 0 for ASCII range.
                    // This simple loop extracts the printable characters.
                    for (FT_UInt j = 0; j < name.string_len; ++j) {
                        char c = static_cast<char>(name.string[j]);
                        if (c >= 32 && c <= 126) result += c;
                    }
                    return result.empty() ? "Unknown Format" : result;
                }
            }
        }
        return "Not Provided";
    }

    void inspect_face(int index) {
        FT_Face face_ptr;
        if (FT_New_Face(library.get(), font_path.data(), index, &face_ptr)) return;
        UniqueFace face(face_ptr);

        std::cout << std::format("--- Face {} ---\n", index);
        std::cout << std::format("  Family:           {}\n", face->family_name ? face->family_name : "Unknown");
        std::cout << std::format("  Style:            {}\n", face->style_name ? face->style_name : "Regular");
        std::cout << std::format("  Version:          {}\n", get_font_version(face.get()));

        if (auto ps_name = FT_Get_Postscript_Name(face.get())) {
            std::cout << std::format("  PostScript Name:  {}\n", ps_name);
        }

        std::cout << std::format("  Glyph Count:      {}\n", face->num_glyphs);

        report_variable_axes(face.get());
        report_features(face.get());
        std::cout << "\n";
    }

    std::string tag_to_string(FT_ULong tag) const {
        char buf[5];
        buf[0] = static_cast<char>((tag >> 24) & 0xFF);
        buf[1] = static_cast<char>((tag >> 16) & 0xFF);
        buf[2] = static_cast<char>((tag >> 8) & 0xFF);
        buf[3] = static_cast<char>(tag & 0xFF);
        buf[4] = '\0';
        return std::string(buf);
    }

    std::string translate_tag(FT_ULong tag) const {
        if (tag == FT_MAKE_TAG('w','g','h','t')) return "Weight";
        if (tag == FT_MAKE_TAG('w','d','t','h')) return "Width";
        if (tag == FT_MAKE_TAG('s','l','n','t')) return "Slant";
        if (tag == FT_MAKE_TAG('i','t','a','l')) return "Italic";
        if (tag == FT_MAKE_TAG('o','p','s','z')) return "Optical Size";
        if (tag == FT_MAKE_TAG('G','R','A','D')) return "Grade";
        if (tag == FT_MAKE_TAG('O','U','T','F')) return "Outline";
        if (tag == FT_MAKE_TAG('F','I','L','L')) return "Fill";
        return "Unknown";
    }

    void report_variable_axes(FT_Face face) const {
        std::cout << "  Variable Axes:    ";
        if (!FT_HAS_MULTIPLE_MASTERS(face)) {
            std::cout << "None\n";
            return;
        }

        FT_MM_Var* mm_var = nullptr;
        if (FT_Get_MM_Var(face, &mm_var) != 0) {
            std::cout << "Failed to query axes\n";
            return;
        }

        std::cout << "\n";
        for (FT_UInt i = 0; i < mm_var->num_axis; ++i) {
            auto& axis = mm_var->axis[i];
            std::string tag = tag_to_string(axis.tag);
            std::string name = translate_tag(axis.tag);
            
            float min = static_cast<float>(axis.minimum) / 65536.0f;
            float max = static_cast<float>(axis.maximum) / 65536.0f;
            float def = static_cast<float>(axis.def) / 65536.0f;

            std::cout << std::format("    - [{}] {:<12} (min: {:>6.1f}, max: {:>6.1f}, default: {:>6.1f})\n", 
                tag, name, min, max, def);
        }

        FT_Done_MM_Var(face->glyph->library, mm_var);
    }

    void report_features(FT_Face face) const {
        const char* format = FT_Get_Font_Format(face);
        std::cout << std::format("  Font Format:      {}\n", format ? format : "Unknown");

        FT_ULong g_len = 0;
        bool has_gsub = (FT_Load_Sfnt_Table(face, FT_MAKE_TAG('G','S','U','B'), 0, nullptr, &g_len) == 0);

        g_len = 0;
        bool has_gpos = (FT_Load_Sfnt_Table(face, FT_MAKE_TAG('G','P','O','S'), 0, nullptr, &g_len) == 0);

        std::cout << std::format("  Layout Features:  {}\n",
            (has_gsub || has_gpos) ? "Advanced (Ligatures/Small Caps supported)" : "Basic Only");

        FT_ULong m_len = 0;
        bool has_math = (FT_Load_Sfnt_Table(face, TTAG_MATH, 0, nullptr, &m_len) == 0);
        std::cout << std::format("  OpenMath Table:   {}\n", has_math ? "Found" : "Not Found");

        auto* os2 = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(face, FT_SFNT_OS2));

        std::vector<std::string_view> scripts;
        bool has_math_chars = false;

        if (os2) {
            if (os2->ulUnicodeRange1 & 0x01)       scripts.push_back("Latin");
            if (os2->ulUnicodeRange1 & 0x01000000) scripts.push_back("Greek");
            if (os2->ulUnicodeRange1 & 0x04000000) scripts.push_back("Cyrillic");
            if (os2->ulUnicodeRange1 & 0x10000000) scripts.push_back("Hebrew");
            if (os2->ulUnicodeRange1 & 0x20000000) scripts.push_back("Arabic");
            if (os2->ulUnicodeRange2 & 0x00000010) scripts.push_back("Devanagari");
            if (os2->ulUnicodeRange2 & 0x20000000) scripts.push_back("CJK");
            if (os2->ulUnicodeRange2 & 0x00000180) scripts.push_back("Japanese (Kana)");

            if ((os2->ulUnicodeRange1 & TT_UCR_MATHEMATICAL_OPERATORS) ||
                (os2->ulUnicodeRange2 & TT_UCR_MATH_ALPHANUMERIC_SYMBOLS)) {
                has_math_chars = true;
            }
        }

        std::cout << std::format("  Math Symbols:     {}\n", has_math_chars ? "Detected" : "None");

        std::cout << "  Primary Scripts:  ";
        if (scripts.empty()) {
            std::cout << "None detected";
        } else {
            for (size_t i = 0; i < scripts.size(); ++i) {
                std::cout << scripts[i] << (i == scripts.size() - 1 ? "" : ", ");
            }
        }
        std::cout << "\n";
        std::cout << std::format("  Fixed Width:      {}\n", FT_IS_FIXED_WIDTH(face) ? "Yes" : "No");
    }
};

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: list_font <font-file>\n";
        return 1;
    }

    try {
        FontInspector inspector(argv[1]);
        inspector.inspect_all_faces();
    } catch (const std::exception& e) {
        std::cerr << std::format("Error: {}\n", e.what());
        return 1;
    }
    return 0;
}
