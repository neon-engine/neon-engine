// An extension that shows something on the screen: it shows a file of the
// user interface, makes elements in it, sets their fields and their style,
// hears a click, and asks how large the view is.

#include <string>

#include <neon/extension/neon-extension.hpp>

// The extension and what it is made of, for this file alone.
namespace
{
  using neon::extension::UiElement;
  using neon::extension::UiEvent;
  using neon::extension::UiListener;
  using neon::extension::World;

  const char *yes_or_no(const bool value)
  {
    return value ? "yes" : "no";
  }

  /// Puts up a sign and a button when it starts, counts the knocks on the
  /// button, and does what the actions ask for.
  class Signing final : public neon::extension::System
  {
    static constexpr const char *kBoard = "assets://ui/board.ui.yml";

    UiElement _sign = 0;
    UiElement _button = 0;
    UiListener _listening = 0;
    int _knocks = 0;

  public:
    void Start(World &world) override
    {
      // shown twice, which is once
      const bool shown = world.ShowUi(kBoard) && world.ShowUi(kBoard);

      // a label at the top of the file, and a button inside what the file
      // calls `board`
      _sign = world.CreateUi(
        "type: label\n"
        "name: sign\n"
        "text: Open\n");
      _button = world.CreateUi(
        "type: button\n"
        "name: knock\n"
        "text: Knock\n"
        "box_sizing: border-box\n"
        "width: 100\n"
        "height: 40\n",
        world.FindUi("board"));

      // the button was made after the file was shown, so it has the focus
      // only when it is given it
      world.Info(std::string("Focused the button it made: ") + yes_or_no(world.FocusUi(_button)));

      const bool written = world.SetUiField(_sign, "text", "Closed");
      const bool styled = world.SetUiStyle(_sign, "color", "#ff8000") && world.SetUiStyle(_sign, "margin-left", "12px");
      const bool found = _sign != 0 && world.FindUi("sign") == _sign && world.FindUi("nothing") == 0;
      world.Info(std::string("Showed the board: ") + yes_or_no(shown) + ", made a sign and a button: "
                 + yes_or_no(_sign != 0 && _button != 0) + ", wrote on it: " + yes_or_no(written) + ", styled it: "
                 + yes_or_no(styled) + ", found it by its name: " + yes_or_no(found));

      // a number and a flag, each from text as a file writes it
      const UiElement bar = world.CreateUi(
        "type: bar\n"
        "name: filled\n"
        "max: 100\n");
      const UiElement box = world.CreateUi(
        "type: checkbox\n"
        "name: lit\n"
        "text: Lit\n");
      const bool numbered = world.SetUiField(bar, "value", "75");
      const bool flagged = world.SetUiField(box, "checked", "true");
      world.Info(std::string("A number from text: ") + yes_or_no(numbered) + ", a flag from text: " + yes_or_no(flagged));

      _listening = world.ListenToUi(_button, "click", [this, &world](const UiEvent &event)
      {
        _knocks++;
        world.Info("Heard a " + std::string(event.name) + " on " + (event.target == _button ? "the button" : "something else")
                   + ", knock " + std::to_string(_knocks));
        world.SetUiField(_sign, "text", "Knocked " + std::to_string(_knocks));
      });
      world.Info(std::string("Listens to the button: ") + yes_or_no(_listening != 0));

      int width = 0;
      int height = 0;
      const bool sized = world.GetViewSize(width, height);
      world.Info(std::string("The view is ") + (sized ? std::to_string(width) + " by " + std::to_string(height) : "unknown"));

      // what is refused: no text and text that is no element, an element
      // that never was, a field and a property that are not there, a value
      // a field cannot hold, a file that was never shown, and no event
      const bool refused = world.CreateUi("") == 0
                           && world.CreateUi(" \n") == 0
                           && world.CreateUi("type: nothing\n") == 0
                           && world.CreateUi("type: label\ntext: Lost\n", 4000000) == 0
                           && !world.SetUiField(4000000, "text", "Lost")
                           && !world.SetUiField(_sign, "weight", "1")
                           && !world.SetUiField(bar, "value", "much")
                           && !world.SetUiStyle(_sign, "colour", "red")
                           && !world.SetUiVisible(4000000, false)
                           && !world.FocusUi(4000000)
                           && !world.FocusUi(_sign)
                           && !world.RemoveUi(4000000)
                           && !world.CloseUi("assets://ui/never.ui.yml")
                           && !world.ShowUi("assets://ui/never.ui.yml")
                           && world.ListenToUi(_button, "", [](const UiEvent &) {}) == 0
                           && world.ListenToUi(4000000, "click", [](const UiEvent &) {}) == 0;
      world.Info(std::string("What is no element, field, property, or file was refused: ") + yes_or_no(refused));
    }

    void Update(World &world, const double delta_time) override
    {
      if (world.WasActionPressed("hide_sign")) { world.SetUiVisible(_sign, false); }

      if (world.WasActionPressed("take_down"))
      {
        world.Info(std::string("Took the sign down: ") + yes_or_no(world.RemoveUi(_sign)));
      }

      if (world.WasActionPressed("stop_listening")) { world.UnlistenToUi(_listening); }

      if (world.WasActionPressed("close_board"))
      {
        world.Info(std::string("Closed the board: ") + yes_or_no(world.CloseUi(kBoard)) + ", and the button is "
                   + (world.FindUi("knock") == 0 ? "gone" : "there"));
      }
    }
  };

  class SignExtension final : public neon::extension::Extension
  {
  public:
    bool Initialize(World &world) override
    {
      AddSystem<Signing>("Signing");
      return true;
    }
  };
}

NEON_EXTENSION(SignExtension)
