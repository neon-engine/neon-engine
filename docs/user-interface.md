# User interface

This note records how menus and what is shown during play are made, why they
are made that way, and what is still open.

**Current decision:** a user interface is a YAML file that lists elements.
It is meant to be read and changed by hand. What the file says follows web
standards: the names and the meanings of its properties are those of CSS, so
that what a property does is said by a specification and not by the engine.
Elements are placed by the rules of CSS Flexible Box Layout.

This is the first version. It draws, lays out, reacts to input, and is
defined in files. It is not a complete set of widgets. It is separate from
the interface of the editor, which will use Dear ImGui.

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
| `fonts` | A list of fonts, each with `family`, `src`, and `weight`. It is what `@font-face` is in CSS | The fonts of the application |
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
| `src` | `image` | Virtual path of the image | None |
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
| `object_fit` | How an `image` fills its box: `fill`, `contain`, `cover` | `fill` |
| `pointer_events` | `auto` or `none` | See above |

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
| `border-style` | `solid` and `none`. All four sides have one colour |
| `border-radius`, `box-shadow` | Not read. Round corners come from an image in nine parts |
| Colours | The notations with `#`, `rgb()`, `rgba()`, and the names `transparent`, `black`, `white`. No other names, no `hsl()` |
| `white-space: normal` joins spaces and line feeds | They are kept. Lines are broken at spaces, as `pre-wrap` |
| `pointer-events` is inherited and starts as `auto` | It is not inherited, and starts as `none` for everything but a button |
| A shorthand and the property it stands for apply in the order they are written | The shorthand is read first, wherever it is written |
| `:hover` and the other pseudo-classes have the order of the style sheet | The order is `focus`, `hover`, `active` |
| `disabled` and `hidden` of HTML | `enabled`, since `disabled` names the state. `hidden` is as in HTML |

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
rounded to a whole pixel. A pixel of the atlas is then a pixel of the frame,
and every character is placed at whole pixels.

## Text

| | |
|---|---|
| Input | UTF-8. Bytes that are not UTF-8 are drawn as the replacement character |
| Characters | Basic Latin, Latin-1 Supplement, Latin Extended-A, dashes, quotation marks, bullet, ellipsis, euro sign, trade mark sign, replacement character |
| A character the font does not have | Drawn as the replacement character, or as `?` |
| Lines | End at a line feed, and at a space when the next word does not fit. A word that is wider than a line is not broken |
| Font of the runtime | Inter, regular and bold, as the family `sans-serif` |

| Not supported | |
|---|---|
| Kerning | Characters are not moved closer together in pairs |
| Shaping | No ligatures, no joining as in Arabic, no marks that combine |
| Right to left | Text runs from left to right |
| Other scripts | Greek, Cyrillic, Chinese, Japanese, Korean, and the rest are not in the atlas |
| Emoji | Not drawn |
| Hinting | stb_truetype draws from the outline alone |
| Italic, underline, letter spacing, rich text | One style for a whole text |
| Breaking a word, hyphens | A soft hyphen is left out |

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
one font and size are one draw call. How many the demo takes was not measured.

| | |
|---|---|
| Blending | Alpha is multiplied into the colours, in the textures when they are loaded and by the shader. A pixel that is see-through adds no colour to its neighbours, so nothing has a fringe |
| Depth | Not tested and not written |
| Order | After the scene, into the same image. A screenshot holds it |
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
| `FontRasterizer`, `FontAtlas`, `PlaceText` | neon-core | The interface to a font, its atlas, and where characters go |
| `STB_FontRasterizer` | neon-stb | The implementation, with stb_truetype |
| `Render2DContext` | neon-core | Drawing in two dimensions |
| `VK_Renderer2D` | neon-vulkan | The implementation |
| `UiView`, `UiViewLoading`, `UiViewFormat` | neon-core | The component of a scene, its system, and how it is written |

Nothing outside neon-stb includes a header of stb_truetype. The library is
linked privately. Files are read through `DocumentFormat` and the file
system.

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
| 1574 tests in all | Pass, 3 disabled as before |

| What was seen and is not right | |
|---|---|
| The distance between letters is uneven in small text | Characters are placed at whole pixels and are not kerned |
| The heart of the demo has rough edges | It is drawn at half its size, and textures of a user interface have no smaller copies |

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
| Round corners, shadows | `border_radius`, `box_shadow`, in the shader |
| Writing a file | `UiFile` reads. The editor will write |
| An option for the size of the frame | `--size 1280x720` |

## Open questions

Standards to consider next:

- **SVG** for images and icons that stay sharp at every scale.
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
- Image decoding for the user interface is in neon-vulkan, as it is for
  models. It belongs behind an interface of its own.
