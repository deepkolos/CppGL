#pragma once

#include "buffer.h"
#include "constant.h"
#include <vector>
namespace CppGL {
struct TextureBuffer : Buffer {
  int width;
  int height;
  int format;
  int border;
  int dataType;
  int internalFormat;
  TextureBuffer(const void *data, int length, int width, int height, int format,
                int border, int dataType, int internalFormat)
      : Buffer{data, length}, width(width), height(height), format(format),
        border(border), dataType(dataType), internalFormat(internalFormat) {}
};
struct Texture {
  std::vector<TextureBuffer*> mips{};
  int TEXTURE_MIN_FILTER = GL_NEAREST;
  int TEXTURE_MAG_FILTER = GL_NEAREST;
  int TEXTURE_WRAP_S;
  int TEXTURE_WRAP_T;
};

struct TextureUnit {
  Texture *map; // 2d
  Texture *cubeMap;
};
} // namespace CppGL