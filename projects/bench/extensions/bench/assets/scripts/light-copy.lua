-- LightCopy: every frame it adds the time of the frame to one field, and
-- does nothing else. It is the least a script can do, so what is measured is
-- the cost of calling a hook for every crate. The same work as
-- NativeLightCopy in C++.
local LightCopy = Component:extend {
  age = 0.0,
}

local LightCopySystem = System:extend(LightCopy)

function LightCopySystem:update(entity, crate, dt)
  crate.age = crate.age + dt
end

return LightCopy, LightCopySystem
