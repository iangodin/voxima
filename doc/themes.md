

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
one of its own (on the plain black every video has had), and `--palette name` (or `theme.set_palette( Palette::get( name ) )`
and `stage.use_palette( theme.palette() )` in a scene) changes it, background and text included.

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
