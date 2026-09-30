#include "tree-ui-system.hpp"

#include <algorithm>
#include <cmath>

// What the user interface does for its elements: it gives them their
// styles, keeps track of what changed, and works out again what depends on
// it, and nothing else.

namespace neon
{
  namespace
  {
    std::size_t DepthOf(const UiElement *element)
    {
      std::size_t depth = 0;
      for (const UiElement *above = element->GetParent(); above != nullptr; above = above->GetParent()) { depth++; }
      return depth;
    }

    bool IsInside(const UiElement *element, const UiElement *other)
    {
      for (const UiElement *above = element->GetParent(); above != nullptr; above = above->GetParent())
      {
        if (above == other) { return true; }
      }
      return false;
    }

    bool is_gone_inside(const UiElement *element, const UiElement *gone)
    {
      return element != nullptr && (element == gone || IsInside(element, gone));
    }

    std::size_t CountOf(const UiElement &element)
    {
      std::size_t count = 1;
      for (const auto &child : element.GetChildren()) { count += CountOf(*child); }
      return count;
    }

    void Forget(std::vector<UiElement *> &elements, const UiElement *gone)
    {
      std::erase_if(elements, [gone](const UiElement *each) { return each == gone || IsInside(each, gone); });
    }

    /// The names of the states that are not what they were.
    std::vector<std::string> Changed(const UiStates &before, const UiStates &after)
    {
      std::vector<std::string> names;

      if (before.hover != after.hover) { names.emplace_back("hover"); }
      if (before.active != after.active) { names.emplace_back("active"); }
      if (before.focus != after.focus) { names.emplace_back("focus"); }
      if (before.disabled != after.disabled) { names.emplace_back("disabled"); }
      if (before.checked != after.checked) { names.emplace_back("checked"); }

      if (before.invalid != after.invalid)
      {
        names.emplace_back("invalid");
        names.emplace_back("valid");
      }

      if ((before.focus_within || before.focus) != (after.focus_within || after.focus))
      {
        names.emplace_back("focus-within");
      }

      return names;
    }
  }

  void Tree_UiSystem::SetWindow(WindowContext *window)
  {
    _window = window;
    _needs_paint = true;
  }

  void Tree_UiSystem::SetClipboard(ClipboardContext *clipboard)
  {
    _clipboard = clipboard != nullptr ? clipboard : &_own_clipboard;
  }

  void Tree_UiSystem::Advance(const double seconds)
  {
    _advance = std::max(0.0, seconds);
    _was_advanced = true;
  }

  bool Tree_UiSystem::ReloadStyles(const int document)
  {
    bool is_read = true;

    for (const auto &each : _documents)
    {
      if (document >= 0 && each->id != document) { continue; }

      const std::string path = each->path;
      _logger->Info("Reading the style sheets of {} again", path);

      std::vector<std::string> errors;
      if (!_file.ReloadStyles(*each, errors))
      {
        for (const auto &error : errors) { _logger->Error("{}", error); }
        _logger->Error("The style sheets of {} are left as they were", path);

        is_read = false;
        continue;
      }

      for (const auto &warning : each->warnings) { _logger->Warn("{}", warning); }
      for (const auto &font : each->sheets.GetFonts()) { _resources.AddFace(font.family, font.weight, font.path); }

      // what was said about the sheets before is said again when it is
      // still the case
      _reported.clear();

      each->root->InvalidateAll(UiDirty::Style | UiDirty::Paint);
    }

    return is_read;
  }

  void Tree_UiSystem::SetUserScale(const float scale)
  {
    _user_scale = scale > 0.0f ? scale : 1.0f;
  }

  float Tree_UiSystem::GetUserScale() const
  {
    return _user_scale;
  }

  void Tree_UiSystem::SetReducedMotion(const bool reduced)
  {
    _reduced_motion = reduced;
  }

  bool Tree_UiSystem::GetReducedMotion() const
  {
    return _reduced_motion;
  }

  void Tree_UiSystem::SetTimeScale(const double scale)
  {
    _time_scale = std::max(0.0, scale);
  }

  double Tree_UiSystem::GetTimeScale() const
  {
    return _time_scale;
  }

  const UiStatistics &Tree_UiSystem::GetStatistics() const
  {
    return _statistics;
  }

  const std::vector<std::unique_ptr<UiDocument>> &Tree_UiSystem::GetDocuments() const
  {
    return _documents;
  }

  float Tree_UiSystem::GetScale() const
  {
    return _documents.empty() ? 1.0f : _documents.back()->frame.scale;
  }

  std::size_t Tree_UiSystem::GetFontCount() const
  {
    return _resources.GetFontCount();
  }

  WindowMetrics Tree_UiSystem::GetMetrics() const
  {
    const auto [width, height] = _renderer->GetRenderResolution();

    WindowMetrics metrics{width, height, width, height};

    if (_window != nullptr)
    {
      // What is drawn to is what the renderer says it is. The window says
      // how many points that is.
      const WindowSize points = _window->GetWindowSize();
      if (points.width > 0 && points.height > 0)
      {
        metrics.point_width = points.width;
        metrics.point_height = points.height;
      }
    }

    return metrics;
  }

  UiCascadeSettings Tree_UiSystem::CascadeOf(const UiDocument &document)
  {
    UiCascadeSettings settings;
    settings.sheets = &document.sheets;
    settings.viewport_width = document.frame.width;
    settings.viewport_height = document.frame.height;
    settings.problems = &_problems;
    return settings;
  }

  void Tree_UiSystem::ComputeStyle(UiElement &element)
  {
    const UiDocument *document = DocumentOf(&element);

    // what the element had, which what it gets is a change of
    const bool had_style = element.HasComputedStyle();
    const UiStyle before = had_style ? element.GetComputedStyle() : UiStyle{};

    _statistics.styles++;
    UiCascade::Compute(element, document != nullptr ? CascadeOf(*document) : UiCascadeSettings{});

    _animator.StyleChanged(element, before, element.GetComputedStyle(), had_style);
    _animator.Apply(element, document != nullptr ? &document->sheets : nullptr, AnimationContext());
  }

  void Tree_UiSystem::ComputePartStyle(const UiElement &element, const std::string &part, UiStyle &style)
  {
    const UiDocument *document = DocumentOf(&element);
    if (document == nullptr) { return; }

    UiCascade::ComputePart(element, part, CascadeOf(*document), style);
  }

  void Tree_UiSystem::ReportProblems()
  {
    for (const auto &problem : _problems)
    {
      // a style is worked out again and again, and says the same each time
      if (_reported.insert(problem).second) { _logger->Warn("{}", problem); }
    }
    _problems.clear();
  }

  void Tree_UiSystem::Invalidated(UiElement &element, const unsigned what)
  {
    UiDocument *document = DocumentOf(&element);
    if (document == nullptr) { return; }

    if ((what & UiDirty::Style) != 0) { document->dirty_styles.push_back(&element); }
    if ((what & UiDirty::Layout) != 0) { document->dirty_layouts.push_back(&element); }
    if ((what & UiDirty::Arrange) != 0) { document->dirty_places.push_back(&element); }

    if ((what & UiDirty::Notice) != 0 && std::ranges::find(_noticing, &element) == _noticing.end())
    {
      _noticing.push_back(&element);
    }

    if ((what & UiDirty::Tick) != 0 &&
        std::ranges::find(document->ticking, &element) == document->ticking.end())
    {
      document->ticking.push_back(&element);
    }

    if ((what & (UiDirty::Paint | UiDirty::Layout | UiDirty::Arrange)) != 0)
    {
      document->needs_paint = true;
      _needs_paint = true;
    }
  }

  void Tree_UiSystem::StatesChanged(UiElement &element, const UiStates &before, const UiStates &after)
  {
    const UiDocument *document = DocumentOf(&element);
    if (document == nullptr || document->sheets.IsEmpty()) { return; }

    const CssDependencies &dependencies = document->sheets.GetDependencies();

    bool reaches_inside = false;
    bool reaches_behind = false;

    for (const auto &name : Changed(before, after))
    {
      reaches_inside = reaches_inside || dependencies.HasStateAbove(name);
      reaches_behind = reaches_behind || dependencies.HasStateBefore(name);
    }

    if (reaches_inside)
    {
      for (const auto &child : element.GetChildren()) { child->InvalidateAll(UiDirty::Style); }
    }

    if (reaches_behind)
    {
      for (UiElement *behind = element.GetNextSibling(); behind != nullptr; behind = behind->GetNextSibling())
      {
        behind->InvalidateAll(UiDirty::Style);
      }
    }
  }

  void Tree_UiSystem::ClassesChanged(UiElement &element)
  {
    const UiDocument *document = DocumentOf(&element);
    if (document == nullptr || document->sheets.IsEmpty()) { return; }

    const CssDependencies &dependencies = document->sheets.GetDependencies();

    if (dependencies.classes_above)
    {
      for (const auto &child : element.GetChildren()) { child->InvalidateAll(UiDirty::Style); }
    }

    if (dependencies.classes_before)
    {
      for (UiElement *behind = element.GetNextSibling(); behind != nullptr; behind = behind->GetNextSibling())
      {
        behind->InvalidateAll(UiDirty::Style);
      }
    }
  }

  void Tree_UiSystem::Joined(UiElement &element)
  {
    std::vector<UiElement *> elements;
    CollectUiElements(element, elements);

    for (UiElement *each : elements)
    {
      if (each->GetId() == 0) { each->SetId(_next_element_id++); }

      _elements[each->GetId()] = each;
      if (each->PaintsEveryFrame()) { _always_painting++; }
    }

    UiDocument *document = DocumentOf(&element);
    if (document == nullptr) { return; }

    // what joined has not followed the values of the game yet
    document->has_followed = false;

    // the elements that show a list of the game
    for (const UiElement *each : elements)
    {
      std::string list;
      std::string template_name;

      const DataValue *written = each->GetWritten().Find("for_each");
      const DataValue *made_from = each->GetWritten().Find("template");

      if (written == nullptr || made_from == nullptr || !written->GetText(list) ||
          !made_from->GetText(template_name) || list.size() < 3)
      {
        continue;
      }

      const std::uint64_t id = each->GetId();
      const bool is_known = std::ranges::any_of(document->repeaters, [id](const auto &repeater)
      {
        return repeater.element == id;
      });

      if (!is_known) { document->repeaters.push_back({id, list.substr(1, list.size() - 2), template_name, 0, false}); }
    }

    // where an element is among those next to it may decide its style
    if (element.GetParent() != nullptr && document->sheets.GetDependencies().structure)
    {
      for (const auto &sibling : element.GetParent()->GetChildren()) { sibling->InvalidateAll(UiDirty::Style); }
    }

    element.InvalidateAll(UiDirty::Style | UiDirty::Layout | UiDirty::Paint);
  }

  void Tree_UiSystem::Leaving(UiElement &element)
  {
    std::vector<UiElement *> elements;
    CollectUiElements(element, elements);

    for (const UiElement *each : elements)
    {
      _elements.erase(each->GetId());
      _animator.Forget(each->GetId());

      if (each->PaintsEveryFrame() && _always_painting > 0) { _always_painting--; }
    }

    if (is_gone_inside(_captured, &element)) { _captured = nullptr; }
    Forget(_noticing, &element);

    const auto is_gone = [&element](const UiElement *each)
    {
      return each != nullptr && (each == &element || IsInside(each, &element));
    };

    if (is_gone(_focused)) { _focused = nullptr; }
    if (is_gone(_pressed)) { _pressed = nullptr; }
    if (is_gone(_hovered)) { _hovered = nullptr; }

    Forget(_in_state, &element);

    if (UiDocument *document = DocumentOf(&element); document != nullptr)
    {
      Forget(document->dirty_styles, &element);
      Forget(document->dirty_layouts, &element);
      Forget(document->dirty_places, &element);
      Forget(document->ticking, &element);

      std::erase_if(document->repeaters, [this](const auto &repeater)
      {
        return !_elements.contains(repeater.element);
      });

      document->needs_paint = true;

      if (element.GetParent() != nullptr)
      {
        // what is left is placed again, and may have another style by
        // where it is now
        element.GetParent()->Invalidate(UiDirty::Layout | UiDirty::Paint);

        if (document->sheets.GetDependencies().structure)
        {
          for (const auto &sibling : element.GetParent()->GetChildren())
          {
            if (sibling.get() != &element) { sibling->InvalidateAll(UiDirty::Style); }
          }
        }
      }
    }

    _needs_paint = true;
  }

  void Tree_UiSystem::Adopt(UiDocument &document)
  {
    document.frame.resources = &_resources;

    // A user interface that has a name has values of its own, which fall
    // back on those every user interface shares.
    document.frame.values = document.name.empty() ? &_values : &ValuesOf(document.name);

    for (const auto &face : document.fonts)
    {
      _resources.AddFace(face.family, face.weight, face.path, face.options);
    }
    for (const auto &font : document.sheets.GetFonts()) { _resources.AddFace(font.family, font.weight, font.path); }

    // What a file starts its values with is shared on the window, where
    // two files show the same health. A surface in the world keeps it to
    // itself: what is shown there knows nothing of what is shown
    // elsewhere. Neither replaces what the game has set there.
    UiValues &defaults = document.surface == Ui_Window_Surface || document.name.empty()
      ? _values
      : ValuesOf(document.name);

    for (const auto &[name, value] : document.values) { defaults.SetDefault(name, value); }

    for (const auto &warning : document.warnings) { _logger->Warn("{}", warning); }

    document.root->SetHost(this);
    document.root->CreateLayout(*_layout, &document.frame);
  }

  void Tree_UiSystem::Settle()
  {
    const auto [width, height] = _renderer->GetRenderResolution();
    const WindowMetrics metrics = GetMetrics();
    const auto density = static_cast<float>(metrics.Density());

    bool scale_changed = false;

    for (const auto &document : _documents)
    {
      const Surface *surface = FindSurface(document->surface);
      if (surface == nullptr) { continue; }

      // The window is seen in points on a display of a density. A surface
      // in the world is an image, whose pixels are its points.
      const bool on_window = surface->id == Ui_Window_Surface;

      int surface_width = width;
      int surface_height = height;
      if (!on_window) { SizeOf(*surface, surface_width, surface_height); }

      const float point_width = on_window
        ? static_cast<float>(metrics.point_width)
        : static_cast<float>(surface_width);
      const float point_height = on_window
        ? static_cast<float>(metrics.point_height)
        : static_cast<float>(surface_height);
      const float surface_density = on_window ? density : 1.0f;

      const float scale = document->ScaleFor(point_width, point_height, surface_density, _user_scale) *
                          surface->scale;

      const float frame_width = static_cast<float>(surface_width) / scale;
      const float frame_height = static_cast<float>(surface_height) / scale;

      if (document->laid_out_scale != scale && document->laid_out_scale != 0.0f) { scale_changed = true; }

      if (document->laid_out_scale != scale || document->laid_out_width != frame_width ||
          document->laid_out_height != frame_height)
      {
        document->frame.scale = scale;
        document->frame.width = frame_width;
        document->frame.height = frame_height;

        document->laid_out_scale = scale;
        document->laid_out_width = frame_width;
        document->laid_out_height = frame_height;

        // Everything may depend on the size of what is shown: the units
        // of the viewport, and what text measures at the scale.
        document->root->InvalidateAll(UiDirty::Style | UiDirty::Layout | UiDirty::Paint);
        document->needs_full_layout = true;
      }

      const CssEnvironment environment{point_width, point_height, surface_density, _reduced_motion};

      if (!(document->sheets.GetEnvironment() == environment) && document->sheets.SetEnvironment(environment))
      {
        document->root->InvalidateAll(UiDirty::Style);
      }
    }

    // Text is drawn at other sizes from now on. The sizes it was drawn
    // at before are given up, and not kept until the end.
    if (scale_changed)
    {
      _resources.ReleaseFonts();
      _draw_cache.Clear();
      _needs_paint = true;
    }

    for (const auto &document : _documents)
    {
      document->frame.time = _time;

      // the values of the user interface, which count those all share
      const std::uint64_t revision = document->frame.values->GetRevision();

      if (!document->has_followed || document->followed_revision != revision)
      {
        document->has_followed = true;
        document->followed_revision = revision;

        _statistics.follows++;
        document->root->Follow(document->frame, false);
      } else
      {
        // what changed by itself, such as a text a script set, is brought
        // up to date on its own
        for (UiElement *element : document->dirty_layouts) { element->Update(document->frame); }
      }

      FollowLists(*document);

      // a style may change where something is, and where something is
      // may not change a style
      SettleStyles(*document);
      SettleLayout(*document);
    }

    ReportProblems();

    for (const auto &name : _values.TakeMissed())
    {
      _logger->Warn("The value '{}' is not set. What refers to it shows its name, or its default", name);
    }
  }

  void Tree_UiSystem::SettleStyles(UiDocument &document)
  {
    // what is worked out may ask for more to be worked out: what is
    // inside an element, when what it inherits changed
    for (int round = 0; round < 64 && !document.dirty_styles.empty(); round++)
    {
      std::vector<UiElement *> elements;
      elements.swap(document.dirty_styles);

      // from the top, since an element takes from the one above it
      std::ranges::stable_sort(elements, [](const UiElement *a, const UiElement *b)
      {
        return DepthOf(a) < DepthOf(b);
      });

      for (UiElement *element : elements)
      {
        if (!element->IsDirty(UiDirty::Style)) { continue; }

        element->RefreshStates();
        (void) element->GetStyle();
      }
    }
  }

  std::vector<UiElement *> Tree_UiSystem::LayoutRootsOf(const UiDocument &document)
  {
    std::vector<UiElement *> roots;

    for (UiElement *element : document.dirty_layouts)
    {
      // An element that changed is placed by the one it is inside of. The
      // nearest above it whose size nothing inside decides is where what
      // changed ends.
      UiElement *root = element->GetParent() != nullptr ? element->GetParent() : element;
      while (!root->IsLayoutBoundary()) { root = root->GetParent(); }

      if (std::ranges::find(roots, root) == roots.end()) { roots.push_back(root); }
    }

    // what is inside another of them is placed with it
    std::vector<UiElement *> outermost;
    for (UiElement *root : roots)
    {
      const bool is_inside = std::ranges::any_of(roots, [root](const UiElement *other)
      {
        return other != root && IsInside(root, other);
      });

      if (!is_inside) { outermost.push_back(root); }
    }

    return outermost;
  }

  void Tree_UiSystem::LayOut(UiDocument &document, UiElement &root)
  {
    _statistics.layouts++;
    _statistics.laid_out_elements += CountOf(root);

    if (root.GetParent() == nullptr)
    {
      _statistics.full_layouts++;

      root.UpdateHidden(false);
      _layout->Calculate(root.GetLayoutNode(), document.frame.width, document.frame.height);
      root.Arrange(*_layout, 0.0f, 0.0f);
      return;
    }

    // The element keeps the box it has. What is inside it is placed in a
    // room of that size, as it was when everything was placed.
    const LayoutBox box = root.GetLayoutBox();

    LayoutStyle held = root.GetLayoutStyle();
    held.width = LayoutLength::Pixels(box.width);
    held.height = LayoutLength::Pixels(box.height);
    held.box_sizing = LayoutBoxSizing::BorderBox;
    held.min_width = LayoutLength::Auto();
    held.min_height = LayoutLength::Auto();
    held.max_width = LayoutLength::Auto();
    held.max_height = LayoutLength::Auto();
    held.position = LayoutPosition::Relative;
    held.inset = {};
    held.margin = {
      LayoutLength::Pixels(0.0f), LayoutLength::Pixels(0.0f), LayoutLength::Pixels(0.0f), LayoutLength::Pixels(0.0f)
    };

    root.UpdateHidden(root.GetParent()->IsHidden());

    _layout->SetStyle(root.GetLayoutNode(), held);
    _layout->Calculate(root.GetLayoutNode(), box.width, box.height);
    _layout->SetStyle(root.GetLayoutNode(), root.GetLayoutStyle());

    root.TakeLayout(*_layout, false);
    root.Replace();
  }

  void Tree_UiSystem::SettleLayout(UiDocument &document)
  {
    // What would be laid out as it was is only drawn again: a text that
    // changed to one that is as large. An element that was never laid out
    // cannot say, and is kept.
    if (!document.needs_full_layout)
    {
      std::erase_if(document.dirty_layouts, [&document, this](UiElement *element)
      {
        if (!element->LaysOutTheSame(document.frame)) { return false; }

        _statistics.layouts_spared++;
        element->ClearDirty(UiDirty::Layout);
        element->Invalidate(UiDirty::Paint);
        return true;
      });
    }

    if (document.needs_full_layout || !document.dirty_layouts.empty())
    {
      // the styles of what changed, as the layout engine is to see them
      if (document.needs_full_layout)
      {
        std::vector<UiElement *> elements;
        CollectUiElements(*document.root, elements);
        for (UiElement *element : elements) { element->PushLayoutStyle(*_layout); }
      } else
      {
        for (UiElement *element : document.dirty_layouts) { element->PushLayoutStyle(*_layout); }
      }

      std::vector<UiElement *> roots;
      if (document.needs_full_layout)
      {
        roots.push_back(document.root.get());
      } else
      {
        roots = LayoutRootsOf(document);
      }

      for (const UiElement *element : document.dirty_layouts)
      {
        const_cast<UiElement *>(element)->ClearDirty(UiDirty::Layout);
      }

      document.dirty_layouts.clear();
      document.needs_full_layout = false;

      for (UiElement *root : roots) { LayOut(document, *root); }

      // placing may have asked for styles, which asked for nothing new
      document.needs_paint = true;
      _needs_paint = true;
    }

    if (!document.dirty_places.empty())
    {
      std::vector<UiElement *> elements;
      elements.swap(document.dirty_places);

      for (UiElement *element : elements)
      {
        if (!element->IsDirty(UiDirty::Arrange)) { continue; }

        _statistics.places++;
        element->Replace();
      }

      document.needs_paint = true;
      _needs_paint = true;
    }
  }

  void Tree_UiSystem::UpdateStates(const bool accept_is_down)
  {
    // the elements that are in a state now: what is under the pointer of
    // each surface and what has the focus, with everything they are
    // inside of
    std::vector<UiElement *> now;

    const auto add = [&now](UiElement *element)
    {
      if (element != nullptr && std::ranges::find(now, element) == now.end()) { now.push_back(element); }
    };

    for (const auto &surface : _surfaces)
    {
      if (surface == nullptr) { continue; }

      for (UiElement *each = surface->hovered; each != nullptr; each = each->GetParent()) { add(each); }
      add(surface->pressed);
    }

    for (UiElement *each = _focused; each != nullptr; each = each->GetParent()) { add(each); }

    std::vector<UiElement *> all = now;
    for (UiElement *each : _in_state)
    {
      if (std::ranges::find(all, each) == all.end()) { all.push_back(each); }
    }

    for (UiElement *element : all)
    {
      const bool takes_input = TakesInput(element);

      // what is hovered and pressed on the surface the element is on
      const UiDocument *document = DocumentOf(element);
      const Surface *surface = document != nullptr ? FindSurface(document->surface) : nullptr;
      const UiElement *hovered = surface != nullptr ? surface->hovered : nullptr;
      const UiElement *pressed = surface != nullptr ? surface->pressed : nullptr;

      bool is_hovered = false;
      for (const UiElement *each = hovered; each != nullptr; each = each->GetParent())
      {
        if (each == element) { is_hovered = true; }
      }

      bool holds_focus = false;
      for (const UiElement *each = _focused; each != nullptr; each = each->GetParent())
      {
        if (each == element) { holds_focus = true; }
      }

      UiStates states = element->GetStates();
      states.hover = takes_input && is_hovered;
      states.focus = takes_input && element == _focused;
      states.focus_within = takes_input && holds_focus;
      states.active = takes_input &&
                      ((element == pressed && hovered == pressed) ||
                       (element == _focused && accept_is_down));

      element->SetStates(states);
    }

    _in_state = now;
    _hovered = _surfaces[Ui_Window_Surface]->hovered;
  }
} // neon
