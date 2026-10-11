# Working in this repository as an agent

This file holds the rules that change how an agent works here and that the
documentation does not say. It is short on purpose; every section points at
the file that explains the detail. Read the pointed-at file before touching
its area. A brief for a subagent cites this file instead of copying it.

- What the engine is and how it is built: [README.md](README.md),
  [docs/development.md](docs/development.md), [docs/quick-start.md](docs/quick-start.md).
- How code is written: [docs/style-guide.md](docs/style-guide.md). It wins
  over any habit from another code base.
- Everything else under [docs/](docs/), one file per subsystem.

## Git and pull requests

- Never check out, branch, or commit in the root checkout. Every piece of
  work, however small, happens in a worktree under `worktrees/`:
  `git worktree add worktrees/<name> -b <type>/<name> origin/main`, then
  `git submodule update --init --recursive` inside it. Several tasks at once
  each get their own worktree.
- Commit as you go, one small commit per feature or fix, and push after each
  step. The history is reviewed commit by commit.
- Pull requests are opened on the Forgejo instance with `fj`, against `main`,
  and nowhere else. GitHub is a mirror that Forgejo pushes to by itself; no
  pull request, issue, or push goes to GitHub.
- An agent never merges a pull request of this repository. The owner reviews
  and merges every one.
- The title starts with `WIP: ` while commits are still expected; the prefix
  is removed when the work is ready for review.
- The body follows `.forgejo/PULL_REQUEST_TEMPLATE.md` exactly: `## Description`
  and `## How to test`. The second section lists the build command, what to
  try in a window, the `--headless` equivalents, and the `ctest` filters of
  the new tests. Close issues with one keyword each: `Fixes #71, fixes #155`.
- `main` only ever receives squashed commits. A stack of pull requests goes
  through a `stack/<topic>` branch: children target it and are merged with
  merge commits, the stack is squashed into `main` by one pull request whose
  title names every child and whose body carries every `Fixes #N`. Never
  merge children into the stack branch yourself.
- When a parent is squashed, merge `origin/main` into the child and push
  normally. No rebase, no force-push.
- After merging `origin/main` into a branch, never `git add -A`: a submodule
  that `main` removed leaves its folder behind and would be staged back.
- Keep a pull request small enough to read in one sitting. Name any new
  concept (a path prefix, an interface function) in the description so the
  reviewer does not have to find it in the diff.

## Issues

- Every bug found while working becomes an issue, without asking: what
  happens, how it was found, what is expected, with `type: bug` and the
  fitting `area:` labels. Fixed in the pull request at hand? File it anyway and
  add `Fixes #N`. Search `fj issue search "<text>"` first; duplicates happen.
- Milestones are labels: `milestone: v0.5 MVP`, `milestone: v1.0`,
  `milestone: v2.0`, set with `fj issue edit <N> labels -a "<label>"`. A new
  issue gets `milestone: v1.0` unless the owner says otherwise.
- Forgejo issues are the source of truth for what is planned.
  `docs/roadmap.md` is not kept in step with them.
- Never read the `fj` token or any other credential from disk for API calls
  on your own initiative.

## Code

- Third-party libraries are used only behind interfaces of `lib/neon-core`,
  implemented in a backend library of their own, encapsulated at the CMake
  target as well: an application or test names only `neon-*` targets and
  headers. `main.cpp` is the composition root and nothing more.
- No raw `new` or `delete`; heap allocation only where something forces it.
- One type per file. A file-local helper lives in an anonymous namespace that
  opens with one comment line saying what it holds.
- What differs between the shipped runtime and the editor is a separate
  object owned by one of them, never a compile flag. Conditional compilation
  is for platforms and backends.
- Game C++ builds standalone against a prebuilt runtime through
  `cmake/NeonSdk.cmake`; it never becomes a target inside the engine tree.
  The C boundary carries no engine types.
- A path prefix is defined in one place only (the file system for
  `assets://`, `user://`, `output://`, `extensions://`, `engine://`;
  `TextureSource` for the in-memory ones). Virtual paths use forward slashes
  on every platform. Content valid on one platform is valid on all.
- `engine://` holds what the runtime ships; the runtime knows no game.
  `extensions://<name>/` holds code and what that code runs on. Game content
  lives in `assets://`. New projects get working defaults, not empty files.
- Prefabs are not scenes. Exporting a game belongs to the editor.
- A failure at start-up ends with exit code 1 and the reason in the log.
  Never a black screen.
- Scripts are LuaJIT and nothing else: Lua 5.1 dialect, no `ffi`, no `jit`.
- Adding a library needs the owner's approval first, per library.
- American spelling in code, comments, docs, recipes, commits, and issues.
- No ambiguous Unicode in anything committed: no en or em dash, curly quote,
  non-breaking space, or ellipsis character. Check before committing with
  `git grep -n -P "[\x{2013}\x{2014}\x{2018}\x{2019}\x{201C}\x{201D}\x{00A0}\x{2026}]" -- . ':!external'`.

## Content

- In recipes a component left at its defaults is `Transform: Default`,
  never `{}`.
- A flow mapping `{type: label, text: Hi}` is fine while it stays short. One
  that runs long is hard to read and is written in block style, one key a
  line. Lists of numbers `[1, 2, 3]` are always fine.
- Engine tests use only assets the owner made or assets a script in `tools/`
  generates from numbers. Never download an asset into the repository, and
  never author artwork, models, or music yourself. A game project's own
  assets may appear in that project's tests.
- Visible or playable engine features get an exhibit in the museum project
  in the same pull request. Infrastructure does not. The museum's design is
  the owner's; do not restyle it.
- `README.md` never names the Forgejo server. The engine is described as
  serving people and agents alike, never as AI-first or people-first.
  Funding is "sponsorship or paid builds"; donations are not set up.

## Building and running

- Every `NeonRuntime` run goes under a timeout enforced from outside, sized to
  the run. On macOS without coreutils:
  `perl -e 'alarm shift; exec @ARGV' 20 bin/debug/darwin-arm64/NeonRuntime/NeonRuntime --headless ...`.
  On Linux, `timeout`. A hang is a bug: file it.
- Headless renders and the `runtime-*` application tests run one at a time
  across every agent (`ctest -j 1 -R runtime-`). Unit tests may run in
  parallel (`-E runtime-`). Do not start several full builds at once.
- Never `cd` in a shell command that runs in parallel with another. Use
  `git -C`, `cmake --build <dir>`, `ctest --test-dir <dir>`, absolute paths.
- A run that presses Apply in a settings menu writes the real user file under
  `~/Library/Application Support/neon-engine/<app>/`. Back it up before and
  restore it after. Setting `HOME` does not redirect it on macOS.
- Testing in a real window and on high-density displays is the owner's job.
  Build so the headless tests prove the change and give the owner a checklist.
- Never install software (brew, pip, docker, npm) without the owner's
  permission for that exact package.

## Not without being asked

Do not start, scaffold, or branch for these: a Metal renderer, the editor,
the web build, anything voxel-specific in the engine, a redesign of the
museum, a repository-wide style sweep from one remark.
When something in a review reads as an objection, ask what bothers the
reviewer and offer the smallest rule that answers it.
