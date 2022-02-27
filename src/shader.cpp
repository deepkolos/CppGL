#include "CppGL/constant.h"
#include "CppGL/math.h"
#include <CppGL/global-state.h>
#include <CppGL/shader.h>

namespace CppGL {
vec4 ShaderSource::texture2D(sample2D textureUint, vec2 uv) {
  auto &textureUintInfo = GLOBAL::GLOBAL_STATE->textureUints[textureUint];
  auto texture = textureUintInfo.map;
  if (texture != nullptr) {
    if (texture->mips.size() != 0) {
      auto mip = texture->mips[0];
      // 左下坐标转左上坐标
      if (texture->TEXTURE_WRAP_S == GL_CLAMP_TO_EDGE)
        uv.x = clamp(uv.x, 0, 1);
      if (texture->TEXTURE_WRAP_T == GL_CLAMP_TO_EDGE)
        uv.y = clamp(uv.y, 0, 1);

      if (texture->TEXTURE_WRAP_S == GL_REPEAT && uv.x < 0 || uv.x > 1)
        uv.x = std::abs(uv.x - (float)(int32_t)(uv.x));
      if (texture->TEXTURE_WRAP_T == GL_REPEAT && uv.y < 0 || uv.y > 1)
        uv.y = std::abs(uv.y - (float)(int32_t)(uv.y));

      uint32_t x = uv.x * mip->width;
      uint32_t y = (1 - uv.y) * mip->height;

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