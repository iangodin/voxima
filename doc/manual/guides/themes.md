# Themes

A **theme** is the look of a video: which fonts text is set in, and the colors it starts with. Choose a built-in
one with `--theme "Academic Classic"` (names below), or write your own (see [Theme files](#theme-files)).
Without `--theme`, a `theme.huml` next to the scripts is used, or else Modern Technical.
Colors can be changed apart from the fonts with `--palette`, so any palette goes with any theme.

## Built-in themes

# Modern Technical

This is a clean, highly legible theme designed for explaining software engineering or modern physics. It pairs well with the crispness of CJK Gothic fonts.

  - Main Font: FiraGO (High x-height, humanist, excellent legibility).
  - Math Font: NewCM Math (The standard for modern, clean LaTeX-style rendering).
  - Emoji Font: Noto Emoji (Variable) (Blobs/Outlines that match Fira’s stroke weight).
  - CJK Fallback: Noto Sans CJK (Clean, modern, and weight-accurate).
  - Monospace Fallback: Fira Mono.
  - Code: Fira Mono, in the colors of GitHub's dark theme (Fira Mono has no italic, so comments are only a color).

# Academic Classic

This theme mimics the look of a prestigious textbook or a formal research paper. It’s perfect for pure mathematics or historical topics.

  - Main Font: New Computer Modern (Book/Regular) (The gold standard for academic "TeX" looks).
  - Math Font: NewCM Math (Native pairing with the text font).
  - Emoji Font: Symbola (More literal, detailed illustrations that fit a "printed" look).
  - CJK Fallback: Source Han Serif (A "Mincho" style that matches the serifs of NewCM).
  - Monospace Fallback: New Computer Modern Mono.
  - Code: New Computer Modern Mono, like a printed textbook's listing: warm, muted inks, bold keywords, italic comments.

# Engineering Blueprint

A high-contrast, technical theme that feels like an architectural drawing or an IDE. It emphasizes structure and geometry.

  - Main Font: IBM Plex Sans (Engineered, rigid, but human-friendly).
  - Math Font: STIX Two Math (A sharp, technical math font often used in scientific publishing).
  - Emoji Font: Noto Emoji (Variable) (Keep it minimal to avoid distracting from the technical data).
  - CJK Fallback: IBM Plex Sans CJK (Specifically designed to sit alongside Plex Latin).
  - Monospace Fallback: IBM Plex Mono.
  - Code: IBM Plex Mono, like a blueprint: cyan and white, amber for literals, bold function names, italic comments.

# Playful Discovery

A softer, more approachable theme for educational content aimed at a general audience or younger students.

  - Main Font: Comfortaa or Quicksand (Rounded, friendly, and very "bouncy" for animations).
  - Math Font: GFS Neohellenic Math (A rounded, sans-serif math font that is rare but very friendly).
  - Emoji Font: Noto Emoji (Variable) (Perfect for this "soft" aesthetic).
  - CJK Fallback: Kosugi Maru (A Japanese rounded Gothic that matches the main font's soft terminals).
  - Code: DejaVu Sans Mono, in soft pastels (the colors of Catppuccin Mocha), italic comments.

# New Computer Modern

  - Code: New Computer Modern Mono, in the colors of One Dark (CodePalette::dark), italic comments.

# Palettes

A theme is fonts; the colors are a `Palette`, chosen apart from it: any palette works with any theme. Each theme starts with
one of its own, whole - background and text included - and `--palette name` (or `theme.set_palette( Palette::get( name ) )`
and `stage.use_palette( theme.palette() )` in a scene) changes it to another. A theme with no palette of its own starts on
plain black (`Palette::house( "one-dark" )`, `Theme`'s own default) instead.

A palette names roles, not uses: `background`, `text`, `muted` (axes, grids, comments), and eight accents (`red orange yellow
green cyan blue purple pink`). Code colors (`palette.code`), a plot's curve colors (the accents in turn: blue, orange, green ...)
and the band behind pointed-out lines all come from them, so they suit the background whichever it is.

  - Dark: one-dark, github-dark, catppuccin-mocha, academic, blueprint (the themes' own, in that order of theme:
    New Computer Modern, Modern Technical, Playful Discovery, Academic Classic, Engineering Blueprint).
  - Light: github-light, catppuccin-latte, solarized-light, paper.
  - Your own: `Palette::add( palette )`, then by name.

Text, axes and lines are drawn in the palette's text color where nothing else was chosen: **pure white means "the theme's text
color"** (`Color::resolved`, `Stage::foreground`), so a scene written for a dark background reads on a light one. Anything given
a color of its own keeps it: use the palette's accents for those (`stage.theme->palette().blue`), not fixed RGB, if the scene
should work on both.

# Theme files

A theme is a HUML file (`themes/*.huml` for the built-in ones, which are compiled in): the fonts as named **families** put in
**stacks**, and the palette it starts with. Your own can start from a built-in one, or from another file, and change only
what it likes:

```huml
meta::
  name: "My Course"
  inherit: "Modern Technical"     # a built-in theme, or <name>.huml in a --theme-dir (or next to the scripts)

palette: "github-light"           # the colors it starts with, whole (see Palettes; --palette changes it)

fonts::                           # families: what a style becomes a file through
  plex_mono::
    regular: "IBMPlexMono-Regular.ttf"
    italic: "IBMPlexMono-Italic.ttf"
    bold: "IBMPlexMono-Bold.ttf"

stacks::                          # lists of families: what the first has no glyph for, the next may
  code:: "plex_mono", "...fallbacks"        # "...name" puts in another stack's families

intents::                         # which stack each kind of text uses
  code: "code"
```

Use it with `--theme my-course.huml` (or `--theme my-course` with `--theme-dir` pointing at it), or call it `theme.huml` in the
script directory and it is used when no `--theme` is given.

**Families** are one of four forms (see `FontFamily.h`):

  - files, by weight and slant: `light regular bold black` and their `_italic` (`italic` is regular's); an optional `small::`
    with the same keys for the tiny and small sizes (an optical size drawn for small text). A style with no file is the nearest
    that has one: the same slant with the weight moved toward regular (black to bold to regular, light to regular), then the
    upright. So a family with only regular and bold serves black, light and any italic.
  - `variable: "file"`: a variable font, its weight, width and slant axes set by the style (or fixed with `weight`, `width`,
    `slant`).
  - `collection: "file"` with `faces:: light: 5, regular: 10, ...`: a font collection, a face for each weight.
  - `notdef: "file"`: the catch-all font that ends a stack, drawing a box for every character.

**Overriding is by merging.** A dict (a family, `fonts`, `intents`) is merged key by key, so `fonts:: fira:: bold: "..."` changes
only that file; a stack (or anything else) given again replaces what it inherits. A mistake in a file (an unknown key, a family
that is not there, a stack that contains itself) is an error that says which file, and what.

The intents are `prose`, `code`, `title`, `label`, `annotation`, `caption` and `math`. Text size does not vary between themes
(`ThemeUtils::get_theme_font_size`): a theme is which files, not how big.

The built-in files are the record of what fonts each theme uses, and a test (`tests/data/font_stacks.txt`, made by
`UPDATE_GOLDEN=1`) keeps them to it: every intent, weight, slant and size loads the same files as before.
