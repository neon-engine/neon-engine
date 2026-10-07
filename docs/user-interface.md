# User interface

This note records how menus and what is shown during play are made, why they
are made that way, and what is still open.

**Current decision:** a user interface is a recipe, a YAML file that lists
elements, see [recipes.md](recipes.md). It is meant to be read and changed by
hand, under the rules every recipe shares. What the file says follows web
standards: the names and the meanings of its properties are those of CSS, so
that what a property does is said by a specification and not by the engine.
Elements are placed by the rules of CSS Flexible Box Layout.

This is the second version. It draws, lays out, reacts to input, scrolls,
takes text, animates, and is defined in files: elements in a UI recipe, and their
look in style sheets that are CSS. It is separate from the interface of the
editor, which will use Dear ImGui.

A user interface is shown on a **surface**. The window is one. A screen in
the world is another, and so is what a camera sees. See
[surfaces](#surfaces).

| Section | What it covers |
|---|---|
| [The file](#the-file) | Elements, properties, states |
| [Style sheets](#style-sheets) | CSS files, selectors, the cascade, inheritance, units, media queries |
| [Scrolling](#scrolling) | `overflow`, scrollbars, what scrolls with what |
| [Typing](#typing) | `input` and `textarea`, the caret, the clipboard, the input method |
| [Choosing](#choosing) | `checkbox`, `toggle`, `radio`, `slider`, `select` |
| [Animation](#animation) | Transitions and keyframes |
| [From code and scripts](#from-code-and-scripts) | Finding, changing, making, and hearing of elements |
| [Points, pixels, and density](#points-pixels-and-density) | The scale on a display of high density |
| [Surfaces](#surfaces) | The window, screens in the world, and what a camera sees |

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

Two user interfaces to look at are
[hud.ui.yml](../tests/game/assets/ui/hud.ui.yml) of the tests, what is shown during
play, and the engine's own [settings.ui.yml](../app/NeonRuntime/engine/ui/settings.ui.yml)
with its theme [settings.css](../app/NeonRuntime/engine/ui/settings.css), a
menu with everything that can be typed and chosen. Each is shown in two
ways:

```
NeonRuntime --ui assets://ui/hud.ui.yml
NeonRuntime --scene assets://scenes/hud-demo.scene.yml
NeonRuntime --ui engine://ui/settings.ui.yml
NeonRuntime --scene assets://scenes/settings-demo.scene.yml
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
| A property | As in CSS, with an underscore where CSS has a hyphen. Scene recipes write names the same way | `background_color`, `flex_direction` |
| A keyword | As in CSS, hyphen included | `space-between`, `row-reverse`, `border-box` |
| A length | A number, which counts as pixels, or text with `px` or `%` | `200`, `200px`, `50%`, `auto` |
| Several values | Text with spaces between them, or a list | `8 16`, `[8, 16]` |
| A colour | As in CSS, in quotes. **Preferred.** A list as in scene recipes is read as well | `"#ff8000"`, `"#ff800080"`, `"#f80"`, `rgb(255, 128, 0)`, `rgba(255, 128, 0, 0.5)`, `[1, 0.5, 0]` |

A `#` starts a comment in YAML, so a value with one in it needs quotes as a
whole: `border: "2px solid #4c566a"`.

### At the top

| Name | Holds | When it is left out |
|---|---|---|
| `ui` | What the file calls itself. Events carry it | It has no name |
| `version` | The version of this layout, which is 1 | It counts as 1 |
| `styles` | A list of style sheets, as paths next to the file or virtual paths. See [style sheets](#style-sheets) | None |
| `templates` | Elements by name, which `template` and lists make copies of. See [lists](#lists) | None |
| `cancel` | What cancel does: `auto`, `close`, `blur`, or `none`. See [input](#input-and-focus) | `auto` |
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
| `class` | Classes a style sheet asks for, with spaces between them: `slot selected` | None |
| `title` | What a tooltip shows when the pointer rests on the element | No tooltip |
| `tab_index` | Where the element comes in the order of the tab key: `0` as the file has it, a number above 0 in front of that, `-1` never | `0` |
| `draggable` | Whether the element can be dragged onto another, which reports `drag_start`, `drag_over`, `drop`, `drag_end` | `false` |
| `template` | The name of a template the element starts from. What the element writes replaces what the template has | None |
| `for_each` | A list of the game that the element is repeated for, with the template it is made from. See [lists](#lists) | None |
| `hidden` | `true`, `false`, or a value such as `"{paused}"` or `"{!paused}"`. It is the attribute `hidden` of HTML | `false` |
| `hover`, `active`, `focus`, `disabled` | Properties that replace those of the element while it is in the state. See [states](#states) | Nothing changes |
| The properties | See [properties](#properties) | Each keeps its default |

### Elements

| `type` | What it is | In HTML | Next to the properties it holds | Holds `children` |
|---|---|---|---|---|
| `panel` | A box, with a background and a border if it has them | `div` | Nothing | Yes |
| `label` | A text | `span` | `text` | No |
| `image` | An image from a file | `img` | `src`, which has to be written | No |
| `button` | What a player chooses. It reports a click. With `action: close` it also closes its file, as the Back button of a menu. With `action: scene` and the `scene` to change to it asks the game for that scene and closes its file, as the Start button of a title screen, see [scenes.md](scenes.md#changing-the-scene). With `on_click: unlock` or `on_click: open('safe', door, $event)` it calls a function of the scripts by its name, see [scripting.md](scripting.md#what-the-user-interface-calls) | `button` | `text`, `enabled`, `autofocus`, `action`, `scene`, `on_click` | Yes, in place of `text` |
| `bar` | How much of something there is | `progress` | `value`, `max` | No |
| `input` | A text of one line that is typed into. See [typing](#typing) | `input` | `value`, `kind`, `placeholder`, `max_length`, `read_only`, `pattern`, `enabled`, `autofocus` | No |
| `textarea` | A text of several lines that is typed into | `textarea` | The same but `kind`, and `rows` | No |
| `checkbox` | A box that is ticked or not. See [choosing](#choosing) | `input type=checkbox` | `checked`, `text`, `enabled`, `autofocus` | No |
| `toggle` | A checkbox drawn as a switch | `input type=checkbox` with `role=switch` | The same | No |
| `radio` | One of several, of which one is chosen at a time | `input type=radio` | `group`, `value`, `checked`, `text`, `enabled`, `autofocus` | No |
| `slider` | A number between two others | `input type=range` | `min`, `max`, `step`, `value`, `enabled`, `autofocus` | No |
| `select` | One of several, from a list that opens | `select` | `options`, `value`, `placeholder`, `enabled`, `autofocus` | No |

| Name | Of | Holds | Default |
|---|---|---|---|
| `text` | `label`, `button` | Text, which may refer to values. See [values](#values-from-the-game) | Empty |
| `src` | `image` | Virtual path of the image, or a list of images for screens of several densities. See [images](#images) | None |
| `enabled` | `button` | `true`, `false`, or a value such as `"{has_save}"` | `true` |
| `autofocus` | `button` | Whether it has the focus when the file is shown | `false` |
| `value` | `bar` | A number, or a value such as `"{health}"` | `0` |
| `max` | `bar` | A number, or a value | `1` |
| `value` | `input`, `textarea` | The text, or a value such as `"{player_name}"`, which typing changes | Empty |
| `kind` | `input` | `text`, `password`, which shows dots, or `number`, which takes digits, a sign, and a point | `text` |
| `placeholder` | `input`, `textarea`, `select` | What is shown while nothing is typed or chosen | Empty |
| `max_length` | `input`, `textarea` | How many characters may be typed, `0` for any number | `0` |
| `read_only` | `input`, `textarea` | Whether the text can be selected and copied, and not changed | `false` |
| `pattern` | `input`, `textarea` | What the text has to fit for the element to be `:valid`: `*` for anything, `?` for one character, `[a-z]` for one of a range. An empty text always fits | None |
| `rows` | `textarea` | How many lines are seen | `2` |
| `checked` | `checkbox`, `toggle`, `radio` | `true`, `false`, or a value such as `"{fullscreen}"`. A radio is chosen while the value is its own `value` | `false` |
| `group` | `radio` | Which radios belong together. It has to be written | None |
| `value` | `radio` | What it stands for, which the value of the game becomes when it is chosen | Empty |
| `min`, `max`, `step` | `slider` | Numbers. `step` is above 0 | `0`, `100`, `1` |
| `value` | `slider` | A number, or a value such as `"{volume}"`, which moving the knob changes | `min` |
| `options` | `select` | A list of texts, or of maps with `value` and `text` | None |
| `value` | `select` | The value of what is chosen, or a value of the game such as `"{quality}"` | Nothing is chosen |
| `enabled`, `autofocus` | Every element that takes input | As for `button` | `true`, `false` |

What the engine gives an element when the file says nothing, as the style
sheet of a browser does for HTML:

| Element | Defaults that differ from CSS |
|---|---|
| Every element | `color: "#ffffff"`, `font_family: sans-serif`, `font_size: 16` |
| `panel`, `label`, `image`, `bar` | `pointer_events: none` |
| `image` | `flex_shrink: 0` |
| `bar` | `width: 160`, `height: 16`, `flex_shrink: 0`, `background_color: "#00000080"`, `accent_color: "#4caf50"` |
| `button` | `padding: 8 16`, `background_color: "#3b4252"`, `justify_content: center`, `align_items: center`, `text_align: center`. In `hover` the background is `#4c566a`, in `active` `#2e3440`. In `focus` it has an outline of 2 in `#ffd166`. In `disabled` its opacity is 0.5 |
| `input`, `textarea`, `select` | `padding: 6 10`, `border: 1`, `min_width: 160`, `overflow: hidden` (`auto` along the height of a `textarea`), `background_color: "#ffffff14"`, `border_color: "#ffffff4d"`, `cursor: text` (`pointer` for `select`). In `focus` the border is `#ffd166`. In `disabled` the opacity is 0.5 |
| `checkbox`, `toggle`, `radio` | `padding: 4`, `align_items: center`, `cursor: pointer`. In `focus` an outline of 2 in `#ffd166`. The box is 18 by 18, the switch 36 by 20 |
| `slider` | `min_width: 160`, `padding: 4 0`, `flex_shrink: 0`, `cursor: pointer`. The knob is 16, the track 4 high |

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
| `visibility` | `visible` or `hidden`, which keeps the room and draws nothing. Inherited | `visible` |
| `overflow` | `visible`, `hidden`, `scroll`, or `auto`, for both sides. See [scrolling](#scrolling) | `visible` |
| `overflow_x`, `overflow_y` | The same for one side. `visible` next to another value counts as `auto`, as in CSS | `visible` |

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
| `caret_color` | The colour of the caret of a text that is typed. Inherited | That of the text |

**Behaviour.** [CSS Overflow](https://www.w3.org/TR/css-overflow-3/),
[CSS Scrollbars Styling](https://www.w3.org/TR/css-scrollbars-1/),
[CSSOM View on scroll-behavior](https://www.w3.org/TR/cssom-view-1/#smooth-scrolling),
[CSS Basic User Interface on cursor](https://www.w3.org/TR/css-ui-4/#cursor).

| Property | Holds | Default |
|---|---|---|
| `scrollbar_width` | `auto`, which is 12 units, `thin`, which is 8, or `none` | `auto` |
| `scrollbar_color` | `auto`, or two colours: what is dragged, and what it is dragged along. Inherited | `auto`, which is the colour of the text at half and at an eighth of its alpha |
| `scroll_behavior` | `auto`, or `smooth`, which moves what the keys, the wheel, and the game scroll to over a quarter of a second | `auto` |
| `scroll_drag` | `none`, `auto`, which drags what is inside with the pointer and lets it run on, or `inertia`, the same with the pointer alone. Not in CSS | `none` |
| `cursor` | `auto`, `default`, `pointer`, `text`, `move`, `grab`, `grabbing`, `crosshair`, `not-allowed`, `wait`, `progress`, `ns-resize`, `ew-resize`, `nesw-resize`, `nwse-resize`, `none`. Inherited. The window is told the shape when it changes | `auto` |

**Transitions and animations.** See [animation](#animation).

| Property | Holds | Default |
|---|---|---|
| `transition_property` | `none`, `all`, or names of properties with commas between them | `all` |
| `transition_duration`, `transition_delay` | Times with a unit, `0.2s` or `200ms`, one for each property | `0s` |
| `transition_timing_function` | `ease`, `linear`, `ease-in`, `ease-out`, `ease-in-out`, `step-start`, `step-end`, `steps(n, position)`, `cubic-bezier(x1, y1, x2, y2)` | `ease` |
| `transition` | For each property: its name, a duration, a timing function, a delay, in the order of CSS | None |
| `animation_name` | Names of `@keyframes`, or `none` | `none` |
| `animation_duration`, `animation_delay` | Times with a unit | `0s` |
| `animation_timing_function` | As for a transition | `ease` |
| `animation_iteration_count` | A number, or `infinite` | `1` |
| `animation_direction` | `normal`, `reverse`, `alternate`, `alternate-reverse` | `normal` |
| `animation_fill_mode` | `none`, `forwards`, `backwards`, `both` | `none` |
| `animation_play_state` | `running` or `paused` | `running` |
| `animation` | For each animation, its parts in any order but that the first time is the duration and the second the delay | None |

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

A style sheet asks for these and for more, as pseudo-classes. See
[selectors](#selectors).

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
| A pixel of CSS | A unit of the file, which is as many pixels of the frame as the [scale](#scaling) says |
| Units | `px`, `%`, `em`, `rem`, `vw`, `vh`, `vmin`, `vmax`, and `calc()` of those, in style sheets. A UI recipe takes `px` and `%`. A number without a unit counts as pixels, which CSS allows for 0 alone. No `ch`, `ex`, `cm`, `in`, `pt` |
| `display` | `flex` and `none`. Every element is a flex container. No `block`, `inline`, `grid` |
| `position` | `relative` and `absolute`. It starts as `relative` and not as `static`, so an absolute element is always placed against its parent. No `fixed`, `sticky` |
| `min-width: auto` of a flex item is the size of its content | It is 0. An element shrinks below its content unless `min_width` says otherwise. Yoga does the same |
| `align-items: baseline`, `order` | Not read |
| `gap` in percent | Pixels only |
| `z-index` orders within a stacking context | It orders an element among the elements next to it |
| `opacity` draws an element with what is inside it as one picture | It is multiplied into the alpha of everything below. Two elements inside that overlap show through each other |
| `overflow: clip` | Not read. `hidden` cuts off and does not scroll by the pointer, which is what `clip` does in CSS; a script can still scroll it |
| A scrollbar takes room in the layout for `scroll` and, when it is shown, for `auto` | The same for `scroll`. For `auto` the bar is drawn over the padding at the right and the bottom, and takes no room |
| `scrollbar-gutter`, `scroll-snap-*`, `overscroll-behavior` | Not read |
| `background-image` repeats at its own size | It is stretched over the border box, as `background-size: 100% 100%` |
| `border-image-slice` keeps the middle out unless `fill` is written | The middle is always drawn |
| `border-image-width` starts as the width of the border | It starts as the width the parts have in the image |
| `border-style` | `solid` and `none`. All four sides have one colour |
| Colours | The notations with `#`, `rgb()`, `rgba()`, `hsl()`, `hsla()`, and the 148 names, in style sheets. A UI recipe takes `#`, `rgb()`, `rgba()`, and `transparent`, `black`, `white`. No `color()`, `lab()`, `oklch()` |
| `white-space: normal` joins spaces and line feeds | They are kept. Lines are broken at spaces, as `pre-wrap`. A `textarea` breaks a word that is wider than a line, as `overflow-wrap: anywhere` |
| `pointer-events` is inherited | It is not inherited. It starts as `none` for everything but what takes input: a button, and what is typed into and chosen |
| A shorthand and the property it stands for apply in the order they are written | In a UI recipe the shorthand is read first, wherever it is written. In a style sheet the order holds |
| `:hover` and the other pseudo-classes have the order of the style sheet | In a UI recipe the order of its states is `focus`, `hover`, `active`. In a style sheet the order holds |
| `disabled` and `hidden` of HTML | `enabled`, since `disabled` names the state. `hidden` is as in HTML |
| The cascade: origins, layers, `@scope`, `@container`, `@supports`, `@property` | The origins are the defaults of the engine, style sheets, and the element. No layers and no other at-rules than those listed under [style sheets](#style-sheets) |
| Selectors: `:nth-of-type()`, `:has()`, `:lang()`, `:target`, `:visited`, `::before`, `::after` | Not read. Pseudo-elements name the parts of elements |
| Media queries see the viewport in CSS pixels | They see the window in points, and `resolution` as its density: `(max-width: 900px)` matches a window narrower than 900 points, whatever the scale of the file. `orientation`, `aspect-ratio`, and `prefers-reduced-motion` are read; `hover`, `pointer`, `prefers-color-scheme` are not |
| `@font-face` | `font-family`, `src: url()`, `font-weight`. No `font-style`, `unicode-range`, formats |
| `inherit`, `initial`, `unset` | As in CSS. No `revert`, `revert-layer` |
| `transition-behavior: allow-discrete` | A property that is not interpolated switches halfway, as `allow-discrete` does. There is no keyword |
| `animation-timeline`, `animation-composition`, `animation-range` | Not read |
| `scroll-behavior: smooth` is the choice of the browser in its length | A quarter of a second, eased out |
| `cursor: url()` | Not read. The shapes of the system only |
| `caret-shape`, `::selection` on other than what is typed into | Not read. `::selection` styles the selection of an input and a textarea |
| `pointer-events` is inherited and starts as `auto` | It is not inherited, and starts as `none` for everything but a button |
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

The rules every recipe shares, that a name that is not known is an error,
that every problem is reported with its line, that what is left out keeps its
default, how numbers are written, and that anchors, aliases, and tags are
refused, are in [recipes.md](recipes.md#made-to-be-changed-by-hand). On top
of them:

| Decision | Reason |
|---|---|
| A message names the element as well | `hud.ui.yml:9: 'width' of label 'health' is 'wide', where a number of pixels, a percentage such as 50%, or auto was expected` |
| Problems are reported from the top of the file to its end | A file is corrected in one pass, in the order it is read |
| A file that is wrong is not shown at all | Half a menu is worse than none |
| A style sheet that cannot be read fails the file | As a UI recipe that is wrong does. A declaration that cannot be read is skipped and logged, see [style sheets](#style-sheets) |

## Style sheets

A file names its style sheets at the top, and they are CSS as it is written
anywhere. What an element writes on itself in the recipe still wins over a sheet,
as the attribute `style` of HTML does.

```yaml
ui: settings
styles: [settings.css, assets://ui/common.css]
```

```css
:root {
  --accent: #88c0d0;
  --row-gap: 12px;
}

@import "colours.css";

@font-face {
  font-family: Title;
  src: url("assets://fonts/title.ttf");
  font-weight: 700;
}

.row { gap: var(--row-gap); padding: 8px 12px; transition: background-color 0.15s ease-out; }
.row:hover { background-color: #4c566a; }
.row > label.name { width: 180px; color: var(--accent); }
button.primary:focus { outline: 2px solid #ffd166; }
input:invalid { border-color: #bf616a !important; }
select::list { background-color: #3b4252; }

@media (max-width: 900px) {
  .row { flex-direction: column; }
}

@media (prefers-reduced-motion) {
  .row { transition: none; }
}
```

| | |
|---|---|
| Where a sheet is | A path without a scheme is next to the file that names it. `@import` is next to the sheet it is written in. A virtual path is taken as it is |
| What is read | [CSS Syntax](https://www.w3.org/TR/css-syntax-3/): rules, declarations, comments, `!important`, strings, escapes, and `@charset`, `@import`, `@font-face`, `@keyframes`, `@media`. Any other at-rule is skipped with a warning |
| Names of properties | As in CSS, with hyphens. The same properties as in a UI recipe, and each keeps its meaning |
| What is wrong | A declaration that cannot be read is skipped and logged once as a warning, as `settings.css:14: 'colour' is not a property that is known`. A selector that cannot be read skips its rule. A sheet that cannot be read at all fails the file, as a UI recipe that is wrong does |
| Changing a sheet while the game runs | `UiContext::ReloadStyles()` reads the sheets again and keeps everything else. The runtime does not watch files |

### Selectors

[Selectors Level 4](https://www.w3.org/TR/selectors-4/).

| Selector | Matches |
|---|---|
| `button`, `*` | The kind of the element, or any |
| `.slot` | An element with the class |
| `#health` | The element with the name. A name is an id here |
| `[enabled=false]`, `[text]`, `[class~=slot]`, `[name^=slot-]`, `[name$=-3]`, `[name*=lot]`, `[lang|=en]` | A field of the element, as text. What an element has next to its properties, and `name`, `class`, `title`, `draggable` |
| `a b`, `a > b`, `a + b`, `a ~ b` | Inside, directly inside, right after, after |
| `:hover`, `:active`, `:focus`, `:focus-visible`, `:focus-within`, `:disabled`, `:enabled`, `:checked`, `:valid`, `:invalid` | The states. `:focus-visible` is `:focus` |
| `:first-child`, `:last-child`, `:only-child`, `:nth-child(2n+1)`, `:nth-last-child(2)`, `:empty` | Where the element is among the elements next to it. `odd` and `even` are read |
| `:root`, `:scope` | The root of the file, and what a query started from |
| `:not(...)`, `:is(...)`, `:where(...)` | As in CSS, with lists inside |
| `::placeholder`, `::selection`, `::tooltip`, `::scrollbar-thumb`, `::scrollbar-track`, `::box`, `::mark`, `::track`, `::thumb`, `::fill`, `::list`, `::option`, `::highlight`, `::arrow` | The parts of elements, which take a background and a colour of their own. A part is asked for with its element: `select::highlight`. `::-webkit-scrollbar-thumb` is read as `::scrollbar-thumb` |

Specificity counts ids, classes with attributes and pseudo-classes, and
types with pseudo-elements, as the specification says. `:where()` counts
nothing; `:is()` and `:not()` count their most specific argument.

### The cascade

What an element ends up with is decided in this order, the later winning:

| Origin | Order within it |
|---|---|
| The defaults of the engine for the kind of element, and for its state | As listed under [elements](#elements) |
| Style sheets, without `!important` | By specificity, then by the order of the sheets and of the rules in them |
| What the UI recipe writes on the element, and its states | As before: `focus`, `hover`, `active` |
| What a script set with `Set()` | The last value set |
| Style sheets, with `!important` | By specificity, then by order |

Elements are matched against selectors when something they are matched by
changed: a state, a class, a field, an element next to them, or a sheet.
A frame in which nothing changed matches nothing.

### Inheritance

Whether a property reaches what is inside an element is one word in the
table of properties, `inherited`, as the specification has it for each.
These are inherited: `color`, `font_family`, `font_size`, `font_weight`,
`line_height`, `text_align`, `visibility`, `cursor`, `caret_color`,
`scrollbar_color`. Everything else is not.

`inherit`, `initial`, and `unset` are read for every property, and custom
properties are inherited as in CSS.

### Units

| Unit | Stands for |
|---|---|
| `px`, or a number | A unit of the file |
| `%` | Of the parent, or of what the property refers to |
| `em` | The `font_size` of the element, or of its parent for `font_size` itself |
| `rem` | The `font_size` of the root |
| `vw`, `vh`, `vmin`, `vmax` | A hundredth of the width and the height of the file in its units |
| `calc()` | `+`, `-`, `*`, `/`, brackets, and `var()`. A percentage next to pixels is kept as both and settled in the layout |

### Media queries

[Media Queries Level 4](https://www.w3.org/TR/mediaqueries-4/). `width`,
`height`, `aspect-ratio`, `orientation`, `resolution`, and
`prefers-reduced-motion`, with `min-`, `max-`, the range syntax
`(400px <= width <= 900px)`, `and`, `not`, `only`, and lists. The width and
the height are the window in points, as they are on the screen. The
resolution is its density, in `dppx`, `dpi`, `dpcm`, or `x`. Reduced
motion is what the game set with `SetReducedMotion()`, since the engine
does not ask the operating system.

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

### Points, pixels, and density

A window has three sizes, and `WindowContext::GetMetrics()` gives them:
its size in **points**, which is what the operating system lays it out in;
its size in **pixels**, which is what is drawn to; and the **density**,
pixels for each point, which is 2 on a display that Apple calls Retina and
1.5 or 2 on Windows at 150 or 200 percent. SDL2 opens the window with
`SDL_WINDOW_ALLOW_HIGHDPI` and the hint for the scaling of Windows, so that
the pixels are the real ones. The pointer arrives in points and is scaled
to pixels by the backend, along each side by itself, since the two can
differ by a pixel.

The scale of a file is one rule:

```
scale = density × user scale × fit(points of the window / reference size)
```

where `fit` is the [scale mode](#scaling) of the file, and 1 for `none`.
The user scale is what the player asked for, `--ui-scale` or
`SetUserScale()`, and starts as 1.

| Window | Density | User scale | Scale of a file for 1920 by 1080 | What 100 units are |
|---|---|---|---|---|
| 1920 by 1080 points, 1920 by 1080 pixels | 1 | 1 | 1 | 100 pixels |
| 1920 by 1080 points, 3840 by 2160 pixels | 2 | 1 | 2 | 200 pixels, the same size on the screen |
| 1280 by 720 points, 2560 by 1440 pixels | 2 | 1 | 2 × 0.667 = 1.333 | 133 pixels: the file is fitted to the smaller window, and drawn sharp |
| 1280 by 720 points, 2560 by 1440 pixels | 2 | 1.5 | 2.0 | 200 pixels |
| 3440 by 1440 points, density 1 | 1 | 1 | 1.333, by the height | 133 pixels |

Text is rasterised at `font_size` × scale, rounded to a whole pixel, so
that it is sharp at every density. When the window is resized or moved to
a display of another density, every file is laid out again and its text
is rasterised anew; the atlases of the old size are released, so that
moving back and forth does not pile them up.

| Option of the runtime | Does |
|---|---|
| `--window-size WxH` | A window of that many points, in place of one that covers the display |
| `--render-scale N` | Without a window: N pixels for each point, as a display of that density gives. `--window-size 1280x720 --render-scale 2` renders 2560 by 1440 |
| `--ui-scale N` | The user scale |

Media queries see the window in points and the density as `resolution`,
so that a sheet can say what a narrow window and a dense display get.

**For the owner, on a real display.** These were not tried on a window
with a display, and are what to look at:

- On a Mac with a Retina display, and on Windows at 150 percent: text is
  sharp and the HUD is the size it is at 1920 by 1080 on a display of
  density 1.
- Moving the window between a display of density 1 and one of density 2:
  the interface changes size once, without a frame in which it is drawn
  at the wrong size, and `GetFontCount()` stays where it was.
- Resizing the window: the layout follows in the same frame. The Vulkan
  renderer does not make its swapchain again on resize, so the window is
  kept from being resized until it does.
- Clicking exactly on the edge of a button at density 2: the pointer is
  scaled to pixels along each side by itself.
- On Windows, that `SDL_HINT_WINDOWS_DPI_SCALING` gives the real pixels
  and not a bitmap that Windows stretches.

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
    src: engine://fonts/inter/Inter-Bold.ttf
    weight: 700
    rendering: sdf
  - family: body
    src: assets://fonts/body-italic.ttf
    style: italic
  - family: arabic
    src: engine://fonts/noto/NotoSansArabic-Regular.ttf
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
src: image://quake/face                  # pixels that were handed to the renderer under a name
src:                                     # for screens of several densities
  - { src: assets://ui/gem-1x.png, scale: 1 }
  - { src: assets://ui/gem-2x.png, scale: 2 }
```

The first five are read by `src`, `background_image`, and
`border_image_source` alike. The list is read by `src` of an `image`.

| | |
|---|---|
| An image of shapes | Is drawn at the size in pixels it has on the screen, and again when that size changes. While the size changes from frame to frame, the picture there is is scaled, and a new one is drawn once the size has been the same for 3 frames. Up to 4 sizes of an image are kept |
| Screens of several densities | The image whose `scale` is the nearest above the scale of the file on the screen is taken, or the densest. It is what `image-set()` and `srcset` are on the web. The others are not read |
| Smaller copies | Every image of pixels has them, each half the size of the one before. They are made with alpha multiplied into the colours, so that what is see-through does not darken what is next to it. Between two copies the renderer blends |
| `image_rendering: pixelated` | The nearest pixel, for art that is drawn pixel by pixel |

### Atlases

An atlas is an image that holds several, and an atlas recipe, `*.atlas.yml`,
that says where
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
  shader: engine://shaders/ui/shine
  shader_values: { speed: 0.4, width: 0.15, lean: 0.4, tint: "#ffffff50" }
```

A shader is the half that colours pixels. Where the corners of an element
go is the work of the shader of the engine. It is written in GLSL, in the
folder `engine/shaders/ui` of the runtime, and compiled by the build
into `engine://shaders/ui/<name>.frag.spv`.

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
| The place in the texture, the colour of the corner, the texture, and the sampler it is read through | `tex_coord`, `color`, `image`, `image_sampler`. The texture and the sampler are bound apart, as every shader of the engine binds them ([shaders.md](shaders.md#what-the-sources-keep-to)): a shader that reads the texture itself writes `texture(sampler2D(image, image_sampler), tex_coord)` |
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
| `engine://shaders/ui/shine` | A band of light that moves over the element | `speed`, `width`, `lean`, `tint` |
| `engine://shaders/ui/dissolve` | The element falls apart into grains | `amount`, `grain`, `edge` |
| `engine://shaders/ui/cooldown` | A shade that is wiped off clockwise | `progress`, `shade` |

Every element with a shader is a draw call of its own.

What a shader of an element is bound to, in the order of `ui-shader.glsl`:

| Set | Binding | What |
|---|---|---|
| 0 | 0 | `image`, a `texture2D`: the texture of the call |
| 0 | 1 | `image_sampler`, a `sampler`: what it is read through, smooth or pixel by pixel as `image_rendering` says |
| 1 | 0 | The shapes of the frame, which `ui_base()` reads |
| 2 | 0 | `Values`, what the shader is given |

`ui-shader.glsl` declares the first two and the shapes; a shader of an
element declares only `Values`.

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
      shader: engine://shaders/unlit
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

The scene of the tests is
[surface-demo.scene.yml](../tests/game/assets/scenes/surface-demo.scene.yml):

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

### Pointing at a screen in the world

`UiSurfacePointing` is the system that finds where the player points. A
ray starts at the camera that draws the window and goes where it looks,
which is where the dot in the middle of the HUD is. A screen is the square
of 1 by 1 of its entity, facing +Z as `quad.obj` does, so the ray meets it
by the entity's transform alone and needs no collider. The nearest screen
the ray hits from its front, within the `reach` of its `UiSurface`, gets
the pointer: `SetPointerUv` with the button of the pointer, the left button
of the mouse, or accept as the press. The left button is read on its own
since there is no pointer, and so no `Pointer_Primary`, while the cursor is
hidden, which it is while the player looks around. A screen with a `reach`
of 0 is pointed at from anywhere.

| | |
|---|---|
| `reach` of `UiSurface` | How near the camera has to be, in units of the world. 3 unless the scene says otherwise |
| `pointed_by` of `UiSurface` | Set by the system on the screen: who points at it, the nearest entity from the camera up that carries a `Player`, or the camera's own. It stays when the player looks away, and is who a click on the screen comes from: `event.instigator` of a [handler](scripting.md#what-the-user-interface-calls) |
| The flag `pointing` | Set by the system on the window's values while a screen is pointed at. The HUD turns its dot into a ring with `hidden: "{!pointing}"` |
| The keys and the controller | Stay with the window. A controller presses with accept, which is the press of the pointer on the screen, so nothing on the screen needs the focus |

`NeonRuntime` adds the system after `UiSurfaceLoading`, with the input as
the game sees it. The surface demo's terminal shows it: walk up to the
monitor, look at Unlock, press.

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
| A surface is drawn when something changed | Its image keeps what was drawn into it, so a frame in which nothing in the user interface changed draws into no surface, and the renderer makes no smaller copies. A change anywhere draws every surface again; one that is new, or could not be drawn into, is drawn in the next frame whatever changed. See [left for later](#left-for-later) |

## Scrolling

[CSS Overflow](https://www.w3.org/TR/css-overflow-3/),
[CSSOM View](https://www.w3.org/TR/cssom-view-1/).

```yaml
- type: panel
  name: list
  overflow_y: auto
  scroll_behavior: smooth
  height: 400
```

| What scrolls it | By |
|---|---|
| The wheel, and two fingers on a trackpad | A notch is 60 units. A trackpad gives its own distance |
| Dragging the thumb of the scrollbar, and pressing its track | The thumb follows the pointer. The track moves a page |
| Arrow keys, page up and down, home and end, while nothing that takes them has the focus | 40 units, nine tenths of the box, and the ends |
| The right stick of a controller | 1200 units a second at full deflection. Within a quarter of the way the stick is at rest and nothing scrolls, since the backends hand it over as it is |
| Dragging what is inside, with `scroll_drag: auto` or `inertia` | After the pointer moved 6 units. When let go it runs on and slows down |
| The focus moving to what is out of sight | What has the focus is brought into view |
| The game | `SetScroll()`, `GetScroll()`, `ScrollIntoView()` |

| Rule | |
|---|---|
| `scroll` reserves the room of the bar, `auto` draws the bar over the padding when there is something to scroll | As HTML does with a classic scrollbar for `scroll`. See the [deviations](#deviations-from-css) |
| `hidden` cuts off, and only the game scrolls it | As `overflow: clip` with `scrollTo()` in a browser |
| What is inside a box that scrolls along an axis is as large as it asks to be along it | `flex_shrink: 0` for that axis, so that a column of rows is not squeezed into the box |
| Nested boxes | The innermost that can move in that direction takes the wheel; when it is at its end the next one outside takes over |
| Only what can be seen is drawn | Elements that are outside of every box that cuts them off are skipped, and are not hit by the pointer |
| `scroll_behavior: smooth` | A quarter of a second, eased out, for the keys, the wheel, and `SetScroll()`. The thumb and dragging are always direct |

The scrollbar is drawn by the engine and styled as in
[CSS Scrollbars Styling](https://www.w3.org/TR/css-scrollbars-1/):
`scrollbar_width`, `scrollbar_color`, and the parts `::scrollbar-thumb` and
`::scrollbar-track`. The bar is 12 units wide, 8 for `thin`, and the thumb
is at least 24 long.

An element that scrolled reports `scroll`, with where it is now.

## Typing

`input` and `textarea` take text.
[HTML on input](https://html.spec.whatwg.org/multipage/input.html),
[on textarea](https://html.spec.whatwg.org/multipage/form-elements.html#the-textarea-element).

```yaml
- type: input
  name: player-name
  value: "{player_name}"
  placeholder: Your name
  max_length: 24
  pattern: "[A-Za-z]*"
```

| Input | Does |
|---|---|
| A click | Puts the caret between characters, nearest to the pointer. Two clicks select the word, three the line. With shift held, selects from the caret |
| Dragging | Selects |
| Typing | Inserts at the caret, in place of the selection |
| Left, right | Move by a character. With the word key, by a word. With shift, select |
| Up, down | Move between lines, remembering where the caret wanted to be. In a text of one line, to its ends |
| Home, end | To the ends of the line. With the shortcut key, of the text |
| Backspace, delete | Take a character. With the word key, a word |
| Return | Submits an `input`, and breaks the line in a `textarea`, which the shortcut key and return submit |
| Escape | Cancel, which takes the focus away. Backspace deletes and does not cancel here |
| Tab | Moves the focus on |
| The shortcut key with A, C, X, V, Z, Y, and shift Z | Select all, copy, cut, paste, undo, redo, redo |
| A controller | The directions move the caret as the arrows do, and accept does nothing |

The shortcut key is command on a Mac and control elsewhere; the word key
is option on a Mac and control elsewhere. The backend says which, so that
the engine does not ask what platform it is on.

| Rule | |
|---|---|
| Characters | A character is what the caret moves over: a code point with the marks that combine with it, a carriage return with its line feed, and a pair of regional indicators. It is what `max_length` counts. There is no library for it, and the joiners of emoji are not read |
| Undo | A word typed letter by letter is one step, and so is the space that ends it. 200 steps are kept |
| The clipboard | Through `ClipboardContext`, which SDL2 implements with the clipboard of the system. An input takes a text of one line: line breaks are left out, as HTML does |
| An input method | What it puts together is shown underlined at the caret, and is not in the text until it is done. The platform is told where the caret is, so that it shows its candidates there, and a keyboard on the screen where there is one |
| The keys | While a text is typed, the keys type: the game sees none of them, and neither do the directions of the interface. A controller still moves the game |
| `kind: password` | Shows a dot for every character. `number` takes digits, a sign, and a point |
| `pattern` | `*`, `?`, `[a-z]`, and `\` before a character. An empty text fits every pattern. The text is `:invalid` while it does not fit, and the field `valid` says so |
| `value: "{name}"` | The field shows the value of the game and sets it as it is typed, both ways. A game reads it with `GetValue()`, or hears `changed` |
| The caret | Blinks twice a second while the field has the focus, in `caret_color`. It is kept in view: an input moves its text sideways, a textarea scrolls |

An `input` reports `changed` with the text as it is typed, `submitted` on
return, and `focused` and `blurred`. What measures text is `UiTextMeasure`,
an interface with one implementation over the atlas, so that shaping can
replace it: where the caret is at a byte, and which byte is at a place.

## Choosing

| Element | The pointer | The keys and a controller |
|---|---|---|
| `checkbox`, `toggle` | A click ticks it, or takes the tick away | Accept does the same. Left and right move a `toggle` off and on |
| `radio` | A click chooses it, and lets go of the others of its `group` in the file | Accept chooses it |
| `slider` | A press puts the knob where the pointer is, and dragging moves it | Left and right move it a `step`, page up and down ten, home and end to the ends. The wheel moves it a step |
| `select` | A click opens the list, and a click on a choice takes it. A click anywhere else closes it | Accept opens it, up and down move through it, accept takes the choice, cancel closes it. While it is closed, up and down change the choice right away, as in HTML. Home and end go to the ends of an open list |

Each reports `changed`: `true` or `false`, the number, or the value. A
radio that is let go of reports `changed` with `false`, in the order of the
file. Each follows a value of the game when it is given one in brackets,
and sets it when it is chosen. The list of a `select` is drawn on top of
everything, eight rows at a time, and takes the pointer before what lies
under it.

Each is drawn from parts a style sheet reaches: `::box` and `::mark` of a
checkbox and a radio, `::track` and `::thumb` of a toggle, `::track`,
`::fill`, and `::thumb` of a slider, `::list`, `::option`, `::highlight`,
`::arrow`, and `::placeholder` of a select. What is ticked and chosen
matches `:checked`.

## Lists

A file makes elements for the rows of a list the game hands it, from a
template.

```yaml
templates:
  row:
    type: button
    name: "item-${index}"
    class: row
    text: "${number}. ${name} x${count}"

root:
  type: panel
  children:
    - type: panel
      name: list
      for_each: "{items}"
      template: row
```

```cpp
ui.SetList("items", {
  {{"name", "Sword"}, {"count", "1"}},
  {{"name", "Arrow"}, {"count", "40"}}
});
```

`${name}` in a template is a field of the row, `${index}` counts from 0
and `${number}` from 1. The elements are made again when the list is set,
and kept while it is not. A template is also what `template` on an element
and `CreateFromTemplate()` start from.

## Animation

[CSS Transitions](https://www.w3.org/TR/css-transitions-1/),
[CSS Animations](https://www.w3.org/TR/css-animations-1/),
[CSS Easing](https://www.w3.org/TR/css-easing-1/).

```css
.row { transition: background-color 0.15s ease-out, opacity 0.3s; }

@keyframes open-window {
  from { opacity: 0; margin-top: 40px; }
  to   { opacity: 1; margin-top: 0; }
}

.window { animation: open-window 0.4s cubic-bezier(0.2, 0.8, 0.2, 1) both; }
```

| Rule | |
|---|---|
| What is animated | Every property of the table. A number, a length, a colour, and edges are interpolated; a colour with its alpha multiplied in, as the specification asks. A keyword, a text, and an image switch halfway |
| A transition | Starts when the computed value of a property changes and the property is named in `transition_property`. One that is reversed while it runs is shortened, as the specification asks. What an element starts with is not a change |
| An animation | Runs from its first keyframe to its last, with the keyframes' own timing functions, `animation_direction`, `animation_fill_mode`, `animation_iteration_count`, and `animation_play_state`. A keyframe that is missing is the value of the element |
| Time | Every frame advances by the time of the window, or by `Advance(seconds)` when the runtime says so, as it does with `--time-step`. `SetTimeScale()` slows and pauses the user interface apart from the game. The same steps give the same frames |
| Layout | A property that moves boxes lays out again what changed, and a colour or an opacity only draws again |
| From code | `StartAnimation(element, "shake", "1s linear")` and `StopAnimation(element)` run keyframes by name |
| Reduced motion | `SetReducedMotion(true)` makes `@media (prefers-reduced-motion)` match. The engine does not ask the operating system |
| Events | `transition_ended` and `animation_ended`, with the name of the property or the animation in `value` |

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
| The pointer over an element that takes it | The element is in `hover`. After it rested for 0.6 seconds the `title` of the element is shown under it, styled as `::tooltip` |
| The button of the pointer, pressed and released on the same element | A click. Two within 0.4 seconds are a `double_click` as well |
| Up, down, left, right | Moves the focus to the nearest element in that direction that takes it, unless what has the focus uses the direction itself: a slider its left and right, a text its arrows. What lies straight ahead is preferred. At the edge the focus stays. A modal file keeps the focus inside |
| Tab, shift and tab | Moves the focus in the order of the file, around again at the end. `tab_index` puts elements in front, and `-1` leaves one out |
| Accept | A click on what has the focus |
| Cancel | What the file on top says: `close` takes it away and the focus returns to where it was before the file was shown; `blur` takes the focus away; `none` does nothing; `auto` closes a modal file and blurs otherwise. What has something open closes it first, as a select its list. The game hears `cancel` in every case |
| The wheel, the right stick | Scroll. See [scrolling](#scrolling) |
| Keys and text | Go to what has the focus. See [typing](#typing) |

| Action | Keys | Controller |
|---|---|---|
| `Ui_Up`, `Ui_Right`, `Ui_Down`, `Ui_Left` | Arrow keys | Pad, left stick |
| `Ui_Accept` | Return, space | The lower button |
| `Ui_Cancel` | Backspace, escape | The right button |
| `Pointer_Primary` | | The first button of the mouse |

These are the engine's, the same for every project. What the game reads is
named by its input map instead, `pause` among them, see [input.md](input.md).

Something has the focus when an element writes `autofocus: true`, when a
file is modal, when the pointer presses an element that takes the focus,
when a direction or tab is pressed, or when the game calls `Focus(name)`.
The cursor takes the shape `cursor` of what it is over, and the window is
told when it changes.

### The pause menu

The engine brings a pause menu and a settings menu of its own,
`engine://ui/pause.ui.yml` and `engine://ui/settings.ui.yml`, so that a game
has working menus without making any. A project that wants them names them
in its settings as `ui.pause_menu` and `ui.settings_menu`, as the sandbox,
the museum, and the bench do; a game with a look of its own names files of
its own there instead, and one that names none has no pause menu, as
neon-quake, which opens its own on escape.

The project names a file in its settings, `engine://ui/pause.ui.yml`, and
the runtime shows it when the action `pause` of the input map is pressed —
escape, or start on a controller, in the default map — and the user
interface did not use the press. The file is modal with `cancel: close`, so escape or the
right button of a controller take it away again, and so does its button
`resume`; its button `quit` closes the window. While it is shown the world
stands still: no system of the game runs, no time passes for the physics,
and what is there is drawn as it was. A key that closed the menu and is
still held does not open it again. Shift and escape close the window
whatever the game does, for when the game has stopped listening.

Its button `settings` shows the settings menu in its place,
`engine://ui/settings.ui.yml`, which the project names in its settings as
well. The world stays still while it is shown. It is modal, so the cursor is
there to use it, and its buttons Back and Apply close it with
`action: close`, as cancel does. Then the pause menu is back. A modal file
shows the cursor while it is shown, and the game has the mouse back once it
is closed, so a menu that takes the mouse writes `modal: true`.

### Which device the player uses

The user interface sets two values on the window from what the player
touched last, so that a file shows the hints that suit it: `input_device`
is `keyboard` or `gamepad`, and `gamepad` is the flag of it. The pause menu
hides one of two hint labels with `hidden: "{gamepad}"` and
`hidden: "{!gamepad}"`. While a controller is used the cursor stays hidden
in a modal file. An input script says `device gamepad` to try it.

### What is left for the game

The game reads the input through the user interface, which hands it on
without what it has used. `main.cpp` gives the world
`ui_system.GetGameInput()` in place of the input system.

| Used | When | The game does not see |
|---|---|---|
| The pointer | It is over an element that takes it, or a press began on a button | Where the pointer is, and its button |
| The keys of the user interface | Something has the focus | `Ui_Up` to `Ui_Cancel` |
| The keyboard | A text is typed | Every action a key gives, the keys, and the text. A controller is still seen |
| The wheel and the right stick | Something under the pointer, or what has the focus, scrolled by them | The wheel, the right stick |
| Everything | A file is modal | Any action, the pointer, the motion of the mouse |

The actions of the input map are worked out again from what is left, so a
key or a button the user interface used fires no action either: a click on
a button does not `shoot`, a text typed does not `move`, see
[input.md](input.md#how-a-game-reads-it). Moving with W, A, S, and D is
never used by a user interface that is not modal.

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

for (const neon::UiEvent &event : ui.GetEvents()) { /* event.element, event.document, event.scene */ }
```

A click of a button with `on_click` carries the function and what it is
handed in `event.call`, a `UiCall`, with the values that were named already
looked up; `ScriptRunning` hands those to the scripts with the entity that
shows the file, and code in C++ reads them from `GetEvents()` as it reads
any click. A click of a button with `action: scene` carries the scene in `event.scene`,
which the runtime hands to the world; it is empty for every other click.

| Rule | Reason |
|---|---|
| An event is there from one `Update()` to the next | A system of the world reads it in the same frame |
| Callbacks are called last, from a copy of the events | A callback is free to load and unload files |
| A name has one callback | The second replaces the first. No function takes it away |
| An element without a name reports nothing | There is nothing to tell it by |

Elements report more than clicks, to listeners on the element and on every
element. See [from code and scripts](#from-code-and-scripts).

| Event | When | Bubbles up |
|---|---|---|
| `click`, `double_click` | A click, and a second one within 0.4 seconds | Yes |
| `pointer_down`, `pointer_up`, `pointer_enter`, `pointer_leave` | The pointer's button, and the pointer coming and going. Enter and leave are told to each element on the way | Down and up |
| `wheel`, `scroll` | The wheel turned over the element, and the element scrolled | Wheel |
| `focused`, `blurred` | The focus came and went | No |
| `key_down`, `key_up` | A key, with its name in `value`, to what has the focus | Yes |
| `changed`, `submitted` | What is typed or chosen changed, and return in an input, with the value in `value` | Yes |
| `cancel` | Cancel was pressed, to what has the focus or to the root of the file on top | Yes |
| `drag_start`, `drag_over`, `drop`, `drag_end` | A `draggable` element is dragged, is over another, is let go on it, and is done | Yes |
| `transition_ended`, `animation_ended` | With the name of the property or the animation | No |

A game makes its sounds from these: a tick on `focused`, a clack on
`click`, a switch on `changed`, and one for `cancel`.

## From code and scripts

A game reaches elements through `UiContext`, by handles that never dangle:
a handle of an element that is gone finds nothing, and `IsAlive()` says
so. Nothing here needs a name in the file, though a name is the easiest way
to find something.

```cpp
neon::UiHandle sword = ui.FindByName("sword");
for (neon::UiHandle slot : ui.Query(".slot:not(.selected)")) { ui.AddClass(slot, "dim"); }

ui.Set(sword, "background-color", "#334");        // as a style sheet would, on the element
std::string colour = ui.GetComputed(sword, "background-color");  // "rgb(51, 51, 68)"
ui.SetField(sword, "text", std::string("Sword of Ada"));
ui.SetVisible(sword, false);
ui.FocusElement(sword);
ui.ScrollIntoView(sword);

neon::UiHandle made = ui.Create("{type: button, text: New}", ui.FindByName("slots"));
neon::UiHandle row = ui.CreateFromTemplate("row", ui.FindByName("list"), {{"name", "Bow"}});
ui.Move(made, ui.FindByName("actions"), 0);
ui.Remove(made);

int listening = ui.On(sword, "click", [](const neon::UiElementEvent &event) { /* event.target, .x, .y */ });
ui.OnAny("changed", [](const neon::UiElementEvent &event) { /* every element */ });
ui.Off(listening);

ui.StartAnimation(sword, "shake", "0.5s linear");
```

| Group | Functions |
|---|---|
| Finding | `FindByName`, `FindByClass`, `FindByType`, `Query(selector)`, `QueryFirst`, `Matches`, `GetRoot`, `GetParent`, `GetChildren`, `GetNextSibling`, `GetPreviousSibling`, `GetElementType`, `GetElementName`, `IsAlive` |
| Styling | `Set`, `GetComputed`, `AddClass`, `RemoveClass`, `ToggleClass`, `HasClass`, `GetClasses`, `ReloadStyles` |
| Fields | `SetField`, `GetField`, `SetElementText`, `GetElementText`, `SetVisible`, `IsVisible`, and `style.<property>` as a field |
| Focus and scrolling | `FocusElement`, `GetFocusedElement`, `Blur`, `GetScroll`, `SetScroll`, `ScrollIntoView`, `GetBox` |
| Making | `Create` from YAML, `CreateFrom` a document, `CreateFromTemplate`, `Remove`, `Move`, `SetList` |
| Events | `On`, `OnAny`, `Off`, `GetElementEvents`. An event bubbles from its target up to the root unless a listener sets `stop`, or the table of [events](#events) says it does not |
| Animation | `StartAnimation`, `StopAnimation` |
| The display | `SetUserScale`, `SetReducedMotion`, `SetTimeScale` |
| Describing | `GetElementTypeNames`, `DescribeElement(type, TypeInfo &)`, which gives every field of a kind with what it holds and what it is for, and the style as a group, through the reflection of the engine. It is what an inspector and a binding for scripts are made from |

A script in Lua has the first of it: a button names the handler of a system
it calls with `on_click`, with arguments, and `ui.set_text` and the rest set
values, see [scripting.md](scripting.md#what-the-user-interface-calls).
Elements, their styles, and listeners from code are not bound yet (#436).
What the rest of a binding for Lua might read as, from the same functions:

```lua
local sword = ui.find("sword")
sword.text = "Sword of Ada"
sword.style.background_color = "#334"
sword:add_class("dim")

for _, slot in ipairs(ui.query(".slot")) do
  slot:on("click", function(event)
    ui.set_text("selected", slot.name)
    event.stop = true
  end)
end

local row = ui.list:create_from_template("row", { name = "Bow", count = 2 })
ui.on_any("changed", function(event) sound.play("switch") end)
```

Every field a script sets goes through `SetField` and is checked as the
description says, so that a script that writes `sword.tex` or gives `rows`
a text is told what is wrong.

A native extension reaches the same `UiContext` through the interface of
extensions, in C: it shows and closes files, finds, makes, and removes
elements, sets their fields and their style from text, and listens to their
events, see
[what an extension shows on the screen](extensions.md#what-an-extension-shows-on-the-screen).

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
| `UiElement`, and the twelve kinds | neon-core | An element. What is the same for every kind is in the base class, and every kind is in `ui/elements` |
| `UiElementTypes` | neon-core | The kinds by name |
| `UiStyle`, `css-values` | neon-core | The properties, and values as CSS writes them |
| `UiProperties` | neon-core | The one table of properties: names, kinds, whether inherited, whether layout, how read and written |
| `css/css-style-sheet`, `css-selector`, `css-media`, `css-expression` | neon-core | Reading a sheet, matching selectors, media queries, and values with `var()`, units, and `calc()` |
| `UiStyleSheets`, `UiCascade`, `ui-declarations` | neon-core | The sheets of a file, what an element ends up with, and a declaration turned into a property |
| `UiAnimator`, `UiTimingFunction` | neon-core | Transitions and keyframes over time |
| `UiElementHost`, `UiDirty`, `UiDrawCache` | neon-core | What is dirty, and what was drawn last frame |
| `UiTextEditor`, `UiTextBoundaries`, `UiTextMeasure` | neon-core | Editing text, the characters of it, and where the caret is |
| `UiScrollbars` | neon-core | Where a scrollbar and its thumb are |
| `ClipboardContext`, `SDL2_Clipboard` | neon-core, neon-sdl2 | The clipboard of the system, behind an interface |
| `InputScript` | neon-core | Input from a script, for a run without a window |
| `WindowMetrics`, `CursorShape` | neon-core | Points, pixels, and density of the window, and what the cursor looks like |
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
| Noto Sans Arabic, Regular | SIL Open Font License 1.1, in `engine/fonts/noto/LICENSE.txt`. The whole font as it is published |
| The images of the gallery, the flat square | Made for the engine |

**The user interface of a game says that it uses FreeType**, which its
license asks for. The engine says so in `CREDITS.md` at the root of the repository.

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
| **Own recipes in YAML with the vocabulary of CSS, and Yoga. Chosen for now**, with the engine's own layout in place of Yoga | YAML | Own. Yoga is MIT | Yes |
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
| 300 checks of the user interface with UI recipes | Pass |
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
| 704 checks of the user interface with UI recipes and style sheets: styles, dirtiness, scrolling, scripting, animation, typing, choosing, describing, the display, navigation | Pass |
| The settings menu at 1920 by 1080, without a window, looked at | The window opens by its animation, the list, the fields, the switch, the sliders, the dropdowns, and the buttons are drawn as the theme says, the focus is on Apply |
| The settings menu at 800 by 600, at 1280 by 720 with a render scale of 2, and at 1920 by 1080 with a user scale of 1.5 | At 800 by 600 the rows go under their labels by the media query and the list scrolls with a thin bar. At the render scale of 2 the image is 2560 by 1440 and sharp. At 1.5 everything is half as large again and still fits |
| The settings menu driven by `--input`: a click into the name, typing, and a click on the dropdown | The name reads what was typed, the dropdown is open on top of the rows under it with its choice highlighted, the border of the field that has the focus is the accent colour |
| The demo scene without a user interface, 1920 by 1080, without a window, after all of this | Still SHA-256 `9244e048a6839d08` |
| 2703 tests in all, in 64 programs | Pass, 3 disabled as before |

| Frame of the user interface, measured | HUD, 11 elements | A list of 500 rows, 2003 elements |
|---|---|---|
| Nothing changed: drawn again from the cache | 4 µs | 12 µs |
| One value changed that a text shows, and the text measures the same | 6 µs | 480 µs, which is drawing 2003 elements again |
| One value changed that changes a size | 30 µs | |
| The pointer moved to another row | 7 µs | 90 µs |
| Everything laid out and drawn, as every frame was before | 210 µs | 12 600 µs |

The numbers are from `user-interface.benchmark`, built by its target name
and not a test, on the machine this was made on in a debug build. Before,
every frame was the last line. Drawing is still done as a whole when
anything changed; drawing only what changed is left for later.

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
| A controller | None was plugged in. The code compiles and follows the documentation of SDL, and the SDL2 backend is tested with a virtual joystick on the dummy driver |
| The pointer and the keys with a window | The SDL2 backend is tested with the dummy video driver of SDL, by pushing its events. A real window was not looked at |
| A display of high density | The scale and the pointer are worked out from the metrics of the window and tested with numbers. See the checklist under [points, pixels, and density](#points-pixels-and-density) |
| An input method | Composition is tested through the events SDL gives. Typing Japanese with a real input method was not tried |
| The clipboard of the system | `SDL2_Clipboard` is tested on the dummy driver, which keeps its own text |

## Left for later

| | |
|---|---|
| Yoga | See [layout](#layout) |
| More elements | Tabs, a tree, a table, a colour picker |
| Drawing only what changed | A frame draws everything again when anything changed. The cache replays it when nothing did |
| Shaping | `UiTextMeasure` is the seam. Kerning, ligatures, and right to left are the work of a shaper behind it |
| Localisation | Text by key, and fonts for other scripts |
| Rich text | Several styles in one text |
| Surfaces that are drawn when what is on them changed | A surface is drawn when anything in the user interface changed (#431). Drawing only the surfaces whose own user interfaces changed needs every change to say which document it belongs to |
| A ray that finds where the player points on a screen in the world | Done, without the physics: the ray meets the square of the entity. See [pointing](#pointing-at-a-screen-in-the-world) |
| Atlases of TexturePacker and Aseprite | See [atlases](#atlases) |
| `filter` and `backdrop_filter` | Both need what is behind an element, or the element itself, as an image of its own. Render targets are what they would be built from |
| Opacity of an element with what is inside it as one picture | The same |
| Fonts for other scripts in the runtime | Noto Sans Arabic is there to show that it works. A game brings the fonts of the languages it is translated into |
| Writing a file | `UiFile` reads. The editor will write |
| Watching files | `ReloadStyles()` reads sheets again when the game asks. Nothing watches the disk |
| The clipboard for more than text | Images and files |
| A scrollbar with buttons, and `scrollbar-gutter` | The bar is a track and a thumb |
| Roles for accessibility | Named as in WAI-ARIA |

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
- **SVG** for images and icons that stay sharp at every scale.
- **Roles for accessibility**, named as in WAI-ARIA, so that a screen reader
  can say what an element is.
- **RmlUi**, as the alternative of taking over a whole stack of standards in
  place of recipes.

Others:

- Whether Yoga replaces the layout engine of the engine, or stands next to it.
- Whether values should have a scope, such as one set for each player in a
  game on a split screen.
- Whether the focus should wrap around at the edge.
- Image decoding for models is in neon-vulkan. For the user interface it is
  behind `ImageDecoder`, which models could use as well.
- Whether what a file of the window starts its values with should be the
  values of that file, as it is on every other surface.
- Whether a surface in the world should be lit by the scene, or glow by
  itself as the demo does with the shader `unlit`.
- Whether media queries should see the file in its units in place of the
  window in points, so that a sheet is written in one measure throughout.
- Whether `scroll_drag` should be on by default where there is a touch
  screen, which the engine cannot tell yet.
