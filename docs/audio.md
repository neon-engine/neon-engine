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
| `group` | The [group](#groups) whose volume the sound is played at | `effects` |

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

### Groups

Every sound belongs to a group, and is played at the volume of its group on
top of its own. There are three from the start:

| Group | For |
|---|---|
| `music` | Music |
| `effects` | Everything else that is heard, and what a sound belongs to unless it says otherwise |
| `voices` | Speech |

The volume of everything, which the `SoundListener` sets, is on top of the
groups. A sound of a group that is not there is reported and played among the
effects, which finds a name that is misspelt, also in a run without sound.
The group of a sound is read when the sound is created. Changing `group`
later does not move it.

A settings menu changes the volume of a group through the `AudioContext`,
which a system of a game is handed when it is made, as it is handed the
physics:

```cpp
audio->SetGroupVolume(neon::sound_group::music, 0.4f);
float music_volume = audio->GetGroupVolume(neon::sound_group::music);
```

A game adds groups of its own before the scene that uses them is played:

```cpp
audio->AddGroup("ambience");
```

The groups and their volumes are kept while the game runs, also when the
audio is started again. Keeping them from one run to the next is up to the
settings of the game.

### Fading

A game fades a sound through its component, as it plays and stops it:

```cpp
auto *music = store.Get<neon::SoundSource>(radio);

music->FadeIn(2.0);        // plays it from its start, from silence up over 2 seconds
music->FadeTo(0.3f, 1.0);  // to 0.3 of its volume over 1 second, for speech over it
music->FadeOut(3.0);       // to silence over 3 seconds, and stops it then

// from one piece of music to the next over 4 seconds
neon::Crossfade(*store.Get<neon::SoundSource>(day), *store.Get<neon::SoundSource>(night), 4.0);
```

| | What happens to `playing` |
|---|---|
| `FadeIn` | Set to true at once |
| `FadeTo` | Nothing. A sound that fades to silence keeps playing |
| `FadeOut` | True while it fades. The engine sets it to false once the sound has stopped |

A fade is on top of `volume`, which stays as it was written. Playing a sound
again starts it at its volume, without what it was faded to. A fade of a sound
that does not play is forgotten, and a fade replaces the one before.

Fades go by the samples that are mixed. Without a sound card those are as
many as each frame lasted, so a run with a fixed time step fades the same
every time.

The `AudioContext` has the same fades by the id of a sound, for sounds that
are not in the world: `FadeIn`, `FadeTo`, `FadeOut`, and `Crossfade`.

### From a settings menu

Values of a user interface can set what is heard without code of the game,
through two components that `UiAudio` acts on:

```yaml
# the slider of the menu is the volume of the music, from 0 to 100
- name: music volume
  components:
    UiVolume:
      value: music
      group: music

# plays while the menu's value `track` is `chilled`, and fades out otherwise
- name: chilled
  components:
    Transform: {}
    SoundSource:
      sound: assets://sounds/Electronic/Chilled (RT 3.428)/Electronic Chilled main.wav
      group: music
      looping: true
    UiSoundSwitch:
      value: track
      equals: chilled
      fade: 2
```

| Component | Field | What it does | Default |
|---|---|---|---|
| `UiVolume` | `value` | The name of the value, as files write it in `{music}` | None. It has to be written |
| | `group` | The group whose volume it sets | None. It has to be written |
| | `full` | What the value is at a volume of 1 | `100` |
| `UiSoundSwitch` | `value` | The name of the value | None. It has to be written |
| | `equals` | What the value is while the sound plays | None. It has to be written |
| | `fade` | Seconds that fading in and out take | `1` |

A volume is handed to the audio only when it changes. A sound that is not
chosen when the scene starts is silent from the start, and the one that is
fades in. One entity for every piece of music, each with its own word, and a
choice of radio buttons in the menu, fade from one piece to the next. The
settings demo does this:

```
NeonRuntime --scene assets://scenes/settings-demo.scene.yml
```

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
| `Headless_AudioSystem` | neon-core | Plays nothing and reads no files. It keeps track of what it is told: what plays, the volumes of the groups, and when a sound that fades out stops |
| `MA_AudioSystem` | neon-miniaudio | The implementation |
| `SoundSource`, `SoundListener` | neon-core | The components |
| `AudioPlayback` | neon-core | The system that keeps the audio in line with the components, and hands on the fades a game asks for |

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
| Groups are known by name, with three from the start | A game adds groups of its own without the engine knowing them, and a scene file names a group as it names a sound |
| A group is a sound group of miniaudio | The sounds of a group are mixed into it, and its volume is set once for all of them. It has no place in the world, so its sounds are placed each for itself |
| A fade is on top of the volume of a sound | `AudioPlayback` hands the volume of a source over every frame, which would undo a fade that set the volume |
| A game fades a source through its component | So that `playing` stays true to what is heard. A fade in through the audio would play a sound that the component says is not playing |
| Sounds are placed after the entities are | A sound is heard where its entity is drawn. Before, it was heard where it was a frame ago, and in the first frame from the origin, which made it loud at the start |

## How it was checked

| Check | Result |
|---|---|
| 38 checks of the audio system without a sound card | Pass. They cover loudness, volume, ending, looping, pitch, distance, two sounds of one file, files that are missing or broken, the components, and two runs being the same |
| WAV, FLAC, and Ogg Vorbis | Read and heard |
| The demo scene without a window | The sound is read and mixed. The image is the same as before, byte for byte |
| The demo scene with a window | The sound card is opened and the run ends cleanly |

Not checked: MP3, for lack of a file to try. How it sounds, since none of this
was listened to. Linux and Windows.

The checks are unit tests now (#72). `ma-audio-system.test.cpp` mixes without
a sound card and measures the level of what would be heard,
`headless-audio-system.test.cpp` covers the audio that plays nothing, and
`audio-playback.test.cpp` covers the system that plays the sounds of the world,
against a mock of the audio.

Groups and fades are measured the same way. A group at half its volume halves
the level, and the volume of everything halves it again. A fade is measured
frame by frame: from 1 to 0.5 over half a second the level of a tone of 0.5
falls by 0.025 every three frames and ends at 0.25. A fade out ends with the
sound stopped after as much time as the fade was given, and a crossfade lowers
one sound as it raises the other, each heard on its own with the group of the
other silent.

## Open questions

- Steam Audio. It takes a sound and gives back one that is placed in the
  world, with the shape of the room. In miniaudio that is a node between a
  sound and the output. It needs a backend library of its own, and the
  listener and the geometry of the scene handed to it.
- Music that is long. A file is held in memory as it is on disk, and decoded
  while it plays. That suits minutes of compressed music. It does not suit
  hours, which want to be read piece by piece through the file system.
- Fading a whole group, such as music that is lowered while a voice speaks.
  miniaudio can fade a sound group as it fades a sound. Groups within groups
  are left out as well.
- Sounds that are played once and forgotten, such as a shot, without an entity
  for each.
- An option to run without audio, and one to choose the sound card.
- The web. miniaudio has a backend for it, which was not tried.
