-- A Track names a piece of music for the Jukebox of scripts/jukebox.lua:
-- the entity carries a SoundSource as well, which the jukebox fades in when
-- the player chooses the name, and out otherwise.
--
--   Track:
--     name: chilled

local Track = Component:extend {
  name = "",
}

return Track
