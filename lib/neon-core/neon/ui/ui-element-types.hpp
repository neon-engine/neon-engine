#ifndef UI_ELEMENT_TYPES_HPP
#define UI_ELEMENT_TYPES_HPP

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ui-element.hpp"

namespace neon
{
  /// The kinds of elements a file can hold, by the name it writes as
  /// `type`. Those of the engine are known. A game adds its own:
  ///
  ///     types.Add<Minimap>("minimap");
  class UiElementTypes
  {
  public:
    using Create = std::function<std::unique_ptr<UiElement>()>;

  private:
    // in the order they were added, which is the order messages list them in
    std::vector<std::pair<std::string, Create>> _types;

  public:
    /// Adds a kind, or replaces the one of the same name.
    void Add(const std::string &name, const Create &create);

    template<typename T>
    void Add(const std::string &name)
    {
      Add(name, [] { return std::make_unique<T>(); });
    }

    /// A new element of the kind, or nullptr when the name is not known.
    [[nodiscard]] std::unique_ptr<UiElement> CreateElement(const std::string &name) const;

    [[nodiscard]] std::vector<std::string> GetNames() const;

    /// Adds the kinds of the engine: input, textarea, checkbox, radio,
    /// toggle, slider, select, panel, label, image, button, and bar.
    void AddEngineElements();
  };
} // neon

#endif //UI_ELEMENT_TYPES_HPP
