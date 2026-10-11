-- A Gaze tells the HUD whether the camera looks at something it can use:
-- it reads the RayCast of its entity every frame and sets the flag of the
-- user interface it names, which the HUD binds its dot and its ring to
-- with `hidden: "{pointing}"` and `hidden: "{!pointing}"`. What the HUD
-- shows for it, and whether it shows anything, is the game's to decide
-- here and in its files, not the engine's.
--
--   Gaze:
--     flag: pointing

local Gaze = Component:extend {
  -- the flag of the user interface that follows the ray
  flag = "pointing",
}

local GazeSystem = System:extend("Gaze", "RayCast")

function GazeSystem:ready(entity, gaze, ray)
  gaze._was = nil
end

function GazeSystem:update(entity, gaze, ray, dt)
  -- said when it changes, and not in every frame
  if ray.is_hitting ~= gaze._was then
    gaze._was = ray.is_hitting
    ui.set_flag(gaze.flag, ray.is_hitting)
  end
end

return Gaze, GazeSystem
