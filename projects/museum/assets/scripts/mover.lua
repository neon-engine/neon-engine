-- A Mover glides its entity from where the scene put it to `travel` away
-- and back, for ever, slowing at both ends. On a kinematic body it is a
-- lift or a platform, which carries and pushes what is in its way; on
-- anything else it is something that hovers. The museum, demo.scene.yml,
-- uses it for both.
--
--   Mover:
--     travel: [0, 2.5, 0]
--     period: 8

local Mover = Component:extend {
  -- how far it goes from where it starts, in meters along each axis
  travel = vec3(0, 1, 0),

  -- seconds there and back
  period = 4.0,
}

local MoverSystem = System:extend(Mover, "Transform")

function MoverSystem:ready(entity, mover, transform)
  local position = transform.position
  mover._start_x = position.x
  mover._start_y = position.y
  mover._start_z = position.z
  mover._time = 0
end

function MoverSystem:update(entity, mover, transform, dt)
  mover._time = mover._time + dt

  -- from 0 to 1 and back, along a cosine, so that it starts and stops softly
  local along = 0.5 - 0.5 * math.cos(mover._time / mover.period * 2 * math.pi)
  local travel = mover.travel
  local position = transform.position
  position.x = mover._start_x + travel.x * along
  position.y = mover._start_y + travel.y * along
  position.z = mover._start_z + travel.z * along
end

return Mover, MoverSystem
