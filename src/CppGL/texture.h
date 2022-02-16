#pragma once

namespace CppGL {
struct Texture {};
static int TEXTURE0 = 0;
struct TextureUnit {
  Texture *map; // 2d
  Texture *cubeMap;
};
} // namespace CppGL