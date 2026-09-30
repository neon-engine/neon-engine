#include "ui-element-types.hpp"

#include "elements/ui-bar.hpp"
#include "elements/ui-button.hpp"
#include "elements/ui-image.hpp"
#include "elements/ui-label.hpp"
#include "elements/ui-panel.hpp"

namespace neon
{
  void UiElementTypes::Add(const std::string &name, const Create &create)
  {
    for (auto &[known, known_create] : _types)
    {
      if (known == name)
      {
        known_create = create;
        return;
      }
    }

    _types.emplace_back(name, create);
  }

  std::unique_ptr<UiElement> UiElementTypes::CreateElement(const std::string &name) const
  {
    for (const auto &[known, create] : _types)
    {
      if (known == name) { return create(); }
    }
    return nullptr;
  }

  std::vector<std::string> UiElementTypes::GetNames() const
  {
    std::vector<std::string> names;
    for (const auto &[name, create] : _types) { names.push_back(name); }
    return names;
  }

  void UiElementTypes::AddEngineElements()
  {
    Add<UiPanel>(UiPanel::kType);
    Add<UiLabel>(UiLabel::kType);
    Add<UiImageElement>(UiImageElement::kType);
    Add<UiButton>(UiButton::kType);
    Add<UiBar>(UiBar::kType);
  }
} // neon
