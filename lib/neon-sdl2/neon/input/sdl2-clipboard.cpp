#include "sdl2-clipboard.hpp"

#include <SDL.h>

namespace neon
{
  bool SDL2_Clipboard::HasText()
  {
    return SDL_HasClipboardText() == SDL_TRUE;
  }

  std::string SDL2_Clipboard::GetText()
  {
    // SDL hands over a copy that is to be freed, and an empty text when
    // there is none
    char *text = SDL_GetClipboardText();
    if (text == nullptr) { return ""; }

    std::string result(text);
    SDL_free(text);
    return result;
  }

  bool SDL2_Clipboard::SetText(const std::string &text)
  {
    return SDL_SetClipboardText(text.c_str()) == 0;
  }
} // neon
