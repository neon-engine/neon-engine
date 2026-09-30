#ifndef UI_DOCUMENT_HPP
#define UI_DOCUMENT_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ui-element.hpp"
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
  };
} // neon

#endif //UI_DOCUMENT_HPP
