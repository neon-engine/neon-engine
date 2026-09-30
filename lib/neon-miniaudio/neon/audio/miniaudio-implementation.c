/* The implementation of miniaudio, which is one file that is compiled once.
   stb_vorbis is declared before it and defined after it, which is how
   miniaudio learns to read Ogg Vorbis. */

#define STB_VORBIS_HEADER_ONLY
#include <extras/stb_vorbis.c>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#undef STB_VORBIS_HEADER_ONLY
#include <extras/stb_vorbis.c>
