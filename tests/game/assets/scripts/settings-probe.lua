-- A SettingsProbe reads the settings of the game and says what it finds, for
-- the test of the `settings` library of the scripts, runtime-scripts. The
-- settings are declared in settings.yml under `game`. It hears of a change
-- with settings.on_change, and makes one itself in its first frame.

local SettingsProbe = Component:extend {
  -- whether the first frame is done
  done = false,
}

local SettingsProbeSystem = System:extend "SettingsProbe"

function SettingsProbeSystem:ready(entity, probe)
  log.info("The probe reads spin_speed as " .. tostring(settings.get("spin_speed")))
  log.info("The probe reads greeting as " .. settings.get("greeting"))
  settings.on_change("spin_speed", function(value)
    log.info("The probe heard spin_speed as " .. tostring(value))
  end)
end

function SettingsProbeSystem:update(entity, probe, dt)
  if probe.done then return end
  probe.done = true
  settings.set("spin_speed", 360)
  -- above the most: refused, which the store says once
  settings.set("spin_speed", 1000)
end

return SettingsProbe, SettingsProbeSystem
