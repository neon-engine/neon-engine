-- A CartEntry possesses the cart it names when the possessed character
-- walks into the Trigger of its entity, and tells the Cart who drives, so
-- that the cart hands the driver back. See docs/possession.md.
--
--   CartEntry:
--     cart: cart

local CartEntry = Component:extend {
  -- the path of the entity that carries the Cart
  cart = "",
}

local CartEntrySystem = System:extend(CartEntry)

function CartEntrySystem:on_trigger_enter(entity, entry, other)
  if not other:has_component("Possessed") or not other:has_component("FirstPersonController") then return end

  local cart = world.find_entity(entry.cart)
  if cart == nil or cart.Cart == nil then
    log.warn("The plate names no cart: " .. entry.cart)
    return
  end

  cart.Cart.driver = other.path
  world.possess(cart)
end

return CartEntry, CartEntrySystem
