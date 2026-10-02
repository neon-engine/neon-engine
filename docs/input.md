# Input

This note records how a game reads the player's input: the actions a project
names, what is bound to each, and the states that say which are live. What
the user interface does with the input before the game sees it is in
[user-interface.md](user-interface.md#input-and-focus); how a run without a
window is operated is in
[development.md](development.md#running-without-a-window).

**Current decision:** a game reads **actions by their names**, such as `move`
and `jump`, and never a key or a button. An **input map** binds the actions
to the devices and sorts them into **states** — walking, driving, swimming, a
menu — of which one is current, and only its actions fire. The map is a
recipe of the project, `assets://input/<name>.input.yml`, which `project.yml`
names as `input`. A project without one plays with the engine's default,
which is the map the engine played with before a project could say. The
backends know nothing of actions: they read the devices as they are, and the
map is applied in neon-core, the same for every backend and for a script.

## The file

```yaml
version: 1

actions:
  move:     { type: axis2, keys: [w, s, a, d], stick: left }
  look:     { type: axis2, mouse: motion, stick: right, rate: 600 }
  throttle: { type: axis, keys: [w, s], trigger: right }
  jump:     { type: button, keys: [space], buttons: [south] }
  shoot:    { type: button, mouse: left, buttons: [right-trigger] }
  pause:    { type: button, keys: [escape], buttons: [start] }

states:
  walking: [move, look, jump, shoot, pause]
  driving: [throttle, look, pause]
  menu:    [pause]
```

The map of the runtime is
[default.input.yml](../app/NeonRuntime/assets/input/default.input.yml).

| Name | Holds | When it is left out |
|---|---|---|
| `version` | The version of this layout, which is 1, as in a scene | An error. A file of a later layout is refused with its version |
| `actions` | The actions by their names, each a map of its type and its bindings. At least one | An error |
| `states` | The states by their names, each the list of the actions that are live in it. At least one; the first is the one the game starts in. A state may be empty, for a cutscene in which nothing fires | An error |

An action has:

| Name | Holds | For |
|---|---|---|
| `type` | `button`: down or not. `axis`: one number, from -1 to 1, or 0 to 1 from a trigger. `axis2`: two numbers, x to the right and y forward. `axis3`: three numbers from a motion sensor, about x, y, and z | All. Required |
| `keys` | Keys, by the names below, each one key or a chord of keys held together, `[left-shift, w]`. A button is down while any of them is. An axis takes exactly two: positive, negative. An axis2 exactly four: up, down, left, right | All |
| `buttons` | Buttons of a controller, in the same way | All |
| `mouse` | A button of the mouse for a button: `left`, `right`, `middle`. `motion` for an axis2: the mouse moves it, in pixels | A button, an axis2 |
| `stick` | `left` or `right`: the stick moves the axis2, from -1 to 1 | An axis2 |
| `trigger` | `left` or `right`: how far the analog trigger is pulled, from 0 to 1. The thresholded form is the button `left-trigger` or `right-trigger` | An axis |
| `sensor` | `gyro` or `accelerometer`: a motion sensor of the controller, the one source of an axis3 | An axis3. Required there |
| `enabled` | `true` turns the sensor on when the map loads, for a game that wants it from the start. A sensor is off otherwise, until the player turns it on | An axis3 |
| `rate` | How much a stick or a trigger pulled all the way counts per second, in the unit the mouse gives, pixels. Without it the raw value from -1 to 1 is taken, which is right for walking and wrong for turning. On an axis3 it scales what the sensor gives | An axis, an axis2, an axis3 |
| `dead_zone` | How far the stick or the trigger goes before it counts, from 0 to below 1. Left out, a quarter of the way, `0.25f`, where a stick rests; `0` takes the device as it is | An axis, an axis2 |
| `curve` | How the stick or the trigger answers past the dead zone: `linear`, or a power above 0 that the value is raised to. `2` is gentle near the middle and quick at the end. Left out, linear | An axis, an axis2 |

An action without a binding is allowed: it never fires until a player binds
it, see [the open questions](#open-questions). A name that is not known is an
error, as in a scene file, and so is a key, a button, or a stick that is not
known, an action a state names that there is not, a binding on a type it is
not for (`stick` on a button, `trigger` on an axis2, `mouse` on an axis,
`rate` on a button, `sensor` or `enabled` on anything but an axis3, keys or
buttons on an axis3, `dead_zone` or `curve` on a button or an axis3), an
axis3 without a sensor, a rate that is not above zero, a dead zone that is
not from 0 to below 1, a curve that is neither `linear` nor a power above 0,
and an axis with other than two keys or an axis2 with other than four. Every
problem is
reported with its line, not only the first, and the runtime stops before
anything else starts.

### The names of the devices

The names are the engine's, the same on every platform and with every
backend. Only `neon-sdl2` knows what SDL calls them.

| Device | Names |
|---|---|
| Keys that move | `left`, `right`, `up`, `down`, `home`, `end`, `page-up`, `page-down` |
| Keys that edit | `backspace`, `delete`, `enter`, `tab`, `escape`, `space` |
| Letters | `a` to `z` |
| Digits | `0` to `9`, the row above the letters. They may be written without quotes |
| Function keys | `f1` to `f12` |
| Modifiers | `left-shift`, `right-shift`, `left-control`, `right-control`, `left-alt`, `right-alt` |
| Buttons of the mouse | `left`, `right`, `middle` |
| The face buttons of a controller | `south`, `east`, `west`, `north`, by where they are: `south` is the lower one, A on one controller and B on another |
| The other buttons | `left-shoulder`, `right-shoulder`, `left-trigger`, `right-trigger` (pulled past the half), `left-stick`, `right-stick` (pressed), `start`, `back`, `guide` |
| The pad | `dpad-up`, `dpad-down`, `dpad-left`, `dpad-right` |
| Sticks | `left`, `right` |
| Triggers, as analog | `left`, `right`, under `trigger` |
| Motion sensors | `gyro`, `accelerometer`, under `sensor` |

### Chords

A binding that is a list of names is a **chord**: its keys, or its buttons,
have to be held together, and the chord is down while every one of them is.
`[left-shift, w]` is shift with W, `[left-shoulder, south]` the shoulder with
the lower face button. A chord stands in any place a key does, so an axis
takes two of them and an axis2 four: `keys: [[left-shift, w], s, a, d]` is an
axis2 that goes forward only with shift. A chord does not take the key away
from a plain binding: with `jump` on `space` and `sprint-jump` on
`[left-shift, space]`, both are down while shift and space are. The order in
a chord does not matter, and an empty chord is an error, as is a list inside
one.

```yaml
actions:
  jump:        { type: button, keys: [space] }
  sprint-jump: { type: button, keys: [[left-shift, space]], buttons: [[left-shoulder, south]] }
  lean:        { type: axis, keys: [[left-shift, d], [left-shift, a]] }
```

A key is bound **by where it is**, as on a keyboard of the US layout: `w` is
the key above `s` on every layout, so a game that is played with W, A, S, and
D is played with the same four keys in France. That is the other way round
from a shortcut of the user interface, which follows the letter the layout
puts on the key, see [user-interface.md](user-interface.md#typing).

### Axes

An axis2 is `x` to the right and `y` forward, so that `move.y` is how much
the player walks ahead and `look.y` how far the view goes up. Keys and the
pad give -1, 0, or 1 in each; a stick gives what it is pushed to, and a stick
on top of keys does not go past 1 in either. The mouse is different: it
gives the pixels it moved in the frame and has no end, so an axis2 with
`mouse: motion` is not cut down.

A stick next to the mouse would be lost: -1 to 1 next to tens of pixels a
frame. `rate` says how many pixels a second the stick pushed all the way
stands for, and the stick then counts `value × rate × time of the frame`,
so that `look` from the stick is as quick on a slow machine as on a fast
one. The default map turns at 600 pixels a second, and the spectator reads
`look` without caring where it came from. An axis2 with a rate is not cut
down to 1 either. `move` has no rate: a stick there is how fast to walk,
not how far, and the game scales it by the time that passed itself.

An axis is one number: the first key or button is the positive one, the
second the negative one, and a trigger gives how far it is pulled, from 0 to
1, at `rate` pixels a second when it has one. The largest of them wins when
several are used at once, so a key pressed all the way beats a trigger half
pulled, whichever way it goes.

### Dead zones and curves

A stick is never quite in the middle when it rests, and a trigger not
always at zero, so the backends hand them over as they are and the map
shapes them, per action. `dead_zone` is how far the device goes before it
counts, and what is past it is stretched so that the end is still 1: with
the quarter that is taken off when nothing is said, a stick pushed half way
reads a third, and one pushed all the way reads 1. A stick's dead zone is
round, by how far the stick is from the middle, so that a push aslant goes
through it as a push straight ahead does; the corner is cut down to 1 in
each direction after, as a stick on top of keys is. `curve` raises what is
stretched to a power: `linear`
is the value as it is, `2` makes the stick gentle near the middle and quick
at the end, for a view that is too lively around rest. Keys, buttons, and
the mouse are not shaped; they have no middle to rest off. The user
interface, which scrolls with the right stick as a device, keeps a rest zone
of its own.

```yaml
actions:
  move: { type: axis2, keys: [w, s, a, d], stick: left, dead_zone: 0.2 }
  look: { type: axis2, mouse: motion, stick: right, rate: 600, curve: 2 }
  gas:  { type: axis, trigger: right, dead_zone: 0.05, curve: linear }
```

An axis3 is what a motion sensor of the controller gives, about or along
its three axes, which are SDL's: x to the right, y up, z toward the player
who holds it. A turn about x is pitch, about y yaw, about z roll. The gyro
gives radians a second, and the frame turns that into radians turned in it,
`value × time of the frame`, as `rate` turns a stick into pixels a frame;
so an `aim` bound to the gyro is added to the view as it is. The
accelerometer gives metres a second squared, gravity included, as it is at
the moment. `rate` scales either. See [sensors](#sensors) for when it reads
at all.

A frame of the keyboard says W is down, and the engine says `move` is
`(0, 1)`. The SDL backend reports the sticks downwards as positive, as SDL
does; the map turns them forward. The time of the frame is the window's,
or the `--time-step` without one, and the input system writes it into the
`InputState` of the frame, so that what re-evaluates the actions after the
user interface has the same time.

## States

A game is in one state of the map at a time, and only the actions of that
state fire: whatever is pressed, `jump` does nothing while driving. The
first state of the file is the one the game starts in. A game changes it
with `SetState("driving")`, which takes effect from the next frame, and
asks `GetState()`. A state the map does not have is refused, logged, and
nothing changes.

An action that is in several states keeps being down across them: `pause`
held while the game goes from walking to driving is not pressed again.

The runtime does not change the state by itself. The world stands still
while the pause menu is shown, so a game that keeps playing under its menus
puts itself into `menu`, and the default map has one.

## Sensors

A sensor is something the player turns on and off, since it costs power and
a game that does not want it should not pay for it. It is **off** until
something turns it on, even when the map binds it, and an axis3 bound to an
off sensor reads zero; the controller is not asked for it either.

| Who turns it | How |
|---|---|
| The map | `enabled: true` on the action, for a game that wants it from the start. Applied when the map loads |
| The settings | `input.gyro` in `settings.yml`, the player's switch, applied after the map and over it, see [settings.md](settings.md). Left out, the map decides |
| The game | `SetSensorEnabled("gyro", true)` and `IsSensorEnabled("gyro")` on the input, which a settings menu toggles later, next to rebinding (#199) |

A controller without the sensor leaves the action at zero, said once in the
log when the sensor is turned on. The SDL2 backend asks the controller with
`SDL_GameControllerSetSensorEnabled` and reads
`SDL_GameControllerGetSensorData`; nothing else knows SDL's names. The
headless input has `SetSensor()` for a test to feed a reading, which a
script will be able to say later.

## How a game reads it

```cpp
// a system of the game, given ui_system.GetGameInput() as its InputContext
const glm::vec2 move = input->ActionAxis("move");
const float throttle = input->ActionAmount("throttle");
if (input->WasActionPressed("jump")) { Jump(); }
if (input->IsActionDown("shoot")) { Shoot(delta_time); }
input->SetState("driving");
```

| Call | Answers |
|---|---|
| `IsActionDown(name)` | Whether the action is down in this frame. An axis is down while it is away from its middle. False for a name the map does not have, and outside the current state |
| `WasActionPressed(name)` | Whether it went down in this frame and was not down in the one before. For what happens once a press, such as a jump or the pause menu |
| `ActionAxis(name)` | Where an axis2 is, as a `glm::vec2`. Zero for anything else |
| `ActionAmount(name)` | Where an axis is, as a `float`. Zero for anything else |
| `ActionAxis3(name)` | What an axis3 read, as a `glm::vec3`: about x, y, and z. Zero for anything else, and while its sensor is off |
| `SetSensorEnabled(sensor, on)`, `IsSensorEnabled(sensor)` | A motion sensor, `gyro` or `accelerometer`, see [sensors](#sensors) |
| `SetState(name)`, `GetState()` | The state the game is in |
| `GetInputMap()` | The map, for a settings menu that lists the bindings |

The user interface still takes the input first, see
[what is left for the game](user-interface.md#what-is-left-for-the-game):
the game reads through `UiInputGate`, which works the actions out again from
what the user interface left. A click on a button does not `shoot` as well,
a word typed into a text field does not `move`, and a press that closed the
pause menu does not reach the game as `pause`. `GetInputState()` is still
there for what is not an action: the pointer, the wheel, the text, and the
keys of the user interface.

What moved to actions:

| Was | Is | Default binding |
|---|---|---|
| `Action::L_Up` to `L_Left`, read by `SpectatorMovement` | `move`, an axis2 | W, S, A, D; the left stick |
| `Action::Mouse` and `Axis::Mouse`, read by `SpectatorMovement` | `look`, an axis2 | The motion of the mouse; the right stick at 600 pixels a second |
| `Action::Pause`, read by `Runtime` for the pause menu | `pause`, a button | Escape; start |

And what came with the player, see [physics.md](physics.md#the-player):

| Action | Is | Default binding | Read by |
|---|---|---|---|
| `jump` | A button | Space; south | `PlayerMovement`, once per press, while the player stands on the ground |
| `run` | A button | Left shift | `PlayerMovement`, which walks at `run_speed` while it is down. A controller has no binding: the stick says how fast to walk |

The `playing` state of the default map has `move`, `look`, `jump`, `run`, and
`pause`; `menu` has `pause` alone.

`Ui_Up` to `Ui_Cancel` and `Pointer_Primary` stay fixed actions of the user
interface: they are the engine's, not the game's, and every project moves
through a menu the same way. `L_*` and `R_*` are gone (#203): nothing of the
engine read them, and the tests that did set keys and buttons instead.

## Scripts

A script of a run without a window holds an action by its name, `hold jump`,
as it holds `ui-accept`, a key by where it is, `hold-key w 15`, and a button
of a controller by its name, `hold-button south`. It pushes the sticks:
`stick X Y` the right one and `left-stick X Y` the left one, from -1 to 1,
to the right and down as the backends give them, and the map shapes them as
it shapes a controller's. A hold of an action the map does not have is
refused before the run starts, with the names the map has, and so is a hold
of an axis: its keys are held with `hold-key`, its buttons with
`hold-button`, and its stick is pushed. `down`, `up`, and `click` are the
left button of the mouse as well as the button of the pointer, so that
`shoot` fires from a script. See
[development.md](development.md#running-without-a-window).

```
1: left-stick 0 -1 30      # walks ahead for half a second
10: hold-button south      # and jumps
20: hold-key w 5; left-stick 0.5 0 5
```

## How it is built

| Piece | Location | Role |
|---|---|---|
| `Key`, `MouseButton`, `ControllerButton`, `Stick`, `ControllerTrigger`, `Sensor` | neon-core, `neon/input/` | The devices by their names, with `NameOf` and the way back |
| `Chord` | neon-core, `neon/input/chord.hpp` | A key or a button, or several held together, which is what a binding holds |
| `InputState` | neon-core, `neon/input/input-state.hpp` | The devices of a frame as they are: the keys that are down, the buttons, both sticks, both triggers, the sensors that are on, the time of the frame, and the actions a script holds by name |
| `InputAction`, `InputMapState`, `InputMap` | neon-core, `neon/input/` | What the file holds, as values, and `InputMap::Default()` |
| `InputMapFile` | neon-core, `neon/input/input-map-file.hpp` | Reads the file through the file system and a `DocumentFormat`, checks it, and collects every problem |
| `InputActions` | neon-core, `neon/input/input-actions.hpp` | Works the actions out once a frame from the map, the state, and an `InputState` |
| `InputContext` | neon-core, `neon/input/input-context.hpp` | The calls above, which `InputSystem`, `UiInputGate`, and the fakes implement |
| `InputSystem` | neon-core, `neon/input/input-system.hpp` | Holds the map, the state, and which sensors are on for the backends, which call `RefreshActions()` at the end of a frame with the time it stands for: the window's for SDL2, the time step for headless. `OnSensorEnabled()` tells a backend to ask the controller |
| `SDL2_InputSystem`, `Headless_InputSystem` | neon-sdl2, neon-core | Read the devices, or a script, into the `InputState`, the sticks and the triggers as they are. Only the first knows SDL's names |
| `main.cpp` | NeonRuntime | Reads the map the project names and gives it to the input system before the script is checked, then applies the player's `input.gyro` |

The tests of the reader are in `tests/input-maps`, with the map of the
runtime read as it is in the repository and checked against the built-in
default. `InputActions` is tested next to its file: states switch, an action
outside the state does not fire, an axis is put together from four keys.

## Open questions

- **Rebinding by the player**: a settings menu lists the actions from
  `GetInputMap()` and lets the player press a key for each. What changed is
  written to `user://input.yml` on top of the project's map, as settings are
  layered ([settings.md](settings.md)), and `DocumentFormat::Write` is needed
  for it. An action without bindings in the project's map is there for that.
- **A sensor in a script**: `SetSensor()` of the headless input has no line
  of the script yet, so a run without a window cannot feed a gyro.
- **Several controllers**: the first one plugged in is read. Local play with
  two needs a device in the binding, or a map per player.
