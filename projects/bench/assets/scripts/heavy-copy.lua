-- HeavyCopy: every frame it copies the position of its crate out of the
-- engine and back in, sixty-four times. No script would do this; it is the
-- worst case, made to measure the crossing between the script and the
-- store, not arithmetic. The same work as NativeHeavyCopy in C++.
local HeavyCopy = Component:extend {
  age = 0.0,
  last = vec3(),
}

local HeavyCopySystem = System:extend(HeavyCopy, "Transform")

-- Each round reads the three numbers of the position into a value of the
-- script, the engine to Lua, and writes that value into the component
-- whole, Lua to the engine.
local function churn(crate, position)
  for i = 1, 64 do
    local copy = vec3(position.x + i, position.y, position.z)
    crate.last = copy
  end
end

function HeavyCopySystem:update(entity, crate, transform, dt)
  crate.age = crate.age + dt
  churn(crate, transform.position)
end

return HeavyCopy, HeavyCopySystem
