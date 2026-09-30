#include "tree-ui-system.hpp"

#include <algorithm>
#include <cmath>

#include <neon/text/utf8.hpp>

// What moves by itself: transitions, animations, what is shown next to the
// pointer, and the rows of a list.

namespace neon
{
  UiAnimator::Context Tree_UiSystem::AnimationContext()
  {
    return [this](const UiElement &element)
    {
      const UiDocument *document = DocumentOf(&element);
      return UiCascade::ContextOf(element, document != nullptr ? CascadeOf(*document) : UiCascadeSettings{});
    };
  }

  void Tree_UiSystem::Animate(const float seconds)
  {
    if (_animator.GetMovingCount() == 0 && seconds <= 0.0f) { return; }

    std::vector<UiAnimator::Ended> ended;

    _animator.Advance(
      seconds,
      [this](const std::uint64_t id) { return ElementOf(UiHandle{id}); },
      [this](const UiElement &element) -> const UiStyleSheets *
      {
        const UiDocument *document = DocumentOf(&element);
        return document != nullptr ? &document->sheets : nullptr;
      },
      AnimationContext(),
      ended);

    for (const auto &[id, name, is_transition] : ended)
    {
      UiElement *element = ElementOf(UiHandle{id});
      if (element == nullptr) { continue; }

      UiElementEvent event;
      event.value = name;
      Emit(*element, is_transition ? "transition_ended" : "animation_ended", event);
    }
  }

  bool Tree_UiSystem::StartAnimation(const UiHandle element, const std::string &name, const std::string &options)
  {
    UiElement *found = ElementOf(element);
    if (found == nullptr) { return false; }

    const UiDocument *document = DocumentOf(found);
    if (document == nullptr || document->sheets.FindKeyframes(name) == nullptr)
    {
      const std::string what = found->Describe();
      _logger->Warn("There are no keyframes '{}' for {}, and nothing is started", name, what);
      return false;
    }

    // what follows the name, read as `animation` of CSS reads it
    DataValue map = DataValue::Map();
    map.Set("animation", DataValue::Text(name + " " + options));

    std::vector<std::string> errors;
    const DataReader reader(map, "", found->Describe(), errors);

    UiStyle style;
    ReadUiBehaviourStyle(reader, style);

    if (!errors.empty() || style.animations.names.size() != 1)
    {
      _logger->Warn(
        "'{}' cannot be read as what follows the name of an animation, such as \"0.3s ease-out both\". "
        "Nothing is started",
        options);
      return false;
    }

    UiAnimator::Options read;
    read.duration = style.animations.durations[0];
    read.delay = style.animations.delays[0];
    read.iteration_count = style.animations.iteration_counts[0];
    read.direction = style.animations.directions[0];
    read.fill_mode = style.animations.fill_modes[0];
    read.play_state = style.animations.play_states[0];
    read.timing_function = style.animations.timing_functions[0];

    (void) found->GetStyle();

    _animator.StartByHand(*found, name, read);
    _animator.Apply(*found, &document->sheets, AnimationContext());
    return true;
  }

  bool Tree_UiSystem::StopAnimation(const UiHandle element, const std::string &name)
  {
    UiElement *found = ElementOf(element);
    if (found == nullptr) { return false; }

    if (!_animator.StopByHand(*found, name)) { return false; }

    const UiDocument *document = DocumentOf(found);
    _animator.Apply(*found, document != nullptr ? &document->sheets : nullptr, AnimationContext());
    return true;
  }

  std::size_t Tree_UiSystem::GetMovingCount() const
  {
    return _animator.GetMovingCount();
  }

  void Tree_UiSystem::PaintTooltip()
  {
    if (!_tooltip.is_shown || _tooltip.text.empty()) { return; }

    const UiElement *element = ElementOf(UiHandle{_tooltip.element});
    const UiDocument *document = DocumentOf(element);
    if (element == nullptr || document == nullptr) { return; }

    // what a style sheet reaches as `::tooltip` of the element
    const UiStyle &style = element->GetPartStyle("tooltip");
    const float scale = document->frame.scale;

    const int pixel_size = std::max(1, static_cast<int>(std::round(style.font_size * scale)));
    const UiFont *font = _resources.GetFont(style.font_family, style.font_weight, pixel_size);
    if (font == nullptr) { return; }

    const auto [frame_width, frame_height] = _renderer->GetRenderResolution();

    const float padding_x = std::round(8.0f * scale);
    const float padding_y = std::round(4.0f * scale);

    TextOptions options;
    options.max_width = std::round(320.0f * scale);
    options.line_height = style.LineHeight() * scale;

    const PlacedText placed = PlaceText(font->atlas, DecodeUtf8(_tooltip.text), options);

    // below the pointer and to its right, and inside of what is shown
    UiRectangle box;
    box.left = std::round(_tooltip.x + 12.0f * scale);
    box.top = std::round(_tooltip.y + 20.0f * scale);
    box.right = box.left + placed.width + 2.0f * padding_x;
    box.bottom = box.top + placed.height + 2.0f * padding_y;

    if (box.right > static_cast<float>(frame_width))
    {
      const float by = box.right - static_cast<float>(frame_width);
      box.left = std::max(0.0f, box.left - by);
      box.right -= by;
    }

    if (box.bottom > static_cast<float>(frame_height))
    {
      // above the pointer, where there is no room below it
      const float height = box.Height();
      box.top = std::max(0.0f, std::round(_tooltip.y - 8.0f * scale) - height);
      box.bottom = box.top + height;
    }

    const Color &background = style.background_color;
    _painter.FillRectangle(box, {background.r, background.g, background.b, background.a * style.opacity});

    const float border = std::round(std::max(0.0f, style.layout.border.top) * scale);
    if (border > 0.0f)
    {
      const Color line = style.BorderColor();
      _painter.FillBorder(box, {border, border, border, border}, {line.r, line.g, line.b, line.a * style.opacity});
    }

    _painter.DrawText(
      *font, placed, box.left + padding_x, box.top + padding_y,
      {style.color.r, style.color.g, style.color.b, style.color.a * style.opacity});
  }

  void Tree_UiSystem::FollowLists(UiDocument &document)
  {
    for (auto &repeater : document.repeaters)
    {
      UiElement *element = ElementOf(UiHandle{repeater.element});
      if (element == nullptr) { continue; }

      const auto found = _lists.find(repeater.list);
      const std::uint64_t revision = found != _lists.end() ? found->second.revision : 0;

      if (repeater.is_made && repeater.revision == revision) { continue; }

      repeater.is_made = true;
      repeater.revision = revision;

      // The rows are made anew. What a row showed is gone with it, which
      // includes where it was scrolled to and what had the focus in it.
      while (!element->GetChildren().empty())
      {
        (void) element->RemoveChild(*element->GetChildren().back(), _layout);
      }

      if (found == _lists.end()) { continue; }

      std::size_t index = 0;
      for (const UiRow &row : found->second.rows)
      {
        UiRow fields = row;

        // where the row is in its list, counted from 0 and from 1
        fields.try_emplace("index", std::to_string(index));
        fields.try_emplace("number", std::to_string(index + 1));

        (void) CreateFromTemplate(repeater.template_name, HandleOf(element), fields);
        index++;
      }
    }
  }
} // neon
