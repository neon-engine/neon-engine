#ifndef INPUT_CONTEXT_HPP
#define INPUT_CONTEXT_HPP

#include "input-state.hpp"

namespace neon
{
  /// Where a text is typed, in pixels of what is drawn to. An input method
  /// shows its candidates next to it.
  struct TextInputArea
  {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool operator==(const TextInputArea &other) const = default;
  };

  class InputContext
  {
  protected:

    ~InputContext() = default;

  public:
    virtual const InputState& GetInputState() = 0;

    virtual void CenterAndHideCursor() = 0;

    virtual void ShowCursor() = 0;

    /// Says that a text is typed from now on, and where its caret is. The
    /// platform then hands over text and not only keys, lets an input
    /// method put characters together, and shows a keyboard on the screen
    /// where it has one. Called again when the caret moved. An input
    /// without a keyboard does nothing.
    virtual void StartTextInput(const TextInputArea &caret) {}

    /// Says that no text is typed any more.
    virtual void StopTextInput() {}
  };
} // neon

#endif //INPUT_CONTEXT_HPP
