-- NoCopy: every frame it reads the height of its crate from the
-- engine's Transform once, and counts how long the crate has been in the
-- scene and in how many frames it was below a line. It is what an everyday
-- script costs: one read from the engine and a little arithmetic. The same
-- work as NativeNoCopy in C++.
local NoCopy = Component:extend {
  floor = -5.0,
  air_time = 0.0,
  passes = integer(0),
}

local NoCopySystem = System:extend(NoCopy, "Transform")

function NoCopySystem:update(entity, crate, transform, dt)
  crate.air_time = crate.air_time + dt
  if transform.position.y < crate.floor then
    crate.passes = crate.passes + 1
  end
end

return NoCopy, NoCopySystem
