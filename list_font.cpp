// list_faces.c
#include <stdio.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s font-file\n", argv[0]);
        return 1;
    }

    const char *path = argv[1];
    FT_Library library;
    if (FT_Init_FreeType(&library)) {
        fprintf(stderr, "FT_Init_FreeType failed\n");
        return 1;
    }

    FT_Face face;
    /* Passing face_index = 0 to FT_New_Face; we will iterate using FT_Open_Face in a loop. */
    int face_index = 0;
    while (1) {
        int err = FT_New_Face(library, path, face_index, &face);
        if (err == FT_Err_Unknown_File_Format) {
            fprintf(stderr, "Unknown/unsupported file format for face %d\n", face_index);
            break;
        } else if (err) {
            /* No more faces or other error */
            break;
        }

        /* Print basic info */
        printf("Face %d:\n", face_index);
        if (face->family_name)
            printf("  Family: %s\n", face->family_name);
        if (face->style_name)
            printf("  Style:  %s\n", face->style_name);

        /* If it's a TTC and has a PostScript name table, try to print PS name */
        const char *psname = FT_Get_Postscript_Name(face);
        if (psname)
            printf("  PostScript name: %s\n", psname);

        printf("  # Glyphs: %ld\n", face->num_glyphs);

        FT_Done_Face(face);
        face_index++;
    }

    FT_Done_FreeType(library);
    return 0;
}

