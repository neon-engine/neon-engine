-- A Spinner turns its entity about an axis, for ever. It is the first
-- script of the engine, and what scripting-demo.scene.yml of the tests and
-- the museum, demo.scene.yml, place, each with a copy of this file; the
-- docs describe how a script is written in docs/scripting.md.
--
-- The component is named after the file: Spinner. A scene writes it as
--
--   Spinner:
--     speed: 180
--
-- and leaves out what keeps its default.

local Spinner = Component:extend {
  -- degrees a second
  speed = 90.0,

  -- about which axis: x, y, or z
  axis = "y",
}

-- The system runs over every entity with a Spinner and a Transform, once
-- per frame, and turns the Transform by what the time of the frame says.
local SpinnerSystem = System:extend(Spinner, "Transform")

function SpinnerSystem:update(entity, spinner, transform, dt)
  local turned = spinner.speed * dt
  local rotation = transform.rotation
  if spinner.axis == "x" then
    rotation.x = rotation.x + turned
  elseif spinner.axis == "z" then
    rotation.z = rotation.z + turned
  else
    rotation.y = rotation.y + turned
  end
end

return Spinner, SpinnerSystem
