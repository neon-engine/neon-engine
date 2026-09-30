# User interface

This note records how menus and what is shown during play are made, why they
are made that way, and what is still open.

**Current decision:** a user interface is a YAML file that lists elements.
It is meant to be read and changed by hand. What the file says follows web
standards: the names and the meanings of its properties are those of CSS, so
that what a property does is said by a specification and not by the engine.
Elements are placed by the rules of CSS Flexible Box Layout.

It draws, lays out, reacts to input, and is defined in files. It is not a
complete set of widgets. It is separate from the interface of the editor,
which will use Dear ImGui.

A user interface is shown on a **surface**. The window is one. A screen in
the world is another, and so is what a camera sees. See
[surfaces](#surfaces).

## The file

```yaml
ui: hud
version: 1

values:
  health: 75

root:
  type: panel
  width: 100%
  height: 100%
  children:
    - type: label
      name: health
      text: "Health: {health}"
      position: absolute
      top: 16
      left: 16
      font_size: 24
      color: "#ffffff"

    - type: bar
      value: "{health}"
      max: 100
      position: absolute
      top: 56
      left: 16
      width: 200
      height: 16

    - type: button
      name: start
      text: Start
      position: absolute
      bottom: 32
      right: 32
      hover:
        background_color: "#4c566a"
```

The user interface of the runtime is
[hud.ui.yml](../app/NeonRuntime/assets/ui/hud.ui.yml). It is shown in two
ways:

```
NeonRuntime --ui assets://ui/hud.ui.yml
NeonRuntime --scene assets://scenes/hud-demo.scene.yml
```

The second is a scene that names its user interface, as a component:

```yaml
entities:
  - name: hud
    components:
      Ui:
        file: assets://ui/hud.ui.yml
```

The user interface is shown for as long as the entity is there. A game shows
and hides files itself with `UiContext::Load` and `Unload`.

### How names and values are written

| What | How | Example |
|---|---|---|
| A property | As in CSS, with an underscore where CSS has a hyphen. Scene files write names the same way | `background_color`, `flex_direction` |
| A keyword | As in CSS, hyphen included | `space-between`, `row-reverse`, `border-box` |
| A length | A number, which counts as pixels, or text with `px` or `%` | `200`, `200px`, `50%`, `auto` |
| Several values | Text with spaces between them, or a list | `8 16`, `[8, 16]` |
| A colour | As in CSS, in quotes. **Preferred.** A list as in scene files is read as well | `"#ff8000"`, `"#ff800080"`, `"#f80"`, `rgb(255, 128, 0)`, `rgba(255, 128, 0, 0.5)`, `[1, 0.5, 0]` |

A `#` starts a comment in YAML, so a value with one in it needs quotes as a
whole: `border: "2px solid #4c566a"`.

### At the top

| Name | Holds | When it is left out |
|---|---|---|
| `ui` | What the file calls itself. Events carry it | It has no name |
| `version` | The version of this layout, which is 1 | It counts as 1 |
| `modal` | Whether the file takes the input away from the game and from the files below it. See [input](#input-and-focus) | `false` |
| `reference_size` | `[width, height]` of the frame the file was made for. See [scaling](#scaling) | `[1920, 1080]` |
| `scale` | `fit`, `width`, `height`, or `none` | `fit` |
| `fonts` | A list of fonts, each with `family`, `src`, `weight`, `style`, and `rendering`. It is what `@font-face` is in CSS. See [fonts](#fonts) | The fonts of the application |
| `values` | What the values are until the game sets them | None |
| `root` | The element everything else is inside | None. It has to be written |

### Every element

| Name | Holds | When it is left out |
|---|---|---|
| `type` | The kind of element | None. It has to be written |
| `name` | What the element is called. Events carry it. Given once in a file | It has no name and reports nothing |
| `hidden` | `true`, `false`, or a value such as `"{paused}"` or `"{!paused}"`. It is the attribute `hidden` of HTML | `false` |
| `hover`, `active`, `focus`, `disabled` | Properties that replace those of the element while it is in the state. See [states](#states) | Nothing changes |
| The properties | See [properties](#properties) | Each keeps its default |

### Elements

| `type` | What it is | In HTML | Next to the properties it holds | Holds `children` |
|---|---|---|---|---|
| `panel` | A box, with a background and a border if it has them | `div` | Nothing | Yes |
| `label` | A text | `span` | `text` | No |
| `image` | An image from a file | `img` | `src`, which has to be written | No |
| `button` | What a player chooses. It reports a click | `button` | `text`, `enabled`, `autofocus` | Yes, in place of `text` |
| `bar` | How much of something there is | `progress` | `value`, `max` | No |

| Name | Of | Holds | Default |
|---|---|---|---|
| `text` | `label`, `button` | Text, which may refer to values. See [values](#values-from-the-game) | Empty |
| `src` | `image` | Virtual path of the image, or a list of images for screens of several densities. See [images](#images) | None |
| `enabled` | `button` | `true`, `false`, or a value such as `"{has_save}"` | `true` |
| `autofocus` | `button` | Whether it has the focus when the file is shown | `false` |
| `value` | `bar` | A number, or a value such as `"{health}"` | `0` |
| `max` | `bar` | A number, or a value | `1` |

What the engine gives an element when the file says nothing, as the style
sheet of a browser does for HTML:

| Element | Defaults that differ from CSS |
|---|---|
| Every element | `color: "#ffffff"`, `font_family: sans-serif`, `font_size: 16` |
| `panel`, `label`, `image`, `bar` | `pointer_events: none` |
| `image` | `flex_shrink: 0` |
| `bar` | `width: 160`, `height: 16`, `flex_shrink: 0`, `background_color: "#00000080"`, `accent_color: "#4caf50"` |
| `button` | `padding: 8 16`, `background_color: "#3b4252"`, `justify_content: center`, `align_items: center`, `text_align: center`. In `hover` the background is `#4c566a`, in `active` `#2e3440`. In `focus` it has an outline of 2 in `#ffd166`. In `disabled` its opacity is 0.5 |

### Properties

Every property follows CSS unless the [table of deviations](#deviations-from-css)
says otherwise. Every element has every property. One that does not apply
does nothing.

**Box model.** [CSS Box Model](https://www.w3.org/TR/css-box-3/),
[CSS Box Sizing](https://www.w3.org/TR/css-sizing-3/),
[MDN](https://developer.mozilla.org/en-US/docs/Learn_web_development/Core/Styling_basics/Box_model).

| Property | Holds | Default |
|---|---|---|
| `width`, `height` | A length, a percentage of the parent, or `auto` | `auto` |
| `min_width`, `min_height` | A length or a percentage | `auto`, which is 0 |
| `max_width`, `max_height` | A length, a percentage, or `none` | `none` |
| `box_sizing` | `content-box` or `border-box` | `content-box` |
| `margin` | One to four values: top, right, bottom, left. `auto` takes what is left over | `0` |
| `margin_top`, `margin_right`, `margin_bottom`, `margin_left` | One value | `0` |
| `padding`, and `padding_top` and so on | The same, without `auto` and not below 0 | `0` |
| `border_width` | One to four numbers of pixels | `0` |
| `border_color` | A colour | That of the text |
| `border` | A width, `solid` or `none`, and a colour, in any order | None |
| `border_top`, `border_right`, `border_bottom`, `border_left` | The same, for one side | None |
| `border_top_width`, and `border_right_width` and so on | A number of pixels | `0` |

**Position.** [CSS Positioned Layout](https://www.w3.org/TR/css-position-3/),
[MDN](https://developer.mozilla.org/en-US/docs/Web/CSS/position).

| Property | Holds | Default |
|---|---|---|
| `position` | `relative` or `absolute` | `relative` |
| `top`, `right`, `bottom`, `left` | A length, a percentage, or `auto` | `auto` |
| `z_index` | A whole number. Higher is drawn later, among the elements next to it | `0` |
| `display` | `flex` or `none` | `flex` |
| `overflow` | `visible` or `hidden`, which cuts off at the padding box | `visible` |

An element that is pinned to a corner is `position: absolute` with two sides:

```yaml
position: absolute
bottom: 32
right: 32
```

**Flexbox.** [CSS Flexible Box Layout](https://www.w3.org/TR/css-flexbox-1/),
[CSS Box Alignment](https://www.w3.org/TR/css-align-3/),
[MDN](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_flexible_box_layout).

| Property | Holds | Default |
|---|---|---|
| `flex_direction` | `row`, `row-reverse`, `column`, `column-reverse` | `row` |
| `flex_wrap` | `nowrap`, `wrap`, `wrap-reverse` | `nowrap` |
| `justify_content` | `flex-start`, `flex-end`, `center`, `space-between`, `space-around`, `space-evenly` | `flex-start` |
| `align_items` | `stretch`, `flex-start`, `flex-end`, `center` | `stretch` |
| `align_self` | `auto`, and the values of `align_items` | `auto` |
| `align_content` | `stretch`, and the values of `justify_content` | `stretch` |
| `flex_grow`, `flex_shrink` | A number that is not below 0 | `0`, `1` |
| `flex_basis` | A length, a percentage, or `auto` | `auto` |
| `flex` | `none`, `auto`, or a number to grow by, which a number to shrink by and a basis may follow | None |
| `gap` | Between rows, then between columns. One value stands for both | `0` |
| `row_gap`, `column_gap` | A number of pixels | `0` |

**Colours and images.** [CSS Color](https://www.w3.org/TR/css-color-4/),
[CSS Backgrounds and Borders](https://www.w3.org/TR/css-backgrounds-3/),
[MDN on border-image-slice](https://developer.mozilla.org/en-US/docs/Web/CSS/border-image-slice).

| Property | Holds | Default |
|---|---|---|
| `background_color` | A colour | `transparent` |
| `background_image` | Virtual path of an image, or `none` | `none` |
| `border_image_source` | Virtual path of an image that is drawn in nine parts, or `none` | `none` |
| `border_image_slice` | One to four numbers: how far the corners reach into the image, in pixels of the image | `0` |
| `border_image_width` | One to four numbers: how wide the parts at the sides are drawn | As wide as in the image |
| `outline_width`, `outline_offset`, `outline_color` | A line around the border box that takes no room | `0`, `0`, that of the text |
| `opacity` | A number from 0 to 1 | `1` |
| `accent_color` | What a `bar` is filled with | `"#4caf50"` |
| `object_fit` | How an `image` fills its box: `fill`, `contain`, `cover`, `none`, `scale-down` | `fill` |
| `pointer_events` | `auto` or `none` | See above |
| `background` | A colour, a gradient, the virtual path of an image, or `none` | None |
| `background_image` | Also a gradient: `linear-gradient(...)` or `radial-gradient(...)` | `none` |
| `background_size` | `auto`, `cover`, `contain`, or one to two lengths, percentages, or `auto` | `100% 100%` |
| `background_position` | One or two of `left`, `center`, `right`, `top`, `bottom`, a length, a percentage | `0% 0%` |
| `background_repeat` | `repeat`, `no-repeat`, `repeat-x`, `repeat-y` | `repeat` |
| `border_image_repeat` | `stretch`, `repeat`, `round` | `stretch` |
| `object_position` | As `background_position` | `50% 50%` |
| `image_rendering` | `auto`, or `pixelated` for art that is drawn pixel by pixel | `auto` |

An image in nine parts is the usual way to skin a panel in a game. The
corners are drawn as they are, the edges are stretched along their side, and
the middle is stretched both ways.

**Text.** [CSS Fonts](https://www.w3.org/TR/css-fonts-4/),
[CSS Text](https://www.w3.org/TR/css-text-3/),
[MDN](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_fonts).

| Property | Holds | Default |
|---|---|---|
| `font_family` | The name of a family, of the file or of the application | `sans-serif` |
| `font_size` | A number of pixels above 0 | `16` |
| `font_weight` | A number from 1 to 1000, `normal`, or `bold`. The nearest weight the family has is taken | `400` |
| `color` | A colour | `"#ffffff"` |
| `text_align` | `left`, `center`, `right` | `left` |
| `line_height` | `normal`, a multiple of the size such as `1.5`, a percentage, or pixels as `"24px"` | `normal` |

More of what text looks like. [CSS Text](https://www.w3.org/TR/css-text-3/),
[CSS Text Decoration](https://www.w3.org/TR/css-text-decor-3/),
[CSS Writing Modes](https://www.w3.org/TR/css-writing-modes-3/).

| Property | In CSS | Holds | Default | Needs `rendering: sdf` |
|---|---|---|---|---|
| `font_family` | `font-family` | A list of families with commas between them. The first that has a character draws it | `sans-serif` | No |
| `font_weight` | `font-weight` | A number from 1 to 1000. The nearest face is taken. 600 and above is made bolder from a face below 600 when the family has no bold | `400` | No |
| `font_style` | `font-style` | `normal` or `italic`. An italic the family does not have is made by leaning its upright face | `normal` | No |
| `letter_spacing` | `letter-spacing` | `normal` or a length, which may be below 0 | `normal` | No |
| `word_spacing` | `word-spacing` | `normal` or a length | `normal` | No |
| `text_transform` | `text-transform` | `none`, `uppercase`, `lowercase`, `capitalize` | `none` | No |
| `text_decoration` | `text-decoration` | `none`, `underline`, `line-through`, a colour, and a thickness, in any order | `none` | No |
| `text_decoration_line`, `text_decoration_color`, `text_decoration_thickness` | The same names with hyphens | Each part of it on its own | None, that of the text, what the font asks for | No |
| `text_shadow` | `text-shadow` | Shadows with commas between them, or a list: to the right, down, a blur, a colour | `none` | No. With `sdf` the blur reaches no further than the distances do |
| `white_space` | `white-space` | `normal`, `nowrap`, `pre`, `pre-wrap`, `pre-line` | `pre-wrap` | No |
| `text_overflow` | `text-overflow` | `clip` or `ellipsis`, for a line that is not broken | `clip` | No |
| `text_stroke_width`, `text_stroke_color` | `-webkit-text-stroke-width`, `-webkit-text-stroke-color` | A line around every glyph, half of it over the glyph | `0`, that of the text | No. It is a picture of its own for every glyph unless the font is `sdf` |
| `color` | `color`, and `background-clip: text` with a gradient | A colour, or `linear-gradient(...)` or `radial-gradient(...)` | `"#ffffff"` | No |
| `direction` | `direction` | `ltr` or `rtl` | `ltr` | No |
| `text_align` | `text-align` | Also `start` and `end`, which depend on `direction` | `left` | No |

A font that is kept as distances is for text that is large, that changes
its size, or that is drawn with a wide line around it: one set of glyphs
serves every size. For small text, bitmaps are sharper, which is why they
are what a font is kept as unless it says otherwise.

**Round corners, shadows, gradients.**
[CSS Backgrounds and Borders](https://www.w3.org/TR/css-backgrounds-3/),
[CSS Images](https://www.w3.org/TR/css-images-3/).

| Property | In CSS | Holds | Default |
|---|---|---|---|
| `border_radius` | `border-radius` | One to four lengths or percentages: left top, right top, right bottom, left bottom | `0` |
| `border_top_left_radius`, `border_top_right_radius`, `border_bottom_right_radius`, `border_bottom_left_radius` | The same names with hyphens | One length or percentage | `0` |
| `border_top_color`, `border_right_color`, `border_bottom_color`, `border_left_color` | The same names with hyphens | A colour | That of `border_color` |
| `box_shadow` | `box-shadow` | Shadows with commas between them, or a list: `inset` or not, to the right, down, a blur, how much larger, a colour | `none` |
| `overflow: hidden` | `overflow` | Cuts off at the round corners of the padding box | |
| `outline_width`, `outline_offset`, `outline_color` | `outline` | Follows the round corners | |

```yaml
border_radius: 12
border: "2px solid #88c0d0"
background: "linear-gradient(to right, #bf616a, #ebcb8b)"
box_shadow: "0 12px 32px 4px rgba(0, 0, 0, 0.6), inset 0 1px 0 #ffffff40"
```

**Moving and turning.** [CSS Transforms](https://www.w3.org/TR/css-transforms-1/).

| Property | In CSS | Holds | Default |
|---|---|---|---|
| `transform` | `transform` | `none`, or steps: `translate(x, y)`, `translateX()`, `translateY()`, `rotate()`, `scale()`, `scaleX()`, `scaleY()` | `none` |
| `transform_origin` | `transform-origin` | As `background_position` | `50% 50%` |

An element is drawn where its transform puts it, with everything inside it,
and the pointer finds it there. Where it is in the layout stays what it was,
as in CSS.

**A shader of its own.** Not in CSS. See [shaders](#shaders-of-elements).

| Property | Holds | Default |
|---|---|---|
| `shader` | Virtual path of a shader without an extension, or `none` | `none` |
| `shader_values` | A map of names and values: a number, a colour, a list of up to four numbers, or a value of the game as `"{charge}"` | None |

### States

A state is named as its pseudo-class in CSS and holds properties.
[MDN](https://developer.mozilla.org/en-US/docs/Web/CSS/Pseudo-classes).

| State | The element is in it |
|---|---|
| `hover` | While the pointer is over it, or over an element inside it |
| `active` | While it is held down: with the pointer, or with accept while it has the focus |
| `focus` | While it has the focus |
| `disabled` | While it cannot be used. No other state applies then |

Several states apply together. Where two write the same property, the later
of `focus`, `hover`, `active` wins. Every element has states, not only
buttons. An element that is to be in `hover` has to take the pointer, which
is `pointer_events: auto`.

What a file writes for an element wins over a state of the engine, as the
style sheet of an author wins over that of a browser. A button with a
`background_color` of its own therefore keeps it under the pointer, until
the file writes a `hover` for it.

### Deviations from CSS

| CSS | Here |
|---|---|
| Selectors, style sheets, and the cascade between rules | None. Properties are written on the element. The one part of the cascade that exists is that the file wins over the defaults of the engine |
| Inheritance: `color` and the font of an element reach what is inside it | None. Every element starts from the defaults |
| Units | `px` and `%`. A number without a unit counts as pixels, which CSS allows for 0 alone. No `em`, `rem`, `vw`, `vh`, `calc()` |
| A pixel of CSS | A unit of the file, which is as many pixels of the frame as the [scale](#scaling) says |
| `display` | `flex` and `none`. Every element is a flex container. No `block`, `inline`, `grid` |
| `position` | `relative` and `absolute`. It starts as `relative` and not as `static`, so an absolute element is always placed against its parent. No `fixed`, `sticky` |
| `min-width: auto` of a flex item is the size of its content | It is 0. An element shrinks below its content unless `min_width` says otherwise. Yoga does the same |
| `align-items: baseline`, `order`, `visibility` | Not read |
| `gap` in percent | Pixels only |
| `z-index` orders within a stacking context | It orders an element among the elements next to it |
| `opacity` draws an element with what is inside it as one picture | It is multiplied into the alpha of everything below. Two elements inside that overlap show through each other |
| `overflow: scroll`, `auto` | Not read. Nothing scrolls |
| `background-image` repeats at its own size | It is stretched over the border box, as `background-size: 100% 100%` |
| `border-image-slice` keeps the middle out unless `fill` is written | The middle is always drawn |
| `border-image-width` starts as the width of the border | It starts as the width the parts have in the image |
| `border-style` | `solid` and `none` |
| Colours | The notations with `#`, `rgb()`, `rgba()`, and the names `transparent`, `black`, `white`. No other names, no `hsl()` |
| `white-space: normal` joins spaces and line feeds | They are kept. Lines are broken at spaces, as `pre-wrap` |
| `pointer-events` is inherited and starts as `auto` | It is not inherited, and starts as `none` for everything but a button |
| A shorthand and the property it stands for apply in the order they are written | The shorthand is read first, wherever it is written |
| `:hover` and the other pseudo-classes have the order of the style sheet | The order is `focus`, `hover`, `active` |
| `disabled` and `hidden` of HTML | `enabled`, since `disabled` names the state. `hidden` is as in HTML |
| `border-radius` in percent is of the width for the corner from side to side, and of the height from top to bottom, which gives a corner that is part of an ellipse | It is of the shorter side, and a corner is part of a circle. `50%` makes the short sides of a box half circles |
| `border-radius` with two radii for a corner, as `10px / 20px` | One radius for a corner |
| The inside of a border whose sides differ in width is round by what is left of the radius on each side | The same, worked out to a pixel near the outline and less exactly far from it |
| `box-shadow` is as soft in its corners as along its sides | The blur is worked out from the distance to the outline, which is exact along a side and a little tighter in a corner |
| `linear-gradient` to a corner, as `to right bottom`, is turned so that the other two corners have the colour of the middle | It runs at 45 degrees between the two sides, which is the same for a square |
| `radial-gradient` has a shape, a size, and a place | An ellipse around the middle of the box that reaches its corners |
| Gradients with lengths for their colours, `repeating-linear-gradient`, `conic-gradient` | Percentages, and up to 8 colours |
| `background-size` starts as `auto` | It starts as `100% 100%` |
| Several backgrounds, `background-attachment`, `background-origin`, `background-clip` | One image, which is placed against the border box and cut off at it |
| `background-repeat: space`, `round`, and `border-image-repeat: space` | Not read |
| A background that is drawn more than 4096 times | Is stretched over the element, and says so once |
| `overflow: hidden` inside `overflow: hidden` cuts off at the round corners of both | At the rectangle of both, and at the round corners of the inner one |
| A `transform` that turns, around `overflow: hidden` | What is inside is cut off at the rectangle around the element as it is drawn, and not at its round corners |
| `transform` with `skew()`, `matrix()`, and three dimensions | Not read |
| `filter`, `backdrop-filter`, `mix-blend-mode`, `clip-path`, `mask` | Not read |
| `text-align` starts as `start` | It starts as `left`. Text that runs from right to left writes `text_align: start` or `right` |
| Text that mixes directions is put in order by the Unicode Bidirectional Algorithm | A text is split into parts of one direction, which are laid out in the order they are written in, from the side `direction` says. Numbers run from left to right. See [text](#text) |
| `letter-spacing` is not applied to scripts whose letters are joined | The same: Arabic, Syriac, Mongolian, N'Ko, Mandaic, Adlam |
| `text-transform` follows the rules of the language | It knows Latin with accents, Greek, Cyrillic, and Armenian, and a sharp s as two capitals. Nothing depends on the language |
| `text-overflow: ellipsis` needs `overflow: hidden` | It needs `white_space` to be `nowrap` or `pre`, and cuts off at the width of the box |
| `text-decoration-style`, `text-underline-offset`, `text-decoration-skip-ink` | One line that is solid, where the font says, across the letters that reach below it |
| `text-shadow` and `box-shadow` take lengths in `em` | In pixels |
| `-webkit-text-stroke` | `text_stroke_width` and `text_stroke_color`, without a prefix |
| `font-style: oblique` with an angle | `italic`, which leans by 0.2 when it is made |
| `font-synthesis`, `font-variation-settings`, `font-feature-settings`, `font-kerning` | Not read. Kerning and ligatures are on, a bold and an italic are made when they are missing |
| `image-set()` and `srcset` | `src` of an `image` as a list of `{ src, scale }` |
| `image-rendering: crisp-edges` | `pixelated` |
| `object-position` with four values, as `right 10px bottom 20px` | One or two values |

## Made to be changed by hand

| Decision | Reason |
|---|---|
| A name that is not known is an error | `colour` would otherwise be ignored without a word |
| A message names the file, the line, and what was expected | `hud.ui.yml:9: 'width' of label 'health' is 'wide', where a number of pixels, a percentage such as 50%, or auto was expected` |
| Every problem of a file is reported, from the top of the file to its end | A file is corrected in one pass |
| A file that is wrong is not shown at all | Half a menu is worse than none |
| What is left out keeps its default | A file holds what was decided and nothing else |
| Anchors, aliases, and tags are refused | As in scenes |

## Layout

Layout is the work of a `LayoutEngine`, an interface in neon-core. The
implementation is `Flex_LayoutEngine`, which follows the algorithm of the
specification and uses no library.

**Yoga is not used.** Yoga, the implementation of flexbox by Meta, was the
first choice. Adding it as a submodule was refused by the permission system
of the session this was built in, and needs a decision of the owner. The
interface is shaped so that a `Yoga_LayoutEngine` in `lib/neon-yoga` can take
the place of the engine's own without anything else changing.

| Part of flexbox | Covered |
|---|---|
| Direction, reversed directions | Yes |
| `flex_grow`, `flex_shrink`, `flex_basis`, with limits, as in 9.7 of the specification | Yes |
| `justify_content`, `align_items`, `align_self`, `align_content`, with what each falls back to | Yes |
| Wrapping, reversed wrapping, gaps | Yes |
| `auto` margins | Yes |
| Absolute boxes, with every combination of sides, and where one goes that names no side | Yes |
| Percentages, and when a size counts as known, as in 9.8 | Yes |
| `baseline`, `order`, `min-width: auto`, `visibility: collapse`, writing modes | No |

### Scaling

A file is made for one size of the frame, its `reference_size`. At another
size everything in it grows or shrinks by one factor, the scale. This is what
the ratio of device pixels is on the web.

| `scale` | The scale is |
|---|---|
| `fit` | The smaller of width and height of the frame, each divided by that of the reference. Everything the file was made for stays in the frame |
| `width`, `height` | That side of the frame, divided by that of the reference |
| `none` | 1. A unit is a pixel |

| Frame | Scale with `fit` | The frame in units of the file |
|---|---|---|
| 1920 by 1080 | 1 | 1920 by 1080 |
| 1280 by 720 | 0.667 | 1920 by 1080 |
| 1280 by 1024 | 0.667 | 1920 by 1536 |
| 3440 by 1440 | 1.333 | 2580 by 1080 |

A frame of another shape has more units along one side. What is pinned to a
corner stays at its corner, and what is sized in percent follows.

Every edge is drawn at a whole pixel. Two boxes that touch share an edge.

**Text stays sharp** because it is not scaled. A font is drawn into its atlas
at the size it has on the screen, which is `font_size` times the scale,
rounded to a whole pixel. A pixel of the atlas is then a pixel of the frame.
Rows of glyphs are at whole pixels. From side to side a glyph is at a
quarter of a pixel, see [text](#text). A font that is kept as distances is
scaled, which is what it is for.

**Images stay sharp** in three ways. An image of shapes, such as an SVG, is
drawn at the size it has on the screen. An image of pixels comes in variants
for screens of several densities, of which the one that suits the scale is
taken. And an image that is drawn smaller than it is has smaller copies.
See [images](#images).

**Boxes stay sharp** because round corners, borders, shadows, and gradients
are worked out for every pixel, and not kept in images.

## Text

| | |
|---|---|
| Input | UTF-8. Bytes that are not UTF-8 are drawn as the replacement character |
| Glyphs | Drawn with FreeType, with light hinting: an outline is fitted to the rows of pixels, and left alone from side to side |
| Shaping | With HarfBuzz: kerning, ligatures, the joined forms of Arabic, the marks and the reordering of the scripts of India, and whatever else a font says in its OpenType tables |
| Scripts | Every script a font of the text has. No font for a script other than Latin is asked for unless a file names one |
| Characters | Basic Latin, Latin-1 Supplement, Latin Extended-A, and common punctuation are drawn when a font is first used at a size. Every other glyph is drawn when a text first asks for it |
| A character no font of the text has | Drawn as the replacement character, or as `?` |
| Lines | End at a line feed, and at a space when the next word does not fit. A word that is wider than a line is not broken. Chinese, Japanese, and Korean are broken between any two characters, but not in front of what closes a sentence or a bracket |
| Font of the runtime | Inter, regular and bold, as the family `sans-serif` |

### Even spacing

| | Before | Now |
|---|---|---|
| Pairs of letters | Every letter as wide as it is alone | Moved together where the font says, as `AV` and `To` |
| Where a glyph is placed | At the nearest whole pixel, up to half a pixel from where it belongs | At the nearest quarter of a pixel, up to an eighth from where it belongs |
| The picture of a glyph | One | Up to four, each moved by a quarter of a pixel more, drawn when it is first needed |
| Rows | At whole pixels | At whole pixels, and fitted to them by hinting |

What this costs is that the stem of a letter that lies between two pixels is
drawn over both, and is softer than one that lies on a pixel. Text that is
spaced evenly and a little softer reads better than text that is sharp and
uneven. It was decided by looking at both.

### Fonts

```yaml
fonts:
  - family: title
    src: assets://fonts/inter/Inter-Bold.ttf
    weight: 700
    rendering: sdf
  - family: body
    src: assets://fonts/body-italic.ttf
    style: italic
  - family: arabic
    src: assets://fonts/noto/NotoSansArabic-Regular.ttf
```

| Name | Holds | When it is left out |
|---|---|---|
| `family` | What the font is asked for by | None. It has to be written |
| `src` | Virtual path of a TrueType or OpenType font | None. It has to be written |
| `weight` | A number from 1 to 1000 | `400` |
| `style` | `normal` or `italic` | `normal` |
| `rendering` | `bitmap`: glyphs at the size they are drawn at. `sdf`: glyphs as distances to their outline, kept at 48 pixels and drawn at every size by a shader | `bitmap` |

**Several fonts for one text.** `font_family: "sans-serif, arabic"` draws
every character with the first family that has it. Inter has no Arabic, so
the Arabic of a text is drawn with Noto Sans Arabic, and the rest with Inter.

**Right to left.** A text is split where its font, its script, or its
direction changes, and each part is shaped on its own. A part whose script
runs from right to left comes out in its order, joined as it has to be,
whatever `direction` says. `direction: rtl` says that the parts are laid out
from the right, and that `text_align: start` is the right.

| Text | Drawn |
|---|---|
| An Arabic or Hebrew word or sentence | In its order |
| A word of Arabic in a sentence of English, or the other way around | In its order, where it is written |
| A number in a text from right to left | From left to right |
| A sentence that mixes directions and is broken into lines, brackets and quotation marks between two directions | Not put in order the way the Unicode Bidirectional Algorithm asks for. See [open questions](#open-questions) |

| Not supported | |
|---|---|
| The Unicode Bidirectional Algorithm | See above |
| Colour emoji | Fonts that keep their glyphs as PNG images are not read, since FreeType is built without libpng. Fonts with layers of colours, as COLR version 0, are read by the code and were not tried with a font. COLR version 1 is not read |
| Variable fonts | The axes of a font are not set. A file for every weight is what works |
| Breaking Thai, Lao, Khmer, and Burmese | They are written without spaces, and breaking them needs a dictionary |
| Breaking a word, hyphens | A soft hyphen is left out |
| Rich text | One style for a whole text |
| Text in a column from top to bottom | Lines run from side to side |

## Images

| Format | Read with | What is read |
|---|---|---|
| PNG | stb_image | 8 and 16 bits for each channel, with and without alpha, with a palette |
| JPEG | stb_image | Baseline and progressive. Not arithmetic coding, not 12 bits |
| TGA | stb_image | With and without compression |
| BMP | stb_image | Without compression, and with alpha |
| PSD | stb_image | **The picture of all layers put together**, which Photoshop writes into the file when it is saved with `Maximize Compatibility`. 8 and 16 bits for each channel, RGB alone. No layers, no effects, no text, no CMYK. 16 bits are brought down to 8 |
| GIF | stb_image | The first picture |
| SVG | LunaSVG | Shapes, paths, fills, strokes, gradients, patterns, clipping, masks, style sheets inside the file, images inside the file as `data:`. No text, no filters, no animation, no scripts. An SVG that refers to an image that is a file of its own is refused |
| WebP, KTX, DDS, PSB, AVIF | | Not read |

An image has to come from where the assets of the game come from, and not
from a player: neither library is made for files from anywhere.

### What an image is asked for by

```yaml
src: assets://ui/heart.png               # a file of pixels
src: assets://ui/shield.svg              # an image of shapes
src: assets://ui/icons.atlas.yml#coin    # a part of an atlas
src: surface://minimap                   # what a camera sees, or a surface
src:                                     # for screens of several densities
  - { src: assets://ui/gem-1x.png, scale: 1 }
  - { src: assets://ui/gem-2x.png, scale: 2 }
```

The first four are read by `src`, `background_image`, and
`border_image_source` alike. The list is read by `src` of an `image`.

| | |
|---|---|
| An image of shapes | Is drawn at the size in pixels it has on the screen, and again when that size changes. While the size changes from frame to frame, the picture there is is scaled, and a new one is drawn once the size has been the same for 3 frames. Up to 4 sizes of an image are kept |
| Screens of several densities | The image whose `scale` is the nearest above the scale of the file on the screen is taken, or the densest. It is what `image-set()` and `srcset` are on the web. The others are not read |
| Smaller copies | Every image of pixels has them, each half the size of the one before. They are made with alpha multiplied into the colours, so that what is see-through does not darken what is next to it. Between two copies the renderer blends |
| `image_rendering: pixelated` | The nearest pixel, for art that is drawn pixel by pixel |

### Atlases

An atlas is an image that holds several, and a description that says where
each one is. It is what artists hand over for the icons of a game.

```yaml
atlas: icons
version: 1

image: assets://ui/icons.png
scale: 1

regions:
  - name: coin
    x: 16
    y: 0
    width: 16
    height: 16

  - name: panel
    x: 32
    y: 8
    width: 32
    height: 24
    slice: [4, 6]
```

| Name | Holds | When it is left out |
|---|---|---|
| `atlas` | What the atlas calls itself | It has no name |
| `version` | The version of this layout, which is 1 | It counts as 1 |
| `image` | Virtual path of the image | None. It has to be written |
| `scale` | Pixels of the image for each unit of a file | `1` |
| `regions` | The parts of the image | None. It has to be written |

| Of a region | Holds | When it is left out |
|---|---|---|
| `name` | What it is asked for by, behind a `#`. Given once | None. It has to be written |
| `x`, `y`, `width`, `height` | Where it is in the image, in pixels from the left top corner | None. They have to be written |
| `slice` | One to four numbers: how far the corners reach into the part when it is drawn in nine parts. Top, right, bottom, left | It is not drawn in nine parts unless `border_image_slice` says so |

The parts of an atlas share one texture, and what is drawn from them shares
one draw call.

**TexturePacker and Aseprite** write the same as JSON. Reading them is a
second reader next to this one, which fills the same parts:

| Here | TexturePacker, `JSON (Hash)` and `JSON (Array)` | Aseprite, `--data` |
|---|---|---|
| `image` | `meta.image` | `meta.image` |
| `scale` | `meta.scale` | `meta.scale` |
| `name` | The name under `frames`, or `filename` | The name under `frames`, or `filename` |
| `x`, `y`, `width`, `height` | `frame.x`, `frame.y`, `frame.w`, `frame.h` | The same |
| `slice` | `borders` of a frame, where the nine-patch of TexturePacker is used | `meta.slices[].keys[].center`, against `bounds` |
| | `rotated` and `trimmed` are not known here. A frame that is turned or trimmed needs them | `meta.frameTags` names animations, which are not known here |

### What other engines read

| | Unity | Godot | Here |
|---|---|---|---|
| PNG, JPEG, TGA, BMP | Yes | Yes | Yes |
| PSD | Yes, with layers through the 2D PSD Importer | No | The picture of all layers put together |
| PSB | Yes, through the 2D PSD Importer | No | No |
| WebP | No | Yes | No. No library for it is part of the engine |
| SVG | Through the package Vector Graphics, as triangles or as pixels | Yes, drawn as pixels when it is imported, at a scale that is set there | Yes, drawn as pixels at the size it has on the screen |
| HDR, EXR | Yes | Yes | No |
| Compressed for the graphics card: DDS, KTX, ASTC, BC | Yes | Yes | No |
| Smaller copies | Yes | Yes | Yes |
| Atlases | Sprite Atlas, made by the editor | AtlasTexture, made by the editor | A description that is written by hand |
| Atlases of TexturePacker and Aseprite | Through importers | Through importers | No. See above |
| Trimmed and turned parts of an atlas | Yes | Yes | No |
| An image in nine parts | Sliced sprites, with tiles | StyleBoxTexture, NinePatchRect, with tiles | `border_image_*`, with `stretch`, `repeat`, and `round` |
| Variants for screens of several densities | Through variants of an atlas | Through the scale of the import | A list under `src` |
| Animated images | Sprite animation | AnimatedTexture, SpriteFrames | No |
| Fonts from files | TrueType and OpenType | TrueType, OpenType, WOFF, WOFF2 | TrueType and OpenType |
| Fonts as distances | TextMeshPro: one channel, or several | Several channels, MSDF | One channel, SDF. Corners are a little rounder than with several |
| Fonts as images | Through TextMeshPro | BMFont | No |
| Shaping, right to left | TextMeshPro: right to left without shaping. UI Toolkit: through Advanced Text Generator | HarfBuzz, ICU, the whole Bidirectional Algorithm | HarfBuzz. See [text](#text) for what is missing |
| Several fonts for one text | Yes | Yes | Yes |
| Colour emoji | Yes | Yes | No |
| Variable fonts | Yes | Yes | No |
| Round corners, shadows, borders | UI Toolkit: yes. uGUI: from images | StyleBoxFlat | Yes |
| Gradients | UI Toolkit: through vector images | GradientTexture | `linear-gradient`, `radial-gradient` |
| Shaders on elements | Materials | ShaderMaterial | `shader` |
| Themes | Style sheets | Theme | No. Style sheets are the work of another branch |
| A user interface in the world | World Space Canvas | SubViewport on a mesh | A surface. See [surfaces](#surfaces) |
| Blur of what is behind an element | Through a shader that reads the screen | Through a shader that reads the screen | No |

## Shaders of elements

An element names a shader of its own and what the shader is given:

```yaml
- type: button
  text: Resume
  shader: assets://shaders/ui/shine
  shader_values: { speed: 0.4, width: 0.15, lean: 0.4, tint: "#ffffff50" }
```

A shader is the half that colours pixels. Where the corners of an element
go is the work of the shader of the engine. It is written in GLSL, in the
folder `shaders/vulkan/ui` of the application, and compiled by the build
into `assets://shaders/ui/<name>.frag.spv`.

```glsl
#version 450
#extension GL_GOOGLE_include_directive : require

#include "../ui-shader.glsl"

layout (set = 2, binding = 0) uniform Values
{
    float intensity;
    vec4 tint;
} values;

void main()
{
    vec4 base = ui_base();
    frag_color = base * values.tint * values.intensity;
}
```

| A shader is given | By |
|---|---|
| The colour the engine would draw, with its texture, its round corners, and what it is cut off at | `ui_base()`. Alpha is multiplied into it |
| Where the pixel is in the box of the element, from 0 to 1 | `ui_element_uv()` |
| The size of the box in pixels | `ui_element_size()` |
| Seconds since the user interface was started | `ui_time()` |
| The place in the texture, the colour of the corner, the texture | `tex_coord`, `color`, `image` |
| What `shader_values` holds | The members of the block `Values`, by their names |

| Rule | Reason |
|---|---|
| The names of `shader_values` are the names of the members of `Values` | They are read from the compiled shader, so nothing is written down twice |
| A member is a `float`, an `int`, or a `vec2` to `vec4` | A matrix is left out |
| A value that is not written is 0 | |
| A number for a vector is the number in every part of it | |
| A name the shader does not declare is said once, as a warning | A name that was misspelled is found |
| A colour is written to `frag_color` with its alpha multiplied into it | That is what the renderer blends |
| The shader draws the box and the content of its element, and not what is inside it | A panel with a shader has children that are drawn as ever |
| A shader that cannot be used is said once, with the element and the file, and the element is drawn without it | |
| The block is up to 256 bytes | |
| Time moves with the time step of the run | A run with `--time-step` gives the same frames every time |

| Comes with the runtime | Does | Values |
|---|---|---|
| `assets://shaders/ui/shine` | A band of light that moves over the element | `speed`, `width`, `lean`, `tint` |
| `assets://shaders/ui/dissolve` | The element falls apart into grains | `amount`, `grain`, `edge` |
| `assets://shaders/ui/cooldown` | A shade that is wiped off clockwise | `progress`, `shade` |

Every element with a shader is a draw call of its own.

## Surfaces

A surface is where a user interface is laid out and drawn. It has a size in
pixels and a scale.

| Surface | Is |
|---|---|
| `window` | What the frame is drawn to. It is there from the start, with or without a window |
| Any other | A render target: an image that is drawn to in place of the frame, and shown by whatever names it |

What shows a surface names it as `surface://` and its name: a model as one
of its `textures`, an `image` of a user interface as its `src`.

### In a scene

```yaml
- name: monitor
  components:
    Transform:
      position: [-1, 0.5, 0]
      scale: [1.2, 0.9, 1]
    Renderable:
      model: assets://models/quad.obj
      shader: assets://shaders/unlit
      textures:
        - surface://terminal
    UiSurface:
      ui: assets://ui/terminal.ui.yml
      name: terminal
      size: [1024, 768]
```

| Of `UiSurface` | Holds | When it is left out |
|---|---|---|
| `ui` | Virtual path of the file | None. It has to be written |
| `name` | What the surface is called. Given once | None. It has to be written |
| `size` | `[width, height]` in pixels | `[1024, 1024]` |
| `scale` | How much larger everything on it is drawn | `1` |

The surface is there for as long as the entity is. The entity that carries
`UiSurface` need not be the one that shows it, and several models can show
one surface. `assets://models/quad.obj` is a flat square on which an image
lies once, with its top at the top.

The scene of the runtime is
[surface-demo.scene.yml](../app/NeonRuntime/assets/scenes/surface-demo.scene.yml):

```
NeonRuntime --scene assets://scenes/surface-demo.scene.yml
```

### What a camera sees

```yaml
- name: security-camera
  components:
    Transform:
      position: [0, 1.5, -2.5]
      rotation: [-25, 180, 0]
    Camera:
      target: texture
      texture: security
      size: [512, 512]
```

A camera whose `target` is `texture` draws what it sees into a render target
of that name. It stands next to the camera of the window and does not take
its place. A mirror, a monitor, and a map are models or images that show
`surface://security`.

| Rule | Reason |
|---|---|
| What shows a texture is left out of what is drawn into that texture | An image is not read while it is written |
| What a camera sees of a surface of a user interface is that of the frame before | The cameras draw while the world is updated, and the user interface after it |
| A texture that cannot be made is said once, and the camera draws nothing | |

### From code

```cpp
const int terminal = ui.CreateSurface("terminal", 1024, 768);
ui.LoadOnto(terminal, "assets://ui/terminal.ui.yml");

ui.SetTextOf("terminal", "door", "open");

// whoever knows where the player points says so
ui.SetPointerUv(terminal, hit.u, hit.v, is_pressed);

if (ui.WasClickedIn("terminal", "unlock")) { Unlock(); }
```

### Values and events of one user interface

A user interface is known by the name its file gives itself under `ui`.

| | |
|---|---|
| `SetNumber`, `SetText`, `SetFlag` | A value every user interface shares |
| `SetNumberOf`, `SetTextOf`, `SetFlagOf` | A value of one user interface, which wins there over the one all share |
| What a file starts a value with, under `values` | On the window: shared, as before. On any other surface: the value of that user interface |
| `OnClick`, `WasClicked` | For an element of that name in any user interface |
| `OnClickIn`, `WasClickedIn` | For an element of that name in one user interface |
| An event | Says the element, the user interface, and the surface |

The user interface that is shown during play and a terminal in the world
both have a button that is called `go`, and both show `{health}`. Neither
knows of the other.

### The pointer, the keys, and the controller

| | |
|---|---|
| The pointer of the window | Comes from the input of the application |
| The pointer of any other surface | Is said by the game: `SetPointer(surface, x, y, is_down)` in pixels of the surface, or `SetPointerUv` in parts of it, as the place on a texture is written. It stays until it is said again or taken away with `ClearPointer` |
| Several surfaces | Each has a pointer of its own, and all are looked at in one frame |
| The keys and the controller | Belong to one surface at a time. It is the window until the game calls `SetInputSurface`. What had the focus loses it |
| A click on a surface that does not have the keys | Is a click, and moves no focus |
| A modal file on the window | Takes the input away from the game and from the files below it on the window. A surface in the world is not held back by it |
| A modal file on a surface in the world | Takes nothing away from the game |

Where the player points on a screen in the world is found by casting a ray
at the model that shows it, which hands over the place on its texture. That
is the work of the physics, and is not part of this.

### How surfaces are drawn

| | |
|---|---|
| Order | What is drawn into render targets is recorded into commands of its own, which run in front of those of the frame. A target is finished before anything shows it, wherever in the frame it was drawn |
| Once | A target is drawn to once in a frame |
| Layout of the image | Ready to be read by a shader between two frames. Ready to be copied from behind the render pass that drew to it, which is where its smaller copies are made |
| Smaller copies | Every target has them, and they are made again whenever it was drawn to. A surface in the world is seen from afar and from the side |
| Formats | Those of the frame, so that every pipeline that draws into the frame draws into a target |
| What is behind a user interface | Nothing. A surface is see-through where nothing is drawn |
| A target that is made after the model that shows it | The model shows plain white until the frame after the target was made |
| A target that is destroyed | Is released once the frame that may show it is finished. Models that show it show plain white from then on |
| Without a window | The same |
| Every surface is drawn in every frame | Whether something changed or not. See [left for later](#left-for-later) |

## Values from the game

A game sets values by name. A file refers to them. The game never reaches for
an element.

```cpp
ui.SetNumber("health", 75);
ui.SetText("player", "Ada");
ui.SetFlag("paused", true);
```

| In a file | Shows |
|---|---|
| `text: "Health: {health}"` | `Health: 75` |
| `text: "{speed:1} m/s"` | A number with one digit behind the point |
| `text: "{{health}}"` | `{health}`, the brackets themselves |
| `value: "{health}"` | A bar that follows the value |
| `hidden: "{!paused}"` | An element that is shown while the game is paused |
| `enabled: "{has_save}"` | A button that can be used while there is a saved game |

| Rule | Reason |
|---|---|
| A whole number is written without a point, any other with two digits behind it | `75`, not `75.000000` |
| A name without a value is shown as it is written, and logged once | A name that was misspelled is seen on the screen |
| `values` of a file do not replace what the game has set | A file can be tried without the game |
| Values belong to the user interface, not to a file | Two files show the same health |
| A text is made again only when a value has changed | Nothing is formatted in a frame in which nothing changed |

## Input and focus

| Input | Does |
|---|---|
| The pointer over an element that takes it | The element is in `hover` |
| The button of the pointer, pressed and released on the same button | A click |
| Up, down, left, right | Moves the focus to the nearest button in that direction. What lies straight ahead is preferred. At the edge the focus stays |
| Accept | A click on what has the focus |

| Action | Keys | Controller |
|---|---|---|
| `Ui_Up`, `Ui_Right`, `Ui_Down`, `Ui_Left` | Arrow keys | Pad, left stick |
| `Ui_Accept` | Return, space | The lower button |
| `Ui_Cancel` | Backspace | The right button. Nothing in the engine reacts to it yet |
| `Pointer_Primary` | | The first button of the mouse |

Something has the focus when a button writes `autofocus: true`, when a file
is modal, when the pointer presses a button, when a direction is pressed, or
when the game calls `Focus(name)`.

### What is left for the game

The game reads the input through the user interface, which hands it on
without what it has used. `main.cpp` gives the world
`ui_system.GetGameInput()` in place of the input system.

| Used | When | The game does not see |
|---|---|---|
| The pointer | It is over an element that takes it, or a press began on a button | Where the pointer is, and its button |
| The keys of the user interface | Something has the focus | `Ui_Up` to `Ui_Cancel` |
| Everything | A file is modal | Any action, the pointer, the motion of the mouse |

Moving with W, A, S, and D is never used by a user interface that is not
modal.

**The cursor.** A game that turns the view with the mouse hides the cursor.
While a modal file is shown the cursor is shown whatever the game asked for,
and hidden again afterwards.

**Files on top of each other.** The file shown last is on top and is asked
first. A modal file takes the input from the files below it.

## Events

Both ways exist. They report the same.

```cpp
// called at the end of Update()
ui.OnClick("start", [&] { StartGame(); });

// or asked for, by a system of the game
if (ui.WasClicked("start")) { StartGame(); }

for (const neon::UiEvent &event : ui.GetEvents()) { /* event.element, event.document */ }
```

| Rule | Reason |
|---|---|
| An event is there from one `Update()` to the next | A system of the world reads it in the same frame |
| Callbacks are called last, from a copy of the events | A callback is free to load and unload files |
| A name has one callback | The second replaces the first. No function takes it away |
| An element without a name reports nothing | There is nothing to tell it by |

## How it is drawn

The renderer draws triangles. Everything a user interface shows is built from
them in neon-core.

| Step | Where |
|---|---|
| An element draws its background, images, border, content, children, and outline | `UiElement::Paint` |
| Every rectangle becomes four corners and two triangles, and joins a batch | `UiPainter` |
| A batch is handed to the renderer when the texture changes or what is cut off changes | `UiPainter` |
| The renderer draws a batch with one draw call | `Render2DContext::DrawTriangles` |

A rectangle without a texture joins any batch, since every corner says
whether it reads the texture. A panel, its border, its bars, and all text of
one font and size are one draw call.

| A new draw call starts where | |
|---|---|
| The texture changes | Another image, another font, another size of a font, another page of an atlas of glyphs |
| What is cut off changes | `overflow: hidden`, with or without round corners |
| The way a texture is read changes | `image_rendering: pixelated` |
| The shader changes | Every element with a `shader` is a call of its own |
| The surface changes | What is drawn into a render target, and what is drawn into the frame |

Round corners, borders, shadows, and gradients start none: a shape is a
rectangle that refers to numbers, which are handed over with the call.

| Draw calls, at 1920 by 1080 | Before | Now |
|---|---|---|
| `hud.ui.yml` as it was | 6, for 46 rectangles | 6, for 46 rectangles |
| `hud.ui.yml` with round corners, a bar that cuts off at them, the shadow of a text, and a button with a shader | | 8, for 52 rectangles |
| `gallery.ui.yml` | | 25, for 516 rectangles |
| `surface-demo.scene.yml`: the window and a terminal in the world | | 14, for 97 rectangles |

The user interface says how many it takes when that changes, at the level
`debug`.

### Shapes

| Shape | Is worked out from |
|---|---|
| A box with round corners | The distance of the pixel to the outline of the box |
| Its border | That distance, and the distance to the inside of the border, whose corners are parts of ellipses where two sides differ in width. A pixel has the colour of the side it lies least deep in, so two sides meet along the line from the corner of the outside to that of the inside |
| The shadow around a box | The distance to the box as the shadow has it: moved, and larger. It fades along the curve of a blur, and is not drawn under the box |
| The shadow in a box | The same, from the inside |
| A gradient | Where the pixel lies along the line of the gradient, or around its middle |
| A glyph that is kept as distances | The distance the texture holds. A line around the glyph and a blur are other distances of the same picture |

The edge of a shape is smoothed over one pixel, at every size and under
every transform, since the shader knows how large a pixel is in the shape.
Shadows and gradients are drawn with less than a step of noise, so that a
slow change has no bands. No shape needs a texture.

| | |
|---|---|
| Blending | Alpha is multiplied into the colours, in the textures when they are loaded and by the shader. A pixel that is see-through adds no colour to its neighbours, so nothing has a fringe |
| Depth | Not tested and not written |
| Order | Surfaces in the world first, each into its image. Then the window, after the scene, into the same image. A screenshot holds it |
| Without a window | The same |
| Without a user interface | Nothing is created and nothing is drawn. The frame is the same byte for byte |

### What a renderer has to implement

`Render2DContext` names no graphics API. A renderer that implements it draws
every user interface: the one of the engine, and a library that hands the
application triangles and textures, such as RmlUi. The Vulkan renderer does
so in `VK_Renderer2D`.

| Function | Does |
|---|---|
| `CreateTexture(width, height, pixels)` | A texture from bytes in memory: red, green, blue, alpha |
| `LoadTexture(path)` | A texture from an image file, read through the file system |
| `GetTextureSize(texture, width, height)` | Its size in pixels |
| `DestroyTexture(texture)` | Releases it |
| `DrawTriangles(triangles)` | Draws, in one call: corners with a place in pixels, a place in the texture, a colour, and whether the texture is read; indices in threes; one texture or none; a translation; a rectangle to cut off at, or none |
| `GetRenderResolution()` | The size of the frame |

What follows can be left out by a renderer. What asks for it is told that it
is not there, and does without.

| Function | Does | Without it |
|---|---|---|
| `UpdateTexture(texture, x, y, width, height, pixels)` | Replaces a part of a texture, for glyphs that were drawn since | A new texture is made for every page of glyphs that changes |
| `CreateTextureWith(width, height, pixels, options)` | A texture with smaller copies, or one that starts again past its edge | Images have no smaller copies |
| `CreateMaterial(shader_path)`, `DestroyMaterial(material)` | A shader of elements | Elements are drawn without their shaders |
| `CreateRenderTarget(name, width, height)`, `DestroyRenderTarget(target)`, `FindRenderTarget(name)`, `GetRenderTargetSize(...)` | An image that is drawn to and shown | There are no surfaces but the window |
| `BeginRenderTarget(target, clear)`, `EndRenderTarget()` | What is drawn between them is drawn into the target | |
| `GetRenderTargetTexture(target)` | The texture a target is drawn with in two dimensions | |

What `DrawTriangles` takes has grown. A corner says which shape it belongs
to and where it is in it. A call holds its shapes, a box with round corners
that nothing is drawn outside of, how its texture is read, its shader with
its values and the box of its element, and the time. A renderer that knows
nothing of them draws rectangles where shapes would be.

| What it has to do | |
|---|---|
| Places | Pixels from the left top corner, to the right and down |
| Blending | By alpha, over what is there. Alpha of what it is given is not multiplied into the colours |
| Depth and culling | None |
| Textures | Not repeated, and without smaller copies |
| What cannot be created | Tried once |

## How it is built

| Piece | Location | Role |
|---|---|---|
| `UiContext` | neon-core | What a game sees: files, values, events, focus |
| `UiSystem` | neon-core | What the runtime sees: `Update`, `Draw`, and the input for the game |
| `Tree_UiSystem` | neon-core | The implementation |
| `UiFile`, `UiDocument` | neon-core | Reads a file into a tree of elements |
| `UiElement`, and the five kinds | neon-core | An element. What is the same for every kind is in the base class |
| `UiElementTypes` | neon-core | The kinds by name |
| `UiStyle`, `css-values` | neon-core | The properties, and values as CSS writes them |
| `UiValues`, `UiTemplate` | neon-core | The values of the game, and text that refers to them |
| `UiInputGate` | neon-core | The input less what was used |
| `UiPainter`, `UiResources` | neon-core | Batches, and the fonts and images that are loaded once |
| `LayoutEngine`, `Flex_LayoutEngine` | neon-core | Layout |
| `FontRasterizer` | neon-core | The interface to a font: glyphs by their number, moved by parts of a pixel, as bitmaps or as distances |
| `TextShaper` | neon-core | The interface to shaping: text in, glyphs with their places out |
| `GlyphAtlas` | neon-core | The glyphs of a font at one size, drawn on demand into pages |
| `ShapeText`, `PlaceShapedText` | neon-core | Splits a text into parts of one font, script, and direction, shapes them, breaks lines, and places glyphs |
| `FontAtlas`, `PlaceText` | neon-core | What text was drawn with before. Kept for what uses it |
| `FT_FontRasterizer`, `HB_TextShaper` | neon-freetype | The implementations, with FreeType and HarfBuzz |
| `STB_FontRasterizer` | neon-stb | The implementation with stb_truetype, which stands in where FreeType is not wanted. It knows glyphs by their number and moves them by parts of a pixel. It draws neither distances nor the line around a glyph, and text is not shaped with it unless a shaper is set |
| `ImageDecoder`, `VectorImageRasterizer` | neon-core | The interfaces to images of pixels and to images of shapes |
| `STB_ImageDecoder` | neon-stb | The implementation, with stb_image |
| `LUNA_VectorImageRasterizer` | neon-lunasvg | The implementation, with LunaSVG |
| `UiBoxPaint`, `ui-image-paint` | neon-core | The shapes of a box, and where an image goes in one |
| `css-functions`, `ui-paint` | neon-core | Gradients, shadows, and transforms as CSS writes them |
| `UiSurfaceView`, `UiSurfaceLoading`, `UiSurfaceFormat` | neon-core | The component that gives an entity a surface, its system, and how it is written |
| `UiClock` | neon-core | Moves the time of the user interface on with that of the world |
| `VK_RenderTarget`, `VK_ShaderValues` | neon-vulkan | A render target, and the values a shader declares |
| `Render2DContext` | neon-core | Drawing in two dimensions |
| `VK_Renderer2D` | neon-vulkan | The implementation |
| `UiView`, `UiViewLoading`, `UiViewFormat` | neon-core | The component of a scene, its system, and how it is written |

Nothing outside neon-stb includes a header of stb, nothing outside
neon-freetype one of FreeType or HarfBuzz, and nothing outside neon-lunasvg
one of LunaSVG. The libraries are linked privately. Files are read through
`DocumentFormat` and the file system, and handed to the libraries as bytes.

### Libraries

| Library | Version | Licence | Built |
|---|---|---|---|
| [FreeType](https://freetype.org) | 2.14.3 | The FreeType License, which asks that the documentation of a product says that it uses FreeType. Or GPL 2 | From its own CMake files, as a static library. `FT_DISABLE_ZLIB`, `FT_DISABLE_BZIP2`, `FT_DISABLE_PNG`, `FT_DISABLE_HARFBUZZ`, `FT_DISABLE_BROTLI` are on, `FT_ENABLE_ERROR_STRINGS` is off, and `SKIP_INSTALL_ALL` is on for FreeType alone. FreeType reads fonts that are compressed with gzip with the zlib it brings itself |
| [HarfBuzz](https://harfbuzz.github.io) | 14.5.0 | The "Old MIT" license | From the one file it offers for that, `src/harfbuzz.cc`, as a static library, without its own build files. `HB_MUTEX_IMPL_STD_MUTEX` and `HB_NO_PRAGMA_GCC_DIAGNOSTIC_ERROR` are defined. Nothing else is, which leaves out FreeType, ICU, glib, Graphite, CoreText, Uniscribe, DirectWrite, and GDI. Without exceptions and without information about types at run time |
| [LunaSVG](https://github.com/sammycage/lunasvg) | 3.5.0 | MIT | From its own CMake files, as a static library. `LUNASVG_BUILD_EXAMPLES` is off, `LUNASVG_DISABLE_LOAD_SYSTEM_FONTS` is on, `USE_SYSTEM_PLUTOVG` is off |
| [PlutoVG](https://github.com/sammycage/plutovg), which LunaSVG brings | 1.3.1 | MIT. It holds parts of FreeType, under the FreeType License, and of stb, which is in the public domain or under MIT | With LunaSVG. `PLUTOVG_BUILD_EXAMPLES` is off, `PLUTOVG_DISABLE_FONT_FACE_CACHE_LOAD` is on |

HarfBuzz reads a font with its own OpenType functions. It does not use
FreeType, and FreeType does not use HarfBuzz, so neither depends on the
other.

| Asset | Licence |
|---|---|
| Noto Sans Arabic, Regular | SIL Open Font License 1.1, in `assets/fonts/noto/LICENSE.txt`. The whole font as it is published |
| The images of the gallery, the flat square | Made for the engine |

**The user interface of a game says that it uses FreeType**, which its
license asks for. The engine says so in `assets/CREDITS.md`.

`Runtime` has no new argument. It is given a user interface with
`SetUiSystem`, and runs as before without one.

### Elements are a tree of their own

They are not entities of the world.

| Reason | Detail |
|---|---|
| Order | What is drawn on top and where the focus goes follow the order of elements. A query of the store hands over blocks in the order of memory |
| Sizes | They follow from each other up and down the tree, which a layout engine works out over a tree |
| Behaviour | A kind of element brings behaviour. A component is data alone |
| Lifetime | A menu outlives the scene it is shown over |
| Saving | A scene saves every entity. Elements would be written into it |

The world and the user interface meet in one place, the component `Ui`.

### A kind of element of a game

One class, and one line that adds it:

```cpp
class Minimap final : public neon::UiElement
{
  float _zoom = 1.0f;

public:
  void ApplyDefaults(neon::UiStyle &style) const override
  {
    style.layout.width = neon::LayoutLength::Pixels(128);
    style.layout.height = neon::LayoutLength::Pixels(128);
  }

  void ReadAttributes(const neon::DataReader &reader) override
  {
    reader.Read("zoom", _zoom);
  }

  void PaintContent(
    neon::UiPainter &painter,
    const neon::UiFrame &frame,
    const neon::UiRectangle &content_box,
    float opacity) override;
};

ui_system.GetElementTypes().Add<Minimap>("minimap");
```

## What was weighed

| | Language | Licence | Draws through the renderer of the application |
|---|---|---|---|
| **Own YAML files with the vocabulary of CSS, and Yoga. Chosen for now**, with the engine's own layout in place of Yoga | YAML | Own. Yoga is MIT | Yes |
| [RmlUi](https://github.com/mikke89/RmlUi) | A dialect of HTML and CSS | MIT | Yes, through an interface the application implements. It has data binding, animation, and bindings for Lua |
| [NoesisGUI](https://www.noesisengine.com) | XAML | Proprietary | Yes, through an interface the application implements |
| [Ultralight](https://ultralig.ht) | HTML and CSS | Proprietary | Yes, or into a bitmap |
| [Slint](https://slint.dev) | Its own | GPL, or commercial | No, it brings renderers of its own |
| [Clay](https://github.com/nicbarker/clay) | C, layout only | zlib | Yes, it hands over what to draw |
| [Dear ImGui](https://github.com/ocornut/imgui) | C++ | MIT | Yes. For tools, not for what players see |

Own files were chosen because they are read the way scenes are, with the same
messages, and because the step to a library stays open: `Render2DContext`
takes what RmlUi and Clay hand over. None of the libraries is part of the
engine.

## How it was checked

| Check | Result |
|---|---|
| The demo scene without a user interface, 1920 by 1080, without a window | The same image as before, SHA-256 `9244e048a6839d08` |
| The demo at 1920 by 1080, without a window, looked at | Text is readable and sharp, nothing has a fringe, the panel lets the scene show through, everything sits where the file says |
| The demo at 1280 by 1024 and at 3440 by 1440 | The corners hold, everything is smaller and larger by the scale. The size was changed in `main.cpp` for it, since the runtime has no option for it |
| The demo with a window, 3 frames | Runs and ends with exit code 0. The window was not looked at |
| 120 checks of layout, worked out by hand from the specification | Pass |
| 300 checks of the user interface with files of YAML | Pass |
| 1574 tests in all, before what follows | Pass, 3 disabled as before |
| 2052 tests in all, with what follows | Pass, 3 disabled as before |
| The scene with surfaces in the world with a window, 5 frames | Runs and ends with exit code 0. The window was not looked at |
| The demo scene without a user interface, again | The same image, SHA-256 `9244e048a6839d08` |
| Small text at 11, 12, 13, 14, and 16 pixels, before and after, enlarged | Spaced evenly. `11` no longer touches, `AVATAR` and `To.` are moved together, the letters of `Resume` are as far apart as each other |
| Round corners, enlarged 8 times | Smooth, and the border follows them. Sides of four colours and four widths meet along the line between the corners of the outside and the inside |
| Shadows | Soft, without bands |
| The SVG of the gallery at 32 and at 128 pixels | Sharp at both. Each is a picture of its own |
| The shaders at two times, with `--time-step` and `--screenshot-at` | The band of light is elsewhere. The others do what they say |
| The terminal in the world | The right way up and the right way around: its red corner is at the left top, its green one at the right top, its blue one at the left bottom. Its colours are those of the file |
| What the second camera sees | Is shown on the second monitor. The terminal is seen from behind there, and is mirrored, as a sheet of glass is |
| Arabic | Joined, and from right to left. Next to English in one line, each in its order |

| What was seen and is not right | |
|---|---|
| The stems of letters differ in how sharp they are in small text | A stem that is placed between two pixels is drawn over both. It is what even spacing costs without the three colours of a pixel of a screen, which are not used |
| The heart of the demo looks as it did | It is drawn at exactly half its size, where blending four pixels is what a smaller copy holds. Its edge is dark in the image itself. Smaller copies show below half the size |
| Corners of glyphs that are kept as distances are a little round at large sizes | One distance for a pixel cannot hold a corner. Several can, which is MSDF, and needs a library |
| The glow of a text that is kept as distances is no wider than 8 pixels at the size 48 | It ends where the distances do |
| The edge of a shadow in a corner is a little tighter than along a side | See [deviations](#deviations-from-css) |

| Not checked | Why |
|---|---|
| Linux and Windows | They cannot be built on the machine this was made on |
| A controller | None was plugged in. The code compiles and follows the documentation of SDL |
| The pointer and the keys with a window | The mapping is in the SDL2 backend, which needs a display. What the user interface does with the input is tested |
| A display of high density | The pointer is scaled to the pixels that are drawn to, which was not tried |

## Left for later

| | |
|---|---|
| Yoga | See [layout](#layout) |
| Scrolling | `overflow: scroll`, and a wheel in the input |
| Text input | A field to type into, and text from the input system |
| More elements | Slider, checkbox, list, tabs |
| Animation | `transition` of CSS |
| Themes | Styles that are shared by files |
| Localisation | Text by key, and fonts for other scripts |
| Rich text | Several styles in one text |
| Surfaces that are drawn when something changed | Every surface is drawn in every frame. What decides whether a user interface has changed is the work of another branch |
| A ray that finds where the player points on a screen in the world | The physics. It calls `SetPointerUv` |
| Atlases of TexturePacker and Aseprite | See [atlases](#atlases) |
| `filter` and `backdrop_filter` | Both need what is behind an element, or the element itself, as an image of its own. Render targets are what they would be built from |
| Opacity of an element with what is inside it as one picture | The same |
| Fonts for other scripts in the runtime | Noto Sans Arabic is there to show that it works. A game brings the fonts of the languages it is translated into |
| Writing a file | `UiFile` reads. The editor will write |
| An option for the size of the frame | `--size 1280x720` |

## Open questions

Standards to consider next:

- **The Unicode Bidirectional Algorithm**, for text that mixes directions.
  [SheenBidi](https://github.com/Tehreer/SheenBidi), under Apache 2.0, is a
  small library in C that does it and nothing else. It would stand behind
  an interface of neon-core that hands over the parts of a line in the
  order they are drawn in, which is what `ShapeText` works out by itself
  today.
- **WebP**, which Godot reads and artists hand over. libwebp, under a BSD
  license, would stand behind `ImageDecoder`.
- **Colour emoji.** Fonts that keep glyphs as PNG images need libpng in
  FreeType. Fonts with layers of colours need to be tried with a font.
- **Variable fonts**, through the axes FreeType and HarfBuzz both set.
- **Fonts as several distances**, MSDF, for corners that stay sharp at large
  sizes. msdfgen, under MIT, makes them.
- **A style block with selectors**, in the file or in one of its own. It is
  the step from properties on the element to CSS, and brings the cascade and
  inheritance with it.
- **Roles for accessibility**, named as in WAI-ARIA, so that a screen reader
  can say what an element is.
- **RmlUi**, as the alternative of taking over a whole stack of standards in
  place of YAML files.

Others:

- Whether Yoga replaces the layout engine of the engine, or stands next to it.
- Whether values should have a scope, such as one set for each player in a
  game on a split screen.
- Whether `Ui_Cancel` closes the modal file on top by itself.
- Whether the focus should wrap around at the edge.
- Layout is worked out twice in every frame. For large trees it should be
  worked out when something changed.
- Image decoding for models is in neon-vulkan. For the user interface it is
  behind `ImageDecoder`, which models could use as well.
- Whether what a file of the window starts its values with should be the
  values of that file, as it is on every other surface.
- Whether a surface in the world should be lit by the scene, or glow by
  itself as the demo does with the shader `unlit`.
