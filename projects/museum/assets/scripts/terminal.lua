-- A Terminal is what the buttons of a user interface call: the file of the
-- user interface names a handler of the system below with `on_click`, and
-- the handler is called for the entity that shows the user interface, with
-- what the file wrote as its arguments.
--
--   on_click: unlock($event)
--   on_click: alarm(blinks_per_second)
--
-- In a handler `self` is the entity that shows the user interface, and the
-- parameters are what the file wrote: what happened, which says who the
-- click comes from, and the value `blinks_per_second` of the user
-- interface. `unlock` slides a door open or shut, and `alarm` has a
-- lamp blink. The Terminal tells the user interface what it did through
-- its values, `door`, `unlock`, and `power`. The terminal of hall 10 of the
-- museum, demo.scene.yml, is one.
--
--   Terminal:
--     ui: museum-terminal
--     door: surfaces-safe-door
--     lamp: surfaces-alarm-lamp

local Terminal = Component:extend {
  -- the name the file of the user interface gives itself under `ui`
  ui = "",

  -- the paths of the entity that is slid, and of the one whose Light blinks
  door = "",
  lamp = "",

  -- how far the door slides up, in meters, and how fast, in meters a second
  lift = 0.95,
  speed = 1.6,

  -- the light of the lamp at its brightest
  light = vec3(6, 0.4, 0.3),

  -- what the bar shows at rest, from 0 to 1. With the alarm it is full
  power = 0.4,
}

local TerminalSystem = System:extend(Terminal)

function TerminalSystem:ready(entity, terminal)
  terminal._unlocked = false
  terminal._alarm = false
  terminal._blinks_per_second = 1
  terminal._lifted = 0
  terminal._time = 0
  terminal._power = terminal.power

  -- where the scene put the door is where it is shut
  local door = world.find_entity(terminal.door)
  if door ~= nil and door.Transform ~= nil then terminal._shut_y = door.Transform.position.y end
end

-- Called by the button that says `on_click: unlock($event)`, on the entity
-- that shows the user interface, which carries the Terminal. The
-- instigator of the event is who pointed at the screen: the player.
function TerminalSystem.handlers:unlock(event)
  local terminal = self.Terminal
  terminal._unlocked = not terminal._unlocked

  local state = terminal._unlocked and "unlocked" or "locked"
  if event.instigator ~= nil then state = state .. " by " .. event.instigator.name end
  ui.set_text_of(terminal.ui, "door", state)
  ui.set_text_of(terminal.ui, "unlock", terminal._unlocked and "Lock" or "Unlock")
end

-- Called by the button that says `on_click: alarm(blinks_per_second)`.
function TerminalSystem.handlers:alarm(blinks_per_second)
  local terminal = self.Terminal
  terminal._alarm = not terminal._alarm
  terminal._blinks_per_second = blinks_per_second or 1
  terminal._time = 0
end

function TerminalSystem:update(entity, terminal, dt)
  -- the door, on its way to where it belongs
  local door = world.find_entity(terminal.door)
  if door ~= nil and door.Transform ~= nil and terminal._shut_y ~= nil then
    local to = terminal._unlocked and terminal.lift or 0
    terminal._lifted = math.move_toward(terminal._lifted, to, terminal.speed * dt)
    door.Transform.position.y = terminal._shut_y + terminal._lifted
  end

  -- the lamp, dark without the alarm
  local level = 0
  if terminal._alarm then
    terminal._time = terminal._time + dt
    level = 0.5 - 0.5 * math.cos(terminal._time * terminal._blinks_per_second * 2 * math.pi)
  end

  local lamp = world.find_entity(terminal.lamp)
  if lamp ~= nil and lamp.Light ~= nil then
    local diffuse = lamp.Light.diffuse
    diffuse.x = terminal.light.x * level
    diffuse.y = terminal.light.y * level
    diffuse.z = terminal.light.z * level
  end

  local power = math.move_toward(terminal._power, terminal._alarm and 1 or terminal.power, dt)
  if power ~= terminal._power then
    terminal._power = power
    ui.set_number_of(terminal.ui, "power", power)
  end
end

return Terminal, TerminalSystem
