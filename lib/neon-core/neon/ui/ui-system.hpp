#ifndef UI_SYSTEM_HPP
#define UI_SYSTEM_HPP

#include <neon/input/input-context.hpp>

#include "ui-context.hpp"

namespace neon
{
  /// The user interface of a game as the runtime sees it. A frame calls
  /// Update() after the input was read and before the world is updated,
  /// and Draw() after the world was drawn and before the frame is
  /// finished.
  class UiSystem : public UiContext
  {
  protected:
    ~UiSystem() = default;

  public:
    virtual void Initialize() = 0;

    /// Reacts to the input, and decides what of it is left for the game.
    virtual void Update() = 0;

    /// Draws on top of what the frame holds.
    virtual void Draw() = 0;

    virtual void CleanUp() = 0;

    /// The input as the game is to see it: without what the user
    /// interface has used. The world is handed this in place of the input
    /// system.
    [[nodiscard]] virtual InputContext *GetGameInput() = 0;
  };
} // neon

#endif //UI_SYSTEM_HPP
