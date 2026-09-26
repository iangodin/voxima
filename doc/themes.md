

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

Each theme's code colors are its CodePalette (Theme::code_palette), set in its create_theme(); they are all for a
dark background. A scene can change them with theme.set_code_palette().
