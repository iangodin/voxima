#include <iostream>
#include <vector>
#include <string_view>
#include <memory>
#include <format>
#include <stdexcept>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H   // For TTAG_MATH
#include FT_TRUETYPE_IDS_H    // For TT_UCR (Unicode Character Range) macros

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
        // Passing -1 to FT_New_Face queries the total number of faces in the file
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

    void inspect_face(int index) {
        FT_Face face_ptr;
        if (FT_New_Face(library.get(), font_path.data(), index, &face_ptr)) return;
        UniqueFace face(face_ptr);

        std::cout << std::format("--- Face {} ---\n", index);
        std::cout << std::format("  Family:           {}\n", face->family_name ? face->family_name : "Unknown");
        std::cout << std::format("  Style:            {}\n", face->style_name ? face->style_name : "Regular");

        if (auto ps_name = FT_Get_Postscript_Name(face.get())) {
            std::cout << std::format("  PostScript Name:  {}\n", ps_name);
        }

        std::cout << std::format("  Glyph Count:      {}\n", face->num_glyphs);
        
        report_features(face.get());
        std::cout << "\n";
    }

    void report_features(FT_Face face) const {
        // 1. Structural OpenMath Check ('MATH' table)
        FT_ULong length = 0;
        FT_Error err = FT_Load_Sfnt_Table(face, TTAG_MATH, 0, nullptr, &length);
        bool has_math_table = (err == 0 && length > 0);
        
        // 2. Language and Math Range Check (OS/2 table)
        auto* os2 = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(face, FT_SFNT_OS2));
        
        std::vector<std::string_view> scripts;
        bool has_math_chars = false;

        if (os2) {
            // General Language Groups
            if (os2->ulUnicodeRange1 & 0x01)       scripts.push_back("Latin");
            if (os2->ulUnicodeRange1 & 0x01000000) scripts.push_back("Greek");
            if (os2->ulUnicodeRange1 & 0x04000000) scripts.push_back("Cyrillic");
            if (os2->ulUnicodeRange1 & 0x10000000) scripts.push_back("Hebrew");
            if (os2->ulUnicodeRange1 & 0x20000000) scripts.push_back("Arabic");
            if (os2->ulUnicodeRange2 & 0x20000000) scripts.push_back("CJK (Unified)");
            if (os2->ulUnicodeRange3 & 0x00000180) scripts.push_back("Japanese (Kana)");

            // Math Character Coverage
            if ((os2->ulUnicodeRange1 & TT_UCR_MATHEMATICAL_OPERATORS) || 
                (os2->ulUnicodeRange2 & TT_UCR_MATH_ALPHANUMERIC_SYMBOLS)) {
                has_math_chars = true;
            }
        }

        std::cout << std::format("  OpenMath Table:   {}\n", has_math_table ? "Found" : "Not Found");
        std::cout << std::format("  Math Symbols:     {}\n", has_math_chars ? "Detected in Unicode ranges" : "None");
        
        std::cout << "  Primary Scripts:  ";
        if (scripts.empty()) {
            std::cout << "Miscellaneous/Symbolic";
        } else {
            for (size_t i = 0; i < scripts.size(); ++i) {
                std::cout << scripts[i] << (i == scripts.size() - 1 ? "" : ", ");
            }
        }
        std::cout << "\n";

        // Extra info: check if font is Monospace
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
