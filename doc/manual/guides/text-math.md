# Text and math

Text is laid out with real shaping (HarfBuzz) and bidirectional reordering, so it works for any script the fonts
have: Latin, Cyrillic, Greek, Arabic, Hebrew, Chinese, Japanese and more, mixed in one line if you like. The
result is vector outlines like any other visual: you can move, scale, fade or morph it.

## One piece of text

```cpp
auto &title = stage.visuals.add( make_text( theme, "Hello, Voxima", FontStyle{ Intent::TITLE }.large(), "en" ) );
move_to( title, { 0.F, 3.F }, Anchor::TOP );
```

`make_text( theme, string, style, language )`. The text's **baseline is at y = 0 and it is not centered**: its
origin is the left end of the first baseline. Use `center_group( *text )` to put the middle of it on the origin,
or `move_to` to place it.

Give the `language` (a BCP-47 tag like `"fr"` or `"zh-Hans"`) so the shaper picks the right glyph forms.
When the text came from the script, use the script's language. Line breaks in the string make several lines
(`TextLayout` sets the `align` and `line_spacing`).

**Paragraphs** wrap to a width, at spaces:

```cpp
auto &para = stage.visuals.add( make_paragraph( theme, long_text, FontStyle{ Intent::PROSE }, 10.F, lang,
	TextLayout{ .align = TextAlign::CENTER } ) );
```

(Only for languages that put spaces between words.)

## Font styles

A `FontStyle` says what the text is **for** (its *intent*) and how it should look; the **theme** decides which
actual font that is. So a scene works with any theme.

```cpp
FontStyle{ Intent::TITLE }                    // what it is for
FontStyle{ Intent::PROSE }.large().bold()     // chained changes
FontStyle{ Intent::CODE }.small()
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
StyledText text( "Hello\nالعالم" );
text.set_font_style( 0, text.size(), FontStyle{ Intent::TITLE }.large() );
text.set_color( 0, 5, theme.palette().red );
text.set_weight( 2, 4, Weight::BLACK );
auto &label = stage.visuals.add( make_styled_text( theme, text ) );
center_group( label );
```

Ranges are `( start, end )` over the text; there are setters for color, weight, slant, scale, width, kind, intent
and language. Words come out as groups tagged `word0`, `word1`, ... (that is how [word effects](narration.md#word-effects) work).

## Formulas

Formulas are built from small functions that describe them, and laid out only when you `build` them, each part
in its own size: numerators, subscripts and radicands come out right on their own.

```cpp
using namespace math;
using namespace math::literals;

Context context( theme );          // the theme and the body size; large by default

auto formula = eq( "a"_m * "a"_m + "b"_m * "b"_m, "c"_m * "c"_m );     // aa + bb = cc
auto &shown = stage.visuals.add( formula.build( context ) );
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
auto quadratic = row( {
	ord( "x" ), rel( "=" ),
	ord( fraction(
		row( { ord( "−b" ), bin( sym::pm ), ord( radical( row( { ord( superscript( text( "b" ), text( "2" ) ) ),
		                                                          bin( "−" ), ord( "4ac" ) } ) ) ) } ),
		text( "2a" ) ) ) } );
```

**Marking parts:** `colored( color, part )`, `cancel( part )` (a strike through, for terms that cancel),
`boxed( part )`, and `tag( "name", part )` to find a part again after the build (to animate it):

```cpp
Visual *half = find_tag( *shown, "half" );
```

Greek letters set by `text()` are italic, as in TeX; capitals are upright (`upright( sym::Gamma )`). See `math/Symbols.h` for
the names. `words( "if" )` is for words between formulas.

## Code listings

```cpp
auto listing = make_code( theme, code::Language::CPP, R"cpp(
int main() { return 0; }
)cpp" );
center_group( *listing );
auto &code = stage.visuals.add( std::move( listing ) );
```

C and C++ are highlighted with the theme's code colors. Two helpers animate a listing:

```cpp
Typewriter typer( code );
co_await typer.type( stage );                       // typed out, as a person would (or type( stage, 4s ): fitted to 4 seconds)

LineHighlight pointer( code, theme );
co_await pointer.to( stage, Lines( 5 ), 0.4s );     // a line; Lines( 9, 12 ) a range;
co_await pointer.to( stage, Lines{ { 7, 8 }, { 13, 13 } }, 0.5s );   // or several places
co_await pointer.to( stage, Lines(), 0.4s );        // and away again
```

Lines are numbered from 1, as an editor numbers them. Lines outside the listing are ignored.
