#include <CppGL/global-state.h>
#include <CppGL/shader.h>

namespace CppGL {
vec4 ShaderSource::texture2D(sample2D textureUint, vec2 uv) {
  auto &textureUintInfo = GLOBAL::GLOBAL_STATE->textureUints[textureUint];

  if (textureUintInfo.map != nullptr) {
    if (textureUintInfo.map->mips.size() != 0) {
      auto mip = textureUintInfo.map->mips[0];
      // 左下坐标转左上坐标
      vec2 uvClamped = clamp(uv, 0, 1);
      int x = uvClamped.x * mip->width;
      int y = (1 - uvClamped.y) * mip->height;

      // std::cout << x << "," << y << std::endl;

      if (mip->format == GL_LUMINANCE) {
        if (mip->dataType == GL_UNSIGNED_BYTE) {
          uint8_t *ptr = (uint8_t *)mip->data;
          uint8_t pixel = ptr[x + y * mip->width];
          float pixelFloat = (float)pixel / 255;
          return {pixelFloat, pixelFloat, pixelFloat, 1};
          // return {1, 1, 1, 1};
        }
      }

      if (mip->format == GL_RGBA) {
        if (mip->dataType == GL_UNSIGNED_BYTE) {
          uint8_t *ptr = (uint8_t *)mip->data;
          int offset = (x + y * mip->width) * 4;
          float r = (float)(ptr[offset]) / 255;
          float g = (float)(ptr[offset + 1]) / 255;
          float b = (float)(ptr[offset + 2]) / 255;
          float a = (float)(ptr[offset + 3]) / 255;
          return {r, g, b, a};
          // return {1, 1, 1, 1};
        }
      }
    }
  }
  return {1, 1, 1, 1};
}
} // namespace CppGL