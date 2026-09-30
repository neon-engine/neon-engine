#ifndef UI_DOCUMENT_HPP
#define UI_DOCUMENT_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ui-element.hpp"
#include "ui-style-sheets.hpp"
#include "ui-values.hpp"

namespace neon
{
  /// `scale` of a file: what the units of the file grow with.
  enum class UiScaleMode
  {
    /// With the width or the height of the frame, whichever leaves room
    /// for everything the file was made for.
    Fit = 0,
    Width,
    Height,
    /// Not at all. A unit is a pixel.
    None
  };

  /// `cancel` of a file: what cancel does while the file takes the input.
  enum class UiCancel
  {
    /// What suits the file: a file that is modal is closed, and any other
    /// gives up the focus.
    Auto = 0,

    /// The file is no longer shown, and the focus returns to where it was
    /// when the file was shown.
    Close,

    /// The focus leaves the file, which stays.
    Blur,

    /// Nothing. The game is told, as it is in every case, and decides.
    None
  };

  /// A font a file says it draws with, as `@font-face` of CSS does.
  struct UiFontFace
  {
    std::string family;
    int weight = 400;
    std::string path;

    /// `style: italic` and `rendering: sdf` of the font.
    UiFaceOptions options;
  };

  /// A user interface as a file holds it: a tree of elements, and what is
  /// written at the top of the file.
  struct UiDocument
  {
    int id = -1;

    /// The virtual path the document was read from, and what it calls
    /// itself.
    std::string path;
    std::string name;

    /// A document that is modal takes the input away from the game and
    /// from the documents below it, as a modal `dialog` of HTML does.
    bool modal = false;

    /// The size of the frame the file was made for, in its units.
    float reference_width = 1920.0f;
    float reference_height = 1080.0f;
    UiScaleMode scale_mode = UiScaleMode::Fit;

    std::vector<UiFontFace> fonts;

    /// What the values are until the game sets them.
    std::vector<std::pair<std::string, UiValue>> values;

    std::unique_ptr<UiElement> root;

    /// What the elements measure and draw themselves with. It lives here
    /// because the elements keep referring to it.
    UiFrame frame;

    /// Pixels of the frame for each unit of the file, at a size of the
    /// frame.
    [[nodiscard]] float ScaleFor(int frame_width, int frame_height) const;

    /// The surface the document is shown on. 0 is the window.
    int surface = 0;
    /// Pixels of the frame for each unit of the file, for a window of a
    /// size in points, on a display of a density, with what the player
    /// asked the user interface to be made larger by:
    ///
    ///     scale = density * user scale * what the file asks for
    ///
    /// What the file asks for is 1 for `scale: none`, and otherwise how
    /// much larger the window is than the size the file was made for.
    [[nodiscard]] float ScaleFor(float point_width, float point_height, float density, float user_scale) const;

    /// The style sheets the file names under `styles`, as it writes them,
    /// and what they hold.
    std::vector<std::string> style_paths;
    UiStyleSheets sheets;

    /// What the file defines under `templates`: elements that are not
    /// shown, and that copies are made of while the game runs, such as the
    /// rows of a list.
    std::vector<std::pair<std::string, DataValue>> templates;

    [[nodiscard]] const DataValue *FindTemplate(const std::string &template_name) const;

    /// What the sheets said that could not be read, which does not keep
    /// the file from being shown.
    std::vector<std::string> warnings;

    // What has to be worked out again, which the user interface keeps
    // track of.

    std::vector<UiElement *> dirty_styles;
    std::vector<UiElement *> dirty_layouts;
    std::vector<UiElement *> dirty_places;
    std::vector<UiElement *> ticking;
    bool needs_paint = true;
    bool needs_full_layout = true;

    /// What the elements were last told of the values of the game, and of
    /// the size of what is shown.
    std::uint64_t followed_revision = 0;
    bool has_followed = false;
    float laid_out_width = 0.0f;
    float laid_out_height = 0.0f;
    float laid_out_scale = 0.0f;

    UiCancel cancel = UiCancel::Auto;

    /// The elements that show a list of the game, each with the name of
    /// the list, the template its rows are made from, and the number the
    /// list had when the rows were made.
    struct Repeater
    {
      std::uint64_t element = 0;
      std::string list;
      std::string template_name;
      std::uint64_t revision = 0;
      bool is_made = false;
    };

    std::vector<Repeater> repeaters;

    /// Where the focus was when another file took the input, to which it
    /// returns when that file is gone. 0 for none.
    std::uint64_t remembered_focus = 0;
  };
} // neon

#endif //UI_DOCUMENT_HPP
