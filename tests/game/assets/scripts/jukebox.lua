-- A Jukebox fades between the pieces of music of a scene when the player
-- chooses one: the dropdown of assets://ui/music.ui.yml names its handler
-- `choose_track` with `on_change`, and the handler is called for the entity
-- that shows the file, which carries the Jukebox. Every piece is an entity
-- with a SoundSource and a Track that says its name; the one whose name is
-- chosen fades in, the others fade out.
--
--   Jukebox:
--     fade: 2
--
--   Track:
--     name: chilled

local Jukebox = Component:extend {
  -- seconds that fading from one piece to the next takes
  fade = 2.0,
}

-- the piece whose name is chosen fades in, the others fade out
local function choose(name, fade)
  for piece, track in world.each("Track") do
    if track.name == name then
      audio.fade_in(piece, fade)
    else
      audio.fade_out(piece, fade)
    end
  end
end

local JukeboxSystem = System:extend(Jukebox)

-- the piece the dropdown starts with plays from the start, the rest wait
function JukeboxSystem:ready(entity, jukebox)
  choose("chilled", jukebox.fade)
end

-- on_change: choose_track($event), with the name chosen as the value
function JukeboxSystem.handlers:choose_track(event)
  log.info("The track is", event.value)
  choose(event.value, self.Jukebox.fade)
end

return Jukebox, JukeboxSystem
