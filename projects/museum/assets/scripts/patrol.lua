-- A Patrol walks its character back and forth along x: from where the
-- scene put it to `distance` further, and back. It writes the velocity of
-- the CharacterBody and nothing else; the physics walks the body, stops it
-- at what is in its way, and keeps it on the ground. The walker of hall 08
-- of the museum, demo.scene.yml, is one.
--
--   Patrol:
--     speed: 1.5
--     distance: 5

local Patrol = Component:extend {
  -- meters a second
  speed = 1.5,

  -- meters from where it starts to where it turns round
  distance = 4.0,
}

-- The system cannot name the CharacterBody it writes: the scripts are read
-- before the physics declares its components (#351), so the body is taken
-- from the entity in the hook, where it is known.
local PatrolSystem = System:extend(Patrol, "Transform")

function PatrolSystem:ready(entity, patrol, transform)
  patrol._start = transform.position.x
  patrol._heading = 1
end

function PatrolSystem:update(entity, patrol, transform, dt)
  local body = entity.CharacterBody
  if body == nil then return end

  local walked = transform.position.x - patrol._start
  if walked >= patrol.distance then
    patrol._heading = -1
  elseif walked <= 0 then
    patrol._heading = 1
  end

  local velocity = body.velocity
  velocity.x = patrol.speed * patrol._heading
end

return Patrol, PatrolSystem
