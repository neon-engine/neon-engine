-- A Cart is driven by the input while its entity is possessed, see
-- docs/possession.md: `move` sets the velocity of its kinematic RigidBody
-- ahead and back and turns it, and `jump` hands the driver back to the
-- character that got in, which the CartEntry of the plate wrote down. The
-- hooks of possession say in the log when the cart is taken and left, and
-- stop it when it is left.
--
--   Cart:
--     speed: 6
--     turn_speed: 90

local Cart = Component:extend {
  -- meters a second ahead with `move` pushed all the way
  speed = 6,

  -- degrees a second the cart turns with `move` pushed to the side
  turn_speed = 90,

  -- the path of the character that drives, written by the CartEntry
  driver = "",
}

-- the RigidBody is reached through the entity: the physics registers it
-- after the scripts are read, so a system cannot run over it
local CartSystem = System:extend(Cart, "Transform")

local function stop(entity)
  local body = entity.RigidBody
  if body == nil then return end
  body.linear_velocity = vec3(0, 0, 0)
  body.angular_velocity = vec3(0, 0, 0)
end

function CartSystem:on_possessed(entity, cart, transform)
  log.info("The cart is possessed: move drives it, jump gets off")
end

function CartSystem:on_unpossessed(entity, cart, transform)
  stop(entity)
  log.info("The cart is left where it stands")
end

function CartSystem:update(entity, cart, transform, dt)
  if not entity:has_component("Possessed") then return end
  local body = entity.RigidBody
  if body == nil then return end

  -- the driver gets off in the frame after the press, so that the press
  -- does not reach the character as a jump in the same frame
  if cart._leaving then
    cart._leaving = false
    local driver = world.find_entity(cart.driver)
    if driver ~= nil and world.possess(driver) then return end
  end

  local x, y = input.action_axis2("move")
  local yaw = math.rad(transform.rotation.y)
  body.linear_velocity = vec3(-math.sin(yaw) * y * cart.speed, 0, -math.cos(yaw) * y * cart.speed)
  body.angular_velocity = vec3(0, -x * cart.turn_speed, 0)

  if input.was_action_pressed("jump") and cart.driver ~= "" then cart._leaving = true end
end

return Cart, CartSystem
