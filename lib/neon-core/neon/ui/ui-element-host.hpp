#ifndef UI_ELEMENT_HOST_HPP
#define UI_ELEMENT_HOST_HPP

#include <string>

namespace neon
{
  class UiElement;
  struct UiStates;
  struct UiStyle;

  /// What about an element is no longer what it was, and has to be worked
  /// out again. Several are added up.
  namespace UiDirty
  {
    /// What the element looks like: it is drawn again.
    constexpr unsigned Paint = 1u;

    /// Where what is inside the element goes, such as after it was
    /// scrolled. Nothing is measured again.
    constexpr unsigned Arrange = 2u;

    /// How large the element is and where it goes.
    constexpr unsigned Layout = 4u;

    /// The style of the element, which the cascade gives it.
    constexpr unsigned Style = 8u;

    /// The element moves by itself, as a caret that blinks does: its
    /// Tick() is called in every frame until it says that it rests.
    constexpr unsigned Tick = 16u;

    /// The element has something to tell the game, which Notify() left
    /// with it.
    constexpr unsigned Notice = 32u;
  }

  /// What a tree of elements belongs to: the user interface that gives the
  /// elements their styles and that is told when one of them changed.
  ///
  /// An element without one works out its style from what its file writes
  /// for it alone, and keeps what changed to itself.
  class UiElementHost
  {
  protected:
    ~UiElementHost() = default;

  public:
    /// Works out the style of an element, from the style sheets and from
    /// what is written for it, and hands it to the element.
    virtual void ComputeStyle(UiElement &element) = 0;

    /// Works out the style of a part of an element, such as the thumb of
    /// its scrollbar, which a style sheet reaches as `::scrollbar-thumb`.
    /// `style` holds what the element gives the part to start with.
    virtual void ComputePartStyle(const UiElement &element, const std::string &part, UiStyle &style) = 0;

    /// Called when something about an element is no longer what it was.
    virtual void Invalidated(UiElement &element, unsigned what) = 0;

    /// Called when the states of an element changed, which may change the
    /// styles of other elements: those a style sheet reaches through it.
    virtual void StatesChanged(UiElement &element, const UiStates &before, const UiStates &after) = 0;

    /// Called when the classes or the fields of an element changed.
    virtual void ClassesChanged(UiElement &element) = 0;

    /// Called in front of an element leaving the tree, with everything
    /// inside it. Nothing may keep pointing at it afterwards.
    virtual void Leaving(UiElement &element) = 0;

    /// Called behind an element joining the tree.
    virtual void Joined(UiElement &element) = 0;
  };
} // neon

#endif //UI_ELEMENT_HOST_HPP
