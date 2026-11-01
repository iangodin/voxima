#pragma once

#include <core/ThemeFile.h>

// The themes most tests set text in: New Computer Modern and Academic Classic, without the CJK fallback (see
// tests/data/themes), so they need only the fonts of download_fonts.py --tests. A test of the built-in themes
// themselves loads them by name, and is tagged [all-fonts].
inline Theme test_theme()
{
	return themes::load( "test", { TEST_DATA_DIR "/themes" } );
}

inline Theme test_academic_theme()
{
	return themes::load( "test-academic", { TEST_DATA_DIR "/themes" } );
}
