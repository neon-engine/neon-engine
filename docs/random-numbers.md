# Random numbers

This note records where a game gets random numbers from, why the engine has
them this way, and what is still open.

**Current decision:** the engine hands a game the random numbers of the
operating system, from its secure generator, through an interface of
neon-core with one implementation that calls the operating system. The engine
itself draws nothing random, and nothing seeds it.

## What it is for

A game's own generators start from it: what is rolled, what drops, what is
built procedurally. The numbers come from the operating system and nobody
can predict or repeat them, so a player cannot steer them with an option, a
setting, or a file.

| Need | How |
|---|---|
| A roll nobody can foresee | Ask the entropy for the bytes, or for a seed |
| A run that can be repeated, such as a replay or a daily run | Take a seed from the entropy once, keep it with the run, and seed the game's own generator from it |
| Identifiers that must not collide | Fill them from the entropy |
| Tokens and keys of networking, later (#144) | The same. The secure generator is what they need |

What must be fair against a player who owns the machine still belongs on a
server (#144). In single player, a player can change anything in memory,
and no source of random numbers changes that.

## What it is not

| It is not | Because |
|---|---|
| A seed of the engine | The engine draws nothing random. Its fixed time step and its physics are deterministic and take no seed, see [physics.md](physics.md#the-time-step) |
| A `--seed` option | A run that is to repeat does so with `--time-step` and a script of input, see [command-line.md](command-line.md). #141 added seeded xoshiro256** numbers with `--seed`, and was closed for this on 2026-10-01 |
| A fallback to the clock | When the operating system gives no entropy, `Fill()` returns false and the backend logs why. It is never quiet about it |
| A generator for gameplay | It gives bytes and a seed. Fast numbers in a range, without bias, are a generator of the game's own, or one the engine adds later, see open questions |

## The interface

| Piece | Location | Role |
|---|---|---|
| `EntropyContext` | [neon-core](../lib/neon-core/neon/random/entropy-context.hpp) | The interface consumers depend on: `Fill(bytes)` fills a span of bytes from the secure generator, and returns false when the operating system cannot |
| `entropy_seed(entropy)` | [neon-core](../lib/neon-core/neon/random/entropy-seed.hpp) | A 64-bit number from eight of its bytes, for seeding a generator. Empty when there was no entropy. The first byte is the lowest on every platform, so the same bytes give the same seed everywhere |
| `OS_Entropy` | [neon-platform](../lib/neon-platform/neon/random/os-entropy.hpp) | The implementation. One class, whose source chooses the call of the operating system when it is compiled |
| `FakeEntropyContext` | [neon-core, testing](../lib/neon-core/neon/testing/fake-entropy-context.hpp) | Hands out bytes a test chooses, and refuses on request |

The interface is as small as it can be: it holds what a game needs to start
its own generator and nothing speculative. A function is added when a
feature requires it.

`main.cpp` creates `OS_Entropy` as it creates the other backends and hands
it to the runtime with `SetEntropy()`. Nothing in the engine draws from it.
A game reaches it through `Runtime::GetEntropy()`, and through Lua once there
is one (#57).

## The backends

| Platform | Call | Notes |
|---|---|---|
| macOS, Linux | `getentropy()` | glibc 2.25 and later, macOS 10.12 and later. It fills at most 256 bytes a call, so the backend calls it as often as it takes |
| Windows | `BCryptGenRandom` with `BCRYPT_USE_SYSTEM_PREFERRED_RNG` | In `bcrypt.dll`, which neon-platform links. Available in the llvm-mingw toolchain the Windows build cross-compiles with |
| The web (#66, #86) | `crypto.getRandomValues()` | Emscripten implements `getentropy()` on top of it, so the POSIX path of the backend serves there unchanged. Standalone WebAssembly (WASI) maps it to `random_get`. Nothing is written for it yet |

The call is chosen with `#if defined(_WIN32)` inside `os-entropy.cpp`, and
nowhere else: a check for the platform is only found inside a backend, see
the [style guide](style-guide.md#architecture-rules).

### Why a library of its own

Every backend library is named after the third-party library it wraps:
SDL2 in neon-sdl2, Jolt in neon-jolt, and so on, and nothing outside such a
library includes a header of its third party. The random numbers call no
third-party library, they call the operating system, and SDL2 has no secure
generator to wrap. neon-core must not call the operating system itself. So
the call sits in neon-platform, the library of the small calls into the
operating system that no other backend covers. It links neon-core and, on
Windows, `bcrypt`.

## Tests

| Library | File | Tests |
|---|---|---|
| neon-core | `random/entropy-seed` | The seed of known bytes, with the first byte lowest; the next seed takes the next bytes; every bit is used; empty without entropy |
| neon-core | `runtime/runtime` | The runtime has no entropy until it is given one |
| neon-platform | `random/os-entropy` | Bytes are filled and are not all zero; two fills differ; nothing to fill is fine; more than 256 bytes are filled; a seed is given |

What comes out is random, so a test can only check that something came out,
and that it is not the same thing twice.

## Open questions

- A generator for gameplay, seeded from the entropy: a whole number or a
  fraction within bounds, without bias, fast enough to call many times a
  frame. xoshiro256** is a candidate, and #141 held one. Is it an interface
  of neon-core, like the entropy, or part of the Lua library a game uses?
- Access from Lua (#57): `Fill`, a seed, or only the generator above?
- Should a game be able to swap the entropy for a fake of its own, for a
  test of the game? The runtime takes any `EntropyContext`, so it can today,
  but nothing documents it as a way of testing a game.
- Should the network code of #144 take its keys from here, or from a
  library of its own that asks the operating system itself?
