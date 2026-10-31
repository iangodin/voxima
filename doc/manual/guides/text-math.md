# Text and math

Text is laid out with real shaping (HarfBuzz) and bidirectional reordering, so it works for any script the fonts
have: Latin, Cyrillic, Greek, Arabic, Hebrew, Chinese, Japanese and more, mixed in one line if you like. The
result is vector outlines like any other visual: you can move, scale, fade or morph it.

The code on this page is from the `text-math` example (`examples/text-math`), which is built with Voxima.

## One piece of text

```cpp
--8<-- "examples/text-math/text-math.cpp:title"
```

`make_text( theme, string, style, language )`. The text's **baseline is at y = 0 and it is not centered**: its
origin is the left end of the first baseline. Use `center_group( *text )` to put the middle of it on the origin,
or `move_to` to place it.

Give the `language` (a BCP-47 tag like `"fr"` or `"zh-Hans"`) so the shaper picks the right glyph forms.
When the text came from the script, use the script's language. Line breaks in the string make several lines
(`TextLayout` sets the `align` and `line_spacing`).

**Paragraphs** wrap to a width, at spaces:

```cpp
--8<-- "examples/text-math/text-math.cpp:paragraph"
```

(Only for languages that put spaces between words.)

## Font styles

A `FontStyle` says what the text is **for** (its *intent*) and how it should look; the **theme** decides which
actual font that is. So a scene works with any theme.

```cpp
--8<-- "examples/text-math/text-math.cpp:font-styles"
```

| Intents | |
|---|---|
| `PROSE`, `TITLE`, `LABEL`, `ANNOTATION`, `CAPTION` | Text of different roles |
| `CODE` | Monospace |
| `MATH` | The math font |

Chain: `.tiny() .small() .medium() .large() .huge()` for size; `.light() .regular() .bold() .black()` for weight;
`.upright() .italic() .oblique() .math_italic()` for slant; `.sans() .serif() .slab() .mono() .script() .fraktur()
.double_struck()` for the kind of letter. Text sizes do not vary between themes: a theme is which fonts, not how
big. If a font has no glyph for a character, the next font in the theme's stack is tried.

## Styled text: several looks in one piece

Build a `StyledText` and set styles on ranges of characters:

```cpp
--8<-- "examples/text-math/text-math.cpp:styled-text"
```

Ranges are `( start, end )` over the text; there are setters for color, weight, slant, scale, width, kind, intent
and language. Words come out as groups tagged `word0`, `word1`, ... (that is how [word effects](narration.md#word-effects) work).

## Formulas

Formulas are built from small functions that describe them, and laid out only when you `build` them, each part
in its own size: numerators, subscripts and radicands come out right on their own.

```cpp
--8<-- "examples/text-math/text-math.cpp:formula"
```

A formula is an `Expr`: cheap to copy, and can be built any number of times (`build( context, MathStyle::text() )`
for in-line size).

**Operators** read like the math: `a + b`, `a - b`, `-a`, `a * b` (side by side, no sign; `times( a, b )` for ×),
`a / b` (a fraction), `eq lt gt leq geq neq`, and `"x"_m` (math italic) and `"sin"_u` (upright). The spacing is the
spacing TeX uses.

**Building blocks** (`math/Expr.h`):

| | |
|---|---|
| Text | `text` (math italic), `upright`, `bold`, `words` (for "if", "for all"), `script`, `fraktur`, `double_struck`, `sans`, `mono` |
| Shapes | `fraction`, `binomial`, `radical`, `root`, `superscript`, `subscript`, `scripts`, `primed`, `overset`, `underset`, `substack`, `continued_fraction` |
| Big operators | `sum`, `product`, `integral`, `large_operator`; limits `lim max min sup inf gcd`; functions `sin cos tan log ln exp ...` |
| Fences | `parens`, `brackets`, `braces`, `bars`, `fenced( open, content, close )`, `big` |
| Marks | `overline`, `underline`, `overbrace`, `underbrace`, `hat`, `tilde`, `bar`, `dot`, `vec`, `widehat`, ... |
| Grids | `matrix`, `pmatrix`, `bmatrix`, `vmatrix`, `cases`, `aligned` (equations lined up as TeX's `align`) |
| Rows | `row( { ord( ... ), bin( "+" ), rel( "=" ), ... } )`: atoms of a class, spaced as TeX does |
| Symbols | `sym::alpha`, `sym::leq`, `sym::infty`, `sym::Rightarrow`, ... (Greek, relations, operators, arrows) |

A fuller formula:

```cpp
--8<-- "examples/text-math/text-math.cpp:quadratic"
```

**Marking parts:** `colored( color, part )`, `cancel( part )` (a strike through, for terms that cancel),
`boxed( part )`, and `tag( "name", part )` to find a part again after the build (to animate it):

```cpp
--8<-- "examples/text-math/text-math.cpp:tag"
```

Greek letters set by `text()` are italic, as in TeX; capitals are upright (`upright( sym::Gamma )`). See `math/Symbols.h` for
the names. `words( "if" )` is for words between formulas.

## Code listings

```cpp
--8<-- "examples/text-math/text-math.cpp:listing"
```

C and C++ are highlighted with the theme's code colors. Two helpers animate a listing:

```cpp
--8<-- "examples/text-math/text-math.cpp:typing"
```

Lines are numbered from 1, as an editor numbers them. Lines outside the listing are ignored.
