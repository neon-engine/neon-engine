#ifndef SDL2_MAIN_HPP
#define SDL2_MAIN_HPP

// The entry point of an application whose window and input are SDL2's.
//
// The source file that holds `int main(int argc, char *argv[])` includes
// this, and its target links neon-sdl2-main. SDL2 then provides what the
// platform starts the application by, WinMain on Windows, and calls that
// main(), which it renames to SDL_main behind this include. It requires the
// full argc/argv signature.
//
// This is for applications alone. A test links neon-sdl2 and has the main()
// of GoogleTest, which is why the entry point is a target of its own and no
// part of neon-sdl2.

#include <SDL_main.h>

#endif //SDL2_MAIN_HPP
