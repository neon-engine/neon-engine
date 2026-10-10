#ifndef UI_SELECT_HPP
#define UI_SELECT_HPP

#include <neon/ui/ui-element.hpp>

namespace neon
{
  /// One of several choices, shown as the one chosen with a list that
  /// opens under it: a dropdown.
  ///
  ///     type: select
  ///     name: quality
  ///     options: [low, medium, high]
  ///     value: "{quality}"
  ///
  /// An option is a text, which is both what is shown and what it stands
  /// for, or a map with `value` and `text`:
  ///
  ///     options:
  ///       - {value: 1, text: Low}
  ///       - {value: 2, text: High}
  ///
  /// `value` is what is chosen, or a value of the game in brackets, which
  /// choosing sets. A click, accept, or space opens the list; up and down
  /// move through it once it is open, and move the focus on while it is
  /// closed; accept chooses, and cancel closes. It reports `changed` with the value, and
  /// with `on_change: tune` choosing calls a function of the scripts.
  ///
  /// The list is drawn on top of everything else, and is the part `list`;
  /// each choice in it the part `option`, and the one the keys are on the
  /// part `highlight`. The arrow at the right is the part `arrow`.
  class UiSelect final : public UiElement
  {
  public:
    struct Option
    {
      std::string value;
      std::string text;
    };

  private:
    UiFlag _enabled{true};
    bool _is_enabled = true;
    bool _autofocus = false;

    // the function `on_change` names, with what it is handed
    UiCall _on_change;

    std::vector<Option> _options;
    std::string _placeholder;
    std::size_t _chosen = 0;
    bool _has_chosen = false;
    std::string _binding;
    bool _has_followed = false;
    std::uint64_t _followed_revision = 0;

    bool _is_open = false;
    std::size_t _highlighted = 0;

    // the first option that is seen, for a list longer than is shown
    std::size_t _first_shown = 0;

    // how high a line of the font is, in units of the file, as it was
    // last measured
    float _line_height = 0.0f;

    /// How many options are shown at a time.
    static constexpr std::size_t shown_at_most = 8;

    /// How high a row of the list is, in units of the file.
    [[nodiscard]] float RowHeight() const;

    /// Where the list is, in units of the file.
    [[nodiscard]] UiRectangle ListBox() const;

    void Open();

    void Close();

    /// Chooses the option, and tells the game. `by_player` says that the
    /// player chose it, and not the game.
    void Choose(std::size_t index, bool by_player);

    /// Keeps the highlighted option in the part of the list that is seen.
    void ShowHighlighted();

    /// The option under a place of the list, or the count of options.
    [[nodiscard]] std::size_t OptionAt(float y) const;

    /// The option whose value is the given one, or the count of options.
    [[nodiscard]] std::size_t IndexOf(const std::string &value) const;

  public:
    static constexpr const char *kType = "select";

    void ApplyDefaults(UiStyle &style) const override;

    void ApplyStateDefaults(UiStyle &style, const UiStates &states) const override;

    void ApplyPartDefaults(const std::string &part, UiStyle &style) const override;

    void ReadAttributes(const DataReader &reader) override;

    [[nodiscard]] bool IsFocusable() const override;

    [[nodiscard]] bool IsClickable() const override;

    [[nodiscard]] bool WantsFocus() const override;

    [[nodiscard]] bool IsEnabled() const override;

    [[nodiscard]] bool UsesDirection(Key direction) const override;

    [[nodiscard]] bool HasContent() const override;

    [[nodiscard]] bool TellsWhenItChanged() const override;

    [[nodiscard]] const UiCall *CallsWhenChanged() const override;

    [[nodiscard]] LayoutSize Measure(
      const UiFrame &frame,
      float available_width,
      float available_height) override;

    void Update(const UiFrame &frame) override;

    void Interact(UiInteraction &interaction, const UiFrame &frame) override;

    void PaintContent(
      UiPainter &painter,
      const UiFrame &frame,
      const UiRectangle &content_box,
      float opacity) override;

    [[nodiscard]] bool HasTopLayer() const override;

    [[nodiscard]] bool TopLayerContains(const UiFrame &frame, float x, float y) const override;

    void PaintTopLayer(UiPainter &painter, const UiFrame &frame) override;

    [[nodiscard]] bool GetField(const std::string &name, FieldValue &value) const override;

    bool SetField(const std::string &name, const FieldValue &value, std::string &error) override;

    [[nodiscard]] std::vector<Field> GetFields() const override;

    /// The value of what is chosen, or empty for nothing.
    [[nodiscard]] std::string GetValue() const;

    /// The text of what is chosen, or the placeholder.
    [[nodiscard]] std::string GetText() const;

    [[nodiscard]] const std::vector<Option> &GetOptions() const;

    [[nodiscard]] bool IsOpen() const;

    [[nodiscard]] std::size_t GetHighlighted() const;
  };
} // neon

#endif //UI_SELECT_HPP
