# Audio

This note records how Neon Engine plays sound, what was weighed, and what is
still open.

**Current decision:** the engine has its own interface for audio, with one
implementation built on [miniaudio](https://miniaud.io). SDL_mixer and SoLoud
are removed. Steam Audio stays in the repository for spatial sound and is not
connected yet.

## In a scene

An entity that carries a `SoundSource` is a source of sound. What is heard is
heard from the entity that carries a `SoundListener`, which usually sits on the
camera.

```yaml
- name: sphere
  components:
    Transform:
      position: [1.2, 1, -2]
    SoundSource:
      sound: assets://sounds/hum.wav
      looping: true
      spatial: true

- name: player
  components:
    Transform:
      position: [0, 0, 2]
  children:
    - name: camera
      components:
        Transform: {}
        Camera: {}
        SoundListener: {}
```

**SoundSource**

| Name | Holds | Default |
|---|---|---|
| `sound` | Virtual path of the file | None. It has to be written |
| `playing` | Whether the sound plays. True plays it when the entity joins the world | `true` |
| `looping` | Whether it starts over when it has ended | `false` |
| `volume` | 1 is the loudness of the file, 0 is silence | `1` |
| `pitch` | 1 is the speed of the file, 2 is twice as fast and an octave higher. Above 0 | `1` |
| `spatial` | Whether the sound has a place in the world | `false` |
| `min_distance` | Up to this distance the sound is as loud as it gets | `1` |
| `max_distance` | From this distance on it does not get quieter | `100` |

A sound with a place is quieter from further away, is heard from the side it
is on, and shifts in pitch when it moves towards the listener or away. A sound
without a place is heard the same everywhere, which is what music and the
sounds of a menu want.

**SoundListener**

| Name | Holds | Default |
|---|---|---|
| `volume` | The loudness of everything that is heard | `1` |

### From code

A game starts and stops a sound through the component:

```cpp
store.Get<neon::SoundSource>(door)->playing = true;
```

The engine sets `playing` to false when a sound that does not loop has ended.
Setting it to true again plays the sound from its start.

### Files

| Format | Read by |
|---|---|
| WAV | miniaudio |
| FLAC | miniaudio |
| MP3 | miniaudio |
| Ogg Vorbis | stb_vorbis, which miniaudio ships |

## What exists today

| Piece | Location | Role |
|---|---|---|
| `AudioContext` | neon-core | What the rest of the engine sees: create, play, stop, place, and listen |
| `AudioSystem` | neon-core | Base class of backends. Adds the lifecycle |
| `Headless_AudioSystem` | neon-core | Plays nothing and reads no files. It keeps track of what it is told |
| `MA_AudioSystem` | neon-miniaudio | The implementation |
| `SoundSource`, `SoundListener` | neon-core | The components |
| `AudioPlayback` | neon-core | The system that keeps the audio in line with the components |

Nothing outside neon-miniaudio includes a header of miniaudio.

### Without a sound card

| `AudioOutput` | What happens |
|---|---|
| `Device` | miniaudio mixes on a thread of its own and hands the result to the sound card |
| `None` | There is no sound card and no thread. Sounds are read and mixed all the same, as much as each frame lasted |

NeonRuntime uses `None` when it runs with `--headless`. A run without a window
therefore finds a sound that is missing or broken, and with a fixed time step
it mixes the same every time.

When no sound card can be opened, the engine says so and goes on as with
`None`. A game starts on a machine without one.

`AudioContext::GetOutputLevel()` tells how loud what was last mixed is. It is
how the checks below hear without a sound card, and what a meter in the editor
would show.

## What was weighed

| | SDL_mixer | SoLoud | miniaudio |
|---|---|---|---|
| Mixing | Fixed channels, one piece of music at a time | Voices and buses with filters | A graph of nodes: sounds, groups, effects |
| Sound with a place | Panning and distance | Yes | Yes, with cones and a shift in pitch |
| Processing of its own for one sound | Through callbacks | Through filters | A node, which is where Steam Audio fits |
| Without a sound card | No | Yes | Yes |
| The version that was in the repository is from | 2024 | 2020 | 2026 |
| License | zlib | zlib | Public domain, or MIT No Attribution |
| Size | A library and its decoders | A library | One file |

Ruled out: OpenAL Soft is under the LGPL, which does not go with static
linking. FMOD and Wwise are not open and cannot be part of the repository.
They could be backends of their own, which the interface allows.

## How the interface is shaped

| Decision | Reason |
|---|---|
| A sound is known by an id | As with what the renderer draws. No type of the backend crosses the interface |
| A file is read through the file system | A sound in an archive has no path the operating system can open |
| A file is read once, however many sounds are made from it | Ten sources of one step sound hold the file once |
| A sound that cannot be created is tried once | Trying every frame would read the file and report it every frame |
| The world tells the audio how far it advanced | Audio without a sound card has no clock of its own |
| A source releases its sound when it leaves the world | Through the hook of the component, as `Renderable` does |

## How it was checked

| Check | Result |
|---|---|
| 38 checks of the audio system without a sound card | Pass. They cover loudness, volume, ending, looping, pitch, distance, two sounds of one file, files that are missing or broken, the components, and two runs being the same |
| WAV, FLAC, and Ogg Vorbis | Read and heard |
| The demo scene without a window | The sound is read and mixed. The image is the same as before, byte for byte |
| The demo scene with a window | The sound card is opened and the run ends cleanly |

Not checked: MP3, for lack of a file to try. How it sounds, since none of this
was listened to. Linux and Windows.

The checks are not part of the repository. They move into the unit tests once
those are merged.

## Open questions

- Steam Audio. It takes a sound and gives back one that is placed in the
  world, with the shape of the room. In miniaudio that is a node between a
  sound and the output. It needs a backend library of its own, and the
  listener and the geometry of the scene handed to it.
- Music that is long. A file is held in memory as it is on disk, and decoded
  while it plays. That suits minutes of compressed music. It does not suit
  hours, which want to be read piece by piece through the file system.
- Groups, such as music, effects, and voices, each with a volume of its own
  that a settings menu changes.
- Sounds that are played once and forgotten, such as a shot, without an entity
  for each.
- Fading in and out, and from one piece of music to another.
- Where the sound is a frame behind. The system runs before entities are
  placed in the world, so a source is heard where it was a frame ago.
- An option to run without audio, and one to choose the sound card.
- The web. miniaudio has a backend for it, which was not tried.
