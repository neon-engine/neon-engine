#ifndef UI_TEXT_FIELD_HPP
#define UI_TEXT_FIELD_HPP

#include <limits>
#include <string>

#include <neon/ui/ui-element.hpp>
#include <neon/ui/ui-text-editor.hpp>
#include <neon/ui/ui-text-measure.hpp>

namespace neon
{
  /// What a text that is typed into is, whether it is one line or several:
  /// the text with its caret and its selection, how it follows a value of
  /// the game and writes back what is typed, what the keys and the pointer
  /// do to it, and how it is drawn.
  ///
  ///     type: input
  ///     name: player
  ///     value: "{player_name}"
  ///     placeholder: Your name
  ///     max_length: 16
  ///
  /// `value` may be text, or the name of a value of the game in brackets.
  /// With a name, what the game sets is shown, and what is typed is written
  /// back to it. The game is told of every change as `changed`, and of
  /// return in a text of one line as `submitted`. With `on_change: tune`
  /// what is typed calls a function of the scripts.
  ///
  /// The parts a style sheet reaches: `::placeholder` for what is shown
  /// while nothing is typed, and `::selection` for what is selected.
  class UiTextField : public UiElement
  {
    UiTextEditor _editor;

    std::string _placeholder;
    std::string _binding;
    std::string _pattern;
    UiFlag _enabled{true};
    bool _is_enabled = true;
    bool _autofocus = false;

    // the function `on_change` names, with what it is handed
    UiCall _on_change;

    // the values of the game the text was last taken from
    std::uint64_t _followed_revision = 0;
    bool _has_followed = false;

    // whether the caret is seen, and since when
    bool _has_focus = false;
    bool _caret_is_shown = true;
    float _blink = 0.0f;

    // how far the text is moved to the left so that the caret is seen,
    // in pixels, for a text of one line
    float _shift = 0.0f;

    // the text as it was last laid out, in units of the file
    float _content_width = 0.0f;
    float _content_height = 0.0f;

    // where the pointer went down, to tell a drag from a click
    bool _is_selecting = false;

    /// Whether a text of several lines has to be scrolled to its caret
    /// when it is next drawn, which is when its size is known.
    bool _scrolls_to_caret = false;

    /// Where the caret wants to be from the left while it moves up and
    /// down, in pixels, so that a short line does not lose it. Not a
    /// number when it was moved sideways since.
    float _wanted_x = std::numeric_limits<float>::quiet_NaN();

    [[nodiscard]] const UiFont *FontOf(const UiFrame &frame) const;

    [[nodiscard]] UiTextMeasure::Request RequestOf(const UiFrame &frame, const std::string &shown) const;

    [[nodiscard]] const UiTextMeasure &MeasureOf(const UiFrame &frame) const;

    /// The place in the text a place in the frame points at, in units of
    /// the file from the corner of the document.
    [[nodiscard]] std::size_t OffsetAt(const UiFrame &frame, float x, float y) const;

    /// Moves the caret up or down a line, to the place under it.
    void MoveVertically(const UiFrame &frame, bool up, bool selecting);

    void KeepCaretInView(const UiFrame &frame);

    void Changed(const UiFrame &frame);

    void HandleKey(UiInteraction &interaction, const UiFrame &frame);

  public:
    /// What holds the text, the caret, and the selection.
    [[nodiscard]] const UiTextEditor &GetEditor() const;

  protected:
    UiTextEditor &GetEditor();

    /// Whether return submits, which it does for a text of one line.
    [[nodiscard]] virtual bool SubmitsOnEnter() const;

    /// The width the lines are broken at, in pixels, or not a number for
    /// lines that are not broken.
    [[nodiscard]] virtual float WrapWidthOf(const UiRectangle &content_box) const;

  public:
    UiTextField();

    void ApplyDefaults(UiStyle &style) const override;

    void ApplyStateDefaults(UiStyle &style, const UiStates &states) const override;

    void ApplyPartDefaults(const std::string &part, UiStyle &style) const override;

    void ReadAttributes(const DataReader &reader) override;

    [[nodiscard]] bool IsFocusable() const override;

    [[nodiscard]] bool WantsFocus() const override;

    [[nodiscard]] bool IsEnabled() const override;

    [[nodiscard]] bool TakesText() const override;

    [[nodiscard]] bool UsesDirection(Key direction) const override;

    [[nodiscard]] bool HasContent() const override;

    [[nodiscard]] bool TellsWhenItChanged() const override;

    [[nodiscard]] const UiCall *CallsWhenChanged() const override;
    [[nodiscard]] bool IsInvalid() const override;

    [[nodiscard]] LayoutSize Measure(
      const UiFrame &frame,
      float available_width,
      float available_height) override;

    void Update(const UiFrame &frame) override;

    void Interact(UiInteraction &interaction, const UiFrame &frame) override;

    [[nodiscard]] bool GetCaretBox(const UiFrame &frame, UiRectangle &box) const override;

    bool Tick(const UiFrame &frame, float seconds) override;

    [[nodiscard]] bool GetContentSize(float &width, float &height) const override;

    void PaintContent(
      UiPainter &painter,
      const UiFrame &frame,
      const UiRectangle &content_box,
      float opacity) override;

    [[nodiscard]] bool GetField(const std::string &name, FieldValue &value) const override;

    bool SetField(const std::string &name, const FieldValue &value, std::string &error) override;

    [[nodiscard]] std::vector<Field> GetFields() const override;

    /// The text as it is now.
    [[nodiscard]] const std::string &GetText() const;

    /// Puts another text in, as the game does. What is typed can no
    /// longer be undone.
    void SetText(const std::string &text);

    [[nodiscard]] const std::string &GetPlaceholder() const;

    /// Whether what is typed fits the pattern, when there is one. A
    /// pattern is a text with `*` for anything and `?` for one character,
    /// or `[0-9]` for one of a range, as a shell writes them.
    [[nodiscard]] bool IsValid() const;
  };

  /// A text of one line that is typed into. `kind` is `text`, `password`,
  /// or `number`.
  class UiInput final : public UiTextField
  {
  public:
    static constexpr const char *kType = "input";

    void ReadAttributes(const DataReader &reader) override;

    [[nodiscard]] bool GetField(const std::string &name, FieldValue &value) const override;

    bool SetField(const std::string &name, const FieldValue &value, std::string &error) override;

    [[nodiscard]] std::vector<Field> GetFields() const override;
  };

  /// A text of several lines that is typed into, and scrolls. `rows` says
  /// how many lines are seen.
  class UiTextArea final : public UiTextField
  {
    int _rows = 3;

  protected:
    [[nodiscard]] bool SubmitsOnEnter() const override;

    [[nodiscard]] float WrapWidthOf(const UiRectangle &content_box) const override;

  public:
    static constexpr const char *kType = "textarea";

    UiTextArea();

    void ApplyDefaults(UiStyle &style) const override;

    void ReadAttributes(const DataReader &reader) override;

    [[nodiscard]] LayoutSize Measure(
      const UiFrame &frame,
      float available_width,
      float available_height) override;

    [[nodiscard]] bool GetField(const std::string &name, FieldValue &value) const override;

    bool SetField(const std::string &name, const FieldValue &value, std::string &error) override;

    [[nodiscard]] std::vector<Field> GetFields() const override;
  };
} // neon

#endif //UI_TEXT_FIELD_HPP
