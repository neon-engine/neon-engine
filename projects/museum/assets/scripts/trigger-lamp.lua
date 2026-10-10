-- A TriggerLamp lights a lamp while the player is inside the Trigger of its
-- entity: it writes the `diffuse` of the Light of the entity `lamp` names,
-- by its path from the top. The plate of hall 09 of the museum,
-- demo.scene.yml, is one.
--
--   TriggerLamp:
--     lamp: scripts-lamp

local TriggerLamp = Component:extend {
  -- the path of the entity that carries the Light
  lamp = "",

  -- the light while the player is inside, and while not
  lit = vec3(1.6, 1.3, 0.7),
  dark = vec3(0, 0, 0),
}

local TriggerLampSystem = System:extend(TriggerLamp)

-- Gives the Light of the lamp the light asked for. A lamp that is not
-- there, or has no Light, is left alone.
local function shine(trigger_lamp, light)
  local lamp = world.find_entity(trigger_lamp.lamp)
  if lamp == nil or lamp.Light == nil then return end

  local diffuse = lamp.Light.diffuse
  diffuse.x = light.x
  diffuse.y = light.y
  diffuse.z = light.z
end

function TriggerLampSystem:on_trigger_enter(entity, trigger_lamp, other)
  if other:has_component("FirstPersonController") then shine(trigger_lamp, trigger_lamp.lit) end
end

function TriggerLampSystem:on_trigger_exit(entity, trigger_lamp, other)
  if other:has_component("FirstPersonController") then shine(trigger_lamp, trigger_lamp.dark) end
end

return TriggerLamp, TriggerLampSystem
