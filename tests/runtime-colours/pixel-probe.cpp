// Prints the colours of pixels of a PNG image, for the scripts of tests
// that look at what the runtime drew, which CMake cannot read itself.
//
//     pixel-probe <image.png> <x>,<y> ...
//
// prints a line for each pixel: its place, and red, green, blue, and alpha
// from 0 to 255, as in `206,279 188 0 0 255`.

#include <cstdio>
#include <string>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main(const int argc, char **argv)
{
  if (argc < 3)
  {
    std::fprintf(stderr, "Usage: pixel-probe <image.png> <x>,<y> ...\n");
    return 2;
  }

  int width = 0;
  int height = 0;
  int channels = 0;
  unsigned char *pixels = stbi_load(argv[1], &width, &height, &channels, 4);
  if (pixels == nullptr)
  {
    std::fprintf(stderr, "Could not read the image %s\n", argv[1]);
    return 1;
  }

  int result = 0;
  for (int i = 2; i < argc; i++)
  {
    int x = 0;
    int y = 0;
    if (std::sscanf(argv[i], "%d,%d", &x, &y) != 2 || x < 0 || y < 0 || x >= width || y >= height)
    {
      std::fprintf(stderr, "%s is no place in an image of %d by %d\n", argv[i], width, height);
      result = 1;
      continue;
    }

    const unsigned char *pixel = pixels + (static_cast<std::size_t>(y) * width + x) * 4;
    std::printf("%d,%d %d %d %d %d\n", x, y, pixel[0], pixel[1], pixel[2], pixel[3]);
  }

  stbi_image_free(pixels);
  return result;
}
