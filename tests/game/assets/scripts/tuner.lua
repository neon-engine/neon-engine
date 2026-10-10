-- A Tuner is what the controls of assets://ui/controls.ui.yml call when the
-- player changes them: each names the handler `tune` with `on_change`, and
-- the handler is called for the entity that shows the user interface, which
-- carries the Tuner. It says in the log what changed and what it holds now,
-- which tests/runtime-scripts reads.
--
--   Tuner:
--     volume: 20

local Tuner = Component:extend {
  -- what the slider starts at, which the game sets and which calls nothing
  volume = 20,
}

local TunerSystem = System:extend(Tuner)

function TunerSystem:ready(entity, tuner)
  ui.set_number("volume", tuner.volume)
end

-- on_change: tune('volume', $event), and the same for the others
function TunerSystem.handlers:tune(what, event)
  log.info("Tuned", what, "by a", event.kind, "of", event.element, "in", event.interface, "to", event.value)
end

return Tuner, TunerSystem
