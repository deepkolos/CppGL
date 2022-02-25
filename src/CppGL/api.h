#pragma once

#include "attribute.h"
#include "buffer.h"
#include "constant.h"
#include "global-state.h"
#include "math.h"
#include "program.h"
#include "rttr/property.h"
#include "rttr/string_view.h"
#include "rttr/type.h"
#include "rttr/variant.h"
#include "shader.h"
#include "texture.h"
#include "vertex-array.h"
#include <_types/_uint8_t.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <malloc/_malloc.h>
#include <map>
#include <rttr/type>
#include <utility>
#include <vector>

namespace CppGL {
using str = rttr::string_view;

void glClear(int mask);
namespace helper {
inline Texture *getTextureFrom(int location) {
  Texture *target = nullptr;
  if (location == GL_TEXTURE_2D) {
    target =
        GLOBAL_STATE.textureUints[GLOBAL_STATE.ACTIVE_TEXTURE - GL_TEXTURE0]
            .map;
  }
  return target;
}
inline void setUniform(
    int location,
    std::function<void(rttr::property &, rttr::property &, ShaderSource *,
                       ShaderSource *, rttr::type &, rttr::type &)>
        fn) {
  Program *program = GLOBAL_STATE.CURRENT_PROGRAM;

  if (program == nullptr)
    return;

  auto vertexShader = program->vertexShader->source;
  auto fragmentShader = program->fragmentShader->source;
  auto vertexTypeInfo = vertexShader->get_derived_info().m_type;
  auto fragmentTypeInfo = fragmentShader->get_derived_info().m_type;

  for (auto [name, info] : program->unifroms) {
    if (info.location == location) {
      auto vertexProp = vertexTypeInfo.get_property(name);
      auto fragmentProp = fragmentTypeInfo.get_property(name);
      fn(vertexProp, fragmentProp, vertexShader, fragmentShader, vertexTypeInfo,
         fragmentTypeInfo);
      return;
    }
  }
}
inline float if0Be1(float a) { return a == 0 ? 1 : a; }
inline void draw(int mode, int first, int count, int dataType,
                 const void *indices) {
  auto program = GLOBAL_STATE.CURRENT_PROGRAM;
  auto vao = GLOBAL_STATE.VERTEX_ARRAY_BINDING;
  auto fbo = GLOBAL_STATE.FRAMEBUFFER_BINDING;
  auto vertexShader = program->vertexShader->source;
  auto fragmentShader = program->fragmentShader->source;
  auto vertexTypeInfo = vertexShader->get_derived_info().m_type;
  auto fragmentTypeInfo = fragmentShader->get_derived_info().m_type;
  const auto &viewport = GLOBAL_STATE.VIEWPORT;
  const int width = (int)viewport.z;
  const int height = (int)viewport.w;
  std::vector<vec4> clipSpaceVertices(count);

  if (vao == nullptr)
    vao = &DEFAULT_VERTEX_ARRAY;
  if (fbo == nullptr)
    fbo = &DEFAULT_FRAMEBUFFER;

  const void *indicesPtr = nullptr;
  if (indices != nullptr)
    indicesPtr = indices;
  else if (vao->indexBuffer != nullptr)
    indicesPtr = vao->indexBuffer->data;

  const uint8_t *indicesU8Ptr = (uint8_t *)indicesPtr;
  const uint16_t *indicesU16Ptr = (uint16_t *)indicesPtr;

  /**
   * @brief 分配varying内存 count * (varying size 总和)
   * |               内存布局                |
   * | count0             count1            |
   * | varyingA varyingB  varyingA varyingB |
   */
  int varyingNum = 0;
  int varyingSizeSumU8 = 0;
  std::map<str, int> varyingOffsetMap;
  for (auto &prop : vertexTypeInfo.get_properties())
    if (prop.get_metadata(0).get_value<ShaderSourceMeta>() ==
        ShaderSourceMeta::Varying) {
      auto size = prop.get_metadata(1).get_value<size_t>();
      varyingOffsetMap[prop.get_name()] = varyingSizeSumU8;
      varyingSizeSumU8 += size;
      varyingNum++;
    }
  uint8_t *const varyingMemU8 = (uint8_t *)malloc(varyingSizeSumU8 * count);
  uint8_t *const varyingLerpedMemU8 = (uint8_t *)malloc(varyingSizeSumU8);

  /**
   * @brief 分配frameBuffer zBuffer
   */
  if (nullptr == fbo->COLOR_ATTACHMENT0.attachment &&
      fbo == &DEFAULT_FRAMEBUFFER) {
    // 初始化framebuffer
    {
      int length = sizeof(vec4) * width * height;
      auto texture = new Texture();
      texture->mips.push_back(new TextureBuffer{malloc(length), length, width,
                                                height, GL_RGBA, 0, GL_FLOAT,
                                                GL_RGBA});
      fbo->COLOR_ATTACHMENT0 = {AttachmentType::COLOR_ATTACHMENT0, 0, 0,
                                texture};
    }
    // 初始化zbuffer
    {
      int length = sizeof(float) * width * height;
      auto texture = new Texture();
      texture->mips.push_back(new TextureBuffer{
          malloc(length), length, width, height, GL_DEPTH_COMPONENT32F, 0,
          GL_FLOAT, GL_DEPTH_COMPONENT32F});
      fbo->DEPTH_ATTACHMENT = {AttachmentType::DEPTH_ATTACHMENT, 0, 0, texture};
    }
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  }

  // TODO resize
  auto frameBufferTextureBuffer =
      fbo->COLOR_ATTACHMENT0.attachment->mips[fbo->COLOR_ATTACHMENT0.level];
  auto zBuffer = static_cast<float *>(const_cast<void *>(
      fbo->DEPTH_ATTACHMENT.attachment->mips[fbo->DEPTH_ATTACHMENT.level]
          ->data));

  /**
   * @brief 循环处理顶点
   * 0. 读取attribute 设置到vertex shader
   * 1. 执行vertex shader
   * 2. 收集varying gl_Position
   */
  for (int ii = 0; ii < count; ii++) {
    int i = ii;
    if (dataType == GL_UNSIGNED_SHORT)
      i = indicesU16Ptr[ii];
    if (dataType == GL_UNSIGNED_BYTE)
      i = indicesU8Ptr[ii];

    // 遍历shader里的attribute列表并更新每一轮的值
    for (auto &[name, attr] : program->attributes) {
      auto &info = vao->attributes[attr.location];
      if (!info.enabled)
        continue; // TODO 写入默认值

      size_t componentLen;
      size_t stride = info.stride;

      switch (info.type) {
      case GL_FLOAT:
        componentLen = sizeof(float);
        break;
      case GL_UNSIGNED_BYTE:
        componentLen = sizeof(uint8_t);
        break;
      }

      if (stride == 0)
        stride = info.size * componentLen;

      auto ptr = static_cast<const char *>(info.buffer->data);
      ptr += info.offset + stride * i;

      // 写入attribute到shader的变量
      auto prop = vertexTypeInfo.get_property(name);
      auto var = prop.get_value(*vertexShader);
      auto const varPtr = var.get_value<float *>();
      if (info.type == GL_UNSIGNED_BYTE && info.normalized) {
        auto varPtrI = varPtr;
        for (int j = 0; j < info.size; j++) {
          *varPtrI = static_cast<float>(*((uint8_t *)(ptr) + j)) / 255;
          varPtrI++;
        }
      } else {
        memcpy(varPtr, ptr, stride);
      }
      auto sizeU8 = prop.get_metadata(1).get_value<int>();
      if (sizeU8 == sizeof(vec4) && info.size == 3) {
        *(varPtr + 3) = 1;
      }
      if (sizeU8 == sizeof(vec4) && info.size == 2) {
        *(varPtr + 2) = 0;
        *(varPtr + 3) = 1;
      }
      if (sizeU8 == sizeof(vec4) && info.size == 1) {
        *(varPtr + 1) = 0;
        *(varPtr + 2) = 0;
        *(varPtr + 3) = 1;
      }
    }

    // 执行vertex shader
    vertexTypeInfo.get_method("main").invoke(vertexShader);

    // 收集gl_Position
    clipSpaceVertices[ii] = vertexShader->gl_Position;

    // 收集varying
    size_t offsetU8 = 0;
    for (auto &prop : vertexTypeInfo.get_properties()) {
      if (prop.get_metadata(0).get_value<ShaderSourceMeta>() ==
          ShaderSourceMeta::Varying) {
        auto src = prop.get_value(*vertexShader).get_value<uint8_t *>();
        auto dst = varyingMemU8 + (ii * varyingSizeSumU8) + offsetU8;
        auto sizeU8 = prop.get_metadata(1).get_value<int>();
        memcpy(dst, src, sizeU8);
        offsetU8 += sizeU8;
      }
    }
  }

  auto viewportMatrix = getViewportMatrix(viewport);

  if (mode == GL_TRIANGLES) {
    for (int vertexIndex = 0; vertexIndex < count; vertexIndex += 3) {
      triangle triangleClip{clipSpaceVertices[vertexIndex],
                            clipSpaceVertices[vertexIndex + 1],
                            clipSpaceVertices[vertexIndex + 2]};
      float *varyingA =
          (float *)(varyingMemU8 + vertexIndex * varyingSizeSumU8);
      float *varyingB =
          (float *)(varyingMemU8 + (vertexIndex + 1) * varyingSizeSumU8);
      float *varyingC =
          (float *)(varyingMemU8 + (vertexIndex + 2) * varyingSizeSumU8);
      /**
       * @brief 透视除法
       */
      if (triangleClip.a.w == 0)
        assert(triangleClip.a.w != 0);
      vec3 triangleClipVecW{if0Be1(triangleClip.a.w), if0Be1(triangleClip.b.w),
                            if0Be1(triangleClip.c.w)};
      vec3 triangleClipVecZ{triangleClip.a.z, triangleClip.b.z,
                            triangleClip.c.z};
      // 把齐次坐标系下转为正常坐标系 TODO 理解
      vec3 triangleClipVecZDivZ = triangleClipVecZ / triangleClipVecW;
      triangle triangleViewport = triangleClip * viewportMatrix;
      triangle triangleProjDiv =
          triangleViewport.perspectiveDivide(triangleClipVecW);
      /**
       * @brief 寻找三角形bounding box
       */
      box2 boundingBox = triangleProjDiv.viewportBoundingBox(viewport);
      // box2 boundingBox = {{0, 0}, {(float)width, (float)height}};
      // std::cout << boundingBox << std::endl;
      // std::cout << "triangleClipVecZ:" << triangleClipVecZ << std::endl;
      /**
       * @brief 光栅化rasterization
       */
      // #pragma omp parallel for
      for (int y = (int)boundingBox.min.y; y < (int)boundingBox.max.y; y++) {
        for (int x = (int)boundingBox.min.x; x < (int)boundingBox.max.x; x++) {
          int bufferIndex = x + y * width;
          vec2 positionViewport{(float)x + 0.5f, (float)y + 0.5f};
          vec3 bcScreen = triangleProjDiv.getBarycentric(positionViewport);

          // 不在三角形内 (TODO 理解)
          if (bcScreen.x < 0 || bcScreen.y < 0 || bcScreen.z < 0)
            continue;
          // if (!triangleViewport.contains(positionViewport))
          //   continue;

          vec3 bcClip = bcScreen / triangleClipVecW;
          // TODO 这里还是不懂
          bcClip = bcClip / (bcClip.x + bcClip.y + bcClip.z);

          // 插值得到深度 TODO 理解为什么需要1-z
          float positionDepth =
              1 - triangleClipVecZDivZ.lerpBarycentric(bcClip);
          float zBufferDepth = zBuffer[bufferIndex];

          // 近远平面裁剪 TODO 确认
          if (positionDepth < 0 || positionDepth > 1)
            continue;

          // 或者深度大于已绘制的
          if (GLOBAL_STATE.DEPTH_TEST && zBufferDepth > positionDepth)
            continue;

          // 插值varying(内存区块按照float插值)
          for (int iF32 = 0, ilF32 = varyingSizeSumU8 / sizeof(float);
               iF32 < ilF32; iF32++) {
            vec3 v{*(varyingA + iF32), *(varyingB + iF32), *(varyingC + iF32)};
            *((float *)(varyingLerpedMemU8) + iF32) = v.lerpBarycentric(bcClip);
          }
          // 设置到varying
          int offsetU8 = 0;
          for (auto &prop : fragmentTypeInfo.get_properties())
            if (prop.get_metadata(0).get_value<ShaderSourceMeta>() ==
                ShaderSourceMeta::Varying) {
              auto var = prop.get_value(*fragmentShader);
              auto varPtr = var.get_value<uint8_t *>();
              auto sizeU8 = prop.get_metadata(1).get_value<int>();

              memcpy(varPtr, varyingLerpedMemU8 + offsetU8, sizeU8);
              offsetU8 += sizeU8;
            }

          // 执行fragment shader
          fragmentShader->_discarded = false;
          fragmentTypeInfo.get_method("main").invoke(*fragmentShader);
          if (fragmentShader->_discarded)
            continue;

          auto &color = fragmentShader->gl_FragColor;

          // 更新zBuffer frameBuffer
          zBuffer[bufferIndex] = positionDepth;

          if (frameBufferTextureBuffer->internalFormat == GL_RGBA) {
            if (frameBufferTextureBuffer->dataType == GL_FLOAT) {
              vec4 *frameBuffer = (vec4 *)frameBufferTextureBuffer->data;
              frameBuffer[bufferIndex] = color;
            } else if (frameBufferTextureBuffer->dataType == GL_UNSIGNED_BYTE) {
              uint8_t *frameBuffer = (uint8_t *)frameBufferTextureBuffer->data;
              frameBuffer[bufferIndex * 4] = (uint8_t)(color.r * 255);
              frameBuffer[bufferIndex * 4 + 1] = (uint8_t)(color.g * 255);
              frameBuffer[bufferIndex * 4 + 2] = (uint8_t)(color.b * 255);
              frameBuffer[bufferIndex * 4 + 3] = (uint8_t)(color.a * 255);
            }
          }
        }
      }
    }
  }

  delete varyingMemU8;
  delete varyingLerpedMemU8;
}
} // namespace helper

inline vec4 ShaderSource::texture2D(sample2D textureUint, vec2 uv) {
  auto &textureUintInfo = GLOBAL_STATE.textureUints[textureUint];

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

inline Shader *glCreateShader(Shader::Type type) { return new Shader(type); }
inline void glShaderSource(Shader *shader, ShaderSource *source) {
  shader->source = source;
}
inline void glCompileShader(Shader *shader) { shader->COMPILE_STATUS = true; };
inline Program *glCreateProgram() { return new Program(); }
inline void glAttachShader(Program *program, Shader *shader) {
  if (shader->type == Shader::VERTEX_SHADER)
    program->vertexShader = shader;
  else if (shader->type == Shader::FRAGMENT_SHADER)
    program->fragmentShader = shader;
}
inline void glLinkProgram(Program *program) {
  // 收集shader上的attribute uniform varying 信息到program里
  int attributeIndex = 0;
  int uniformIndex = 0;
  int varyingIndex = 0;
  auto processTypeInfo = [&](rttr::type &&type) {
    for (auto &prop : type.get_properties()) {
      auto name = prop.get_name();
      auto type = prop.get_type().get_name();
      auto attr = prop.get_metadata(0).get_value<ShaderSourceMeta>();
      switch (attr) {
      case ShaderSourceMeta::Attribute:
        program->attributes[name] = {attributeIndex++, type, name};
        break;
      case ShaderSourceMeta::Uniform:
        program->unifroms[name] = {uniformIndex++, type, name};
        break;
      case ShaderSourceMeta::Varying:
        // program->varyings[name] = {varyingIndex++, type, name};
        break;
      }
    }
  };

  processTypeInfo(program->vertexShader->source->get_derived_info().m_type);
  processTypeInfo(program->fragmentShader->source->get_derived_info().m_type);
}
inline void glDetachShader(Program *program, Shader *shader) {}
inline void glUseProgram(Program *program) {
  GLOBAL_STATE.CURRENT_PROGRAM = program;
}
inline Buffer *glCreateBuffer() { return new Buffer(); }
inline void glBindBuffer(int location, Buffer *buffer) {
  if (location == GL_ARRAY_BUFFER)
    GLOBAL_STATE.ARRAY_BUFFER_BINDING = buffer;
  if (location == GL_ELEMENT_ARRAY_BUFFER) {
    auto vao = GLOBAL_STATE.VERTEX_ARRAY_BINDING;
    if (vao == nullptr)
      vao = &DEFAULT_VERTEX_ARRAY;
    vao->indexBuffer = buffer;
  }
}
inline void glBufferData(int location, int length, const void *data,
                         int usage) {
  Buffer *target = nullptr;

  if (location == GL_ARRAY_BUFFER)
    target = GLOBAL_STATE.ARRAY_BUFFER_BINDING;
  if (location == GL_ELEMENT_ARRAY_BUFFER) {
    auto vao = GLOBAL_STATE.VERTEX_ARRAY_BINDING;
    if (vao == nullptr)
      vao = &DEFAULT_VERTEX_ARRAY;
    target = vao->indexBuffer;
  }

  if (target != nullptr) {
    target->data = data;
    target->length = length;
  }
}
inline void glEnableVertexAttribArray(int location) {
  // 设置默认的vao
  if (DEFAULT_VERTEX_ARRAY.attributes.size() < location + 1)
    DEFAULT_VERTEX_ARRAY.attributes.resize(location + 1);

  DEFAULT_VERTEX_ARRAY.attributes[location] = {true};
}
inline void glVertexAttribPointer(int location, int size, int type,
                                  bool normalized, int stride, int offset) {
  // 写入vertex array表格
  if (DEFAULT_VERTEX_ARRAY.attributes.size() < location + 1)
    DEFAULT_VERTEX_ARRAY.attributes.resize(location + 1);

  auto &attributeInfo = DEFAULT_VERTEX_ARRAY.attributes[location];
  attributeInfo.buffer = GLOBAL_STATE.ARRAY_BUFFER_BINDING;
  attributeInfo.type = type;
  attributeInfo.stride = stride;
  attributeInfo.offset = offset;
  attributeInfo.size = size;
  attributeInfo.normalized = normalized;
}
inline int glGetAttribLocation(Program *program, str name) {
  auto dataInfo = program->attributes[name];
  return dataInfo.location;
}
inline int glGetUniformLocation(Program *program, str name) {
  auto dataInfo = program->unifroms[name];
  return dataInfo.location;
}
inline Texture *glCreateTexture() { return new Texture(); }
inline void glBindTexture(int location, Texture *tex) {
  if (location == GL_TEXTURE_2D)
    GLOBAL_STATE.textureUints[GLOBAL_STATE.ACTIVE_TEXTURE - GL_TEXTURE0].map =
        tex;
}
inline void glTexImage2D(int location, int mipLevel, int internalFormat,
                         int width, int height, int border, int format,
                         int dataType, const void *data) {
  Texture *target = nullptr;
  if (location == GL_TEXTURE_2D) {
    target =
        GLOBAL_STATE.textureUints[GLOBAL_STATE.ACTIVE_TEXTURE - GL_TEXTURE0]
            .map;
  }
  if (target->mips.size() <= mipLevel)
    target->mips.resize(mipLevel + 1);

  if (data == nullptr) {
    int storeSize = width * height;
    if (internalFormat == GL_RGBA)
      storeSize *= 4;

    if (dataType == GL_UNSIGNED_BYTE)
      data = malloc(sizeof(uint8_t) * storeSize);
    if (dataType == GL_UNSIGNED_SHORT)
      data = malloc(sizeof(uint16_t) * storeSize);
  }

  target->mips[mipLevel] = new TextureBuffer{
      data, 0, width, height, format, border, dataType, internalFormat};
}
inline void glTexParameteri(int location, int key, int value) {
  Texture *target = helper::getTextureFrom(location);

  if (target != nullptr) {
    if (key == GL_TEXTURE_MIN_FILTER)
      target->TEXTURE_MIN_FILTER = value;
    if (key == GL_TEXTURE_MAG_FILTER)
      target->TEXTURE_MIN_FILTER = value;
    if (key == GL_TEXTURE_WRAP_S)
      target->TEXTURE_MIN_FILTER = value;
    if (key == GL_TEXTURE_WRAP_T)
      target->TEXTURE_MIN_FILTER = value;
  }
}
inline void glGenerateMipmap(int location) {
  Texture *target = helper::getTextureFrom(location);
  // TODO
}
inline void glViewport(float x, float y, float w, float h) {
  GLOBAL_STATE.VIEWPORT.x = x;
  GLOBAL_STATE.VIEWPORT.y = y;
  GLOBAL_STATE.VIEWPORT.z = w;
  GLOBAL_STATE.VIEWPORT.w = h;
}
inline void glClearColor(float r, float g, float b, float a) {
  GLOBAL_STATE.COLOR_CLEAR_VALUE = {r, g, b, a};
}
inline void glClear(int mask) {
  auto vao = GLOBAL_STATE.VERTEX_ARRAY_BINDING;
  auto fbo = GLOBAL_STATE.FRAMEBUFFER_BINDING;
  const auto &viewport = GLOBAL_STATE.VIEWPORT;
  const int width = (int)viewport.z;
  const int height = (int)viewport.w;

  if (fbo == nullptr)
    fbo = &DEFAULT_FRAMEBUFFER;

  if (mask | GL_COLOR_BUFFER_BIT &&
      fbo->COLOR_ATTACHMENT0.attachment != nullptr &&
      fbo->COLOR_ATTACHMENT0.attachment->mips.size() != 0) {
    auto frameBufferTextureBuffer =
        fbo->COLOR_ATTACHMENT0.attachment->mips[fbo->COLOR_ATTACHMENT0.level];
    auto color = GLOBAL_STATE.COLOR_CLEAR_VALUE;
    auto colorRedU8 = (uint8_t)(color.r * 255);
    auto colorGreenU8 = (uint8_t)(color.g * 255);
    auto colorBlueU8 = (uint8_t)(color.g * 255);
    auto colorAlphaU8 = (uint8_t)(color.g * 255);
    // clearColor
    for (int x = 0; x < width; x++) {
      for (int y = 0; y < height; y++) {
        int bufferIndex = x + y * width;
        if (frameBufferTextureBuffer->internalFormat == GL_RGBA) {
          if (frameBufferTextureBuffer->dataType == GL_FLOAT) {
            vec4 *frameBuffer = (vec4 *)frameBufferTextureBuffer->data;
            frameBuffer[bufferIndex] = color;
          } else if (frameBufferTextureBuffer->dataType == GL_UNSIGNED_BYTE) {
            uint8_t *frameBuffer = (uint8_t *)frameBufferTextureBuffer->data;
            frameBuffer[bufferIndex * 4] = colorRedU8;
            frameBuffer[bufferIndex * 4 + 1] = colorGreenU8;
            frameBuffer[bufferIndex * 4 + 2] = colorBlueU8;
            frameBuffer[bufferIndex * 4 + 3] = colorAlphaU8;
          }
        }
      }
    }
  }
  if (mask | GL_DEPTH_BUFFER_BIT &&
      fbo->DEPTH_ATTACHMENT.attachment != nullptr &&
      fbo->DEPTH_ATTACHMENT.attachment->mips.size() != 0) {
    auto zBuffer = static_cast<float *>(
        const_cast<void *>(fbo->DEPTH_ATTACHMENT.attachment->mips[0]->data));

    // 重置zBuffer
    std::fill_n(zBuffer, width * height, -std::numeric_limits<float>::max());
  }
}
inline void glEnable(int feature) {
  if (feature == GL_CULL_FACE)
    GLOBAL_STATE.CULL_FACE = GL_TRUE;
  if (feature == GL_DEPTH_TEST)
    GLOBAL_STATE.DEPTH_TEST = GL_TRUE;
}
inline void glActiveTexture(int textureUint) {
  GLOBAL_STATE.ACTIVE_TEXTURE = textureUint;
}
inline void glUniform1i(int location, int value) {
  helper::setUniform(
      location, [&](rttr::property &vertexProp, rttr::property &fragmentProp,
                    ShaderSource *vertexShader, ShaderSource *fragmentShader,
                    rttr::type &, rttr::type &) {
        if (vertexProp.is_valid()) {
          auto ptr = vertexProp.get_value(*vertexShader).get_value<int *>();
          *ptr = value;
        }
        if (fragmentProp.is_valid()) {
          auto ptr = fragmentProp.get_value(*fragmentShader).get_value<int *>();
          *ptr = value;
        }
      });
}
inline void glUniform3fv(int location, int count, const void *data) {
  helper::setUniform(
      location, [&](rttr::property &vertexProp, rttr::property &fragmentProp,
                    ShaderSource *vertexShader, ShaderSource *fragmentShader,
                    rttr::type &, rttr::type &) {
        if (vertexProp.is_valid()) {
          auto var = vertexProp.get_value(*vertexShader);
          auto varPtr = var.get_value<vec3 *>();
          auto size = vertexProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }

        if (fragmentProp.is_valid()) {
          auto var = fragmentProp.get_value(*fragmentShader);
          auto varPtr = var.get_value<vec3 *>();
          auto size = fragmentProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }
      });
}
inline void glUniformMatrix4fv(int location, int count, bool transpose,
                               const void *data) {
  helper::setUniform(
      location, [&](rttr::property &vertexProp, rttr::property &fragmentProp,
                    ShaderSource *vertexShader, ShaderSource *fragmentShader,
                    rttr::type &, rttr::type &) {
        if (vertexProp.is_valid()) {
          auto var = vertexProp.get_value(*vertexShader);
          auto varPtr = var.get_value<mat4 *>();
          auto size = vertexProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }

        if (fragmentProp.is_valid()) {
          auto var = fragmentProp.get_value(*fragmentShader);
          auto varPtr = var.get_value<mat4 *>();
          auto size = fragmentProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }
      });
}
inline void glUniform4fv(int location, int count, const void *data) {
  helper::setUniform(
      location, [&](rttr::property &vertexProp, rttr::property &fragmentProp,
                    ShaderSource *vertexShader, ShaderSource *fragmentShader,
                    rttr::type &, rttr::type &) {
        if (vertexProp.is_valid()) {
          auto var = vertexProp.get_value(*vertexShader);
          auto varPtr = var.get_value<vec4 *>();
          auto size = vertexProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }

        if (fragmentProp.is_valid()) {
          auto var = fragmentProp.get_value(*fragmentShader);
          auto varPtr = var.get_value<vec4 *>();
          auto size = fragmentProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }
      });
}
inline void glDrawElements(int mode, int count, int dataType,
                           const void *indices) {
  helper::draw(mode, 0, count, dataType, indices);
}
inline void glDrawArrays(int mode, int first, int count) {
  helper::draw(mode, first, count, 0, 0);
}
inline FrameBuffer *glCreateFramebuffer() { return new FrameBuffer(); }
inline RenderBuffer *glCreateRenderbuffer() { return new RenderBuffer(); }
inline void glBindFramebuffer(int location, FrameBuffer *buffer) {
  GLOBAL_STATE.FRAMEBUFFER_BINDING = buffer;
}
inline void glBindRenderbuffer(int location, RenderBuffer *buffer) {
  GLOBAL_STATE.RENDERBUFFER_BINDING = buffer;
}
inline void glFramebufferTexture2D(int target, int attachment, int textarget,
                                   Texture *texture, int level) {
  if (target == GL_FRAMEBUFFER && GLOBAL_STATE.RENDERBUFFER_BINDING) {
    if (attachment == GL_COLOR_ATTACHMENT0) {
      if (textarget == GL_TEXTURE_2D) {
        GLOBAL_STATE.FRAMEBUFFER_BINDING->COLOR_ATTACHMENT0.attachment =
            texture;
        GLOBAL_STATE.FRAMEBUFFER_BINDING->COLOR_ATTACHMENT0.level = level;
      }
    }
  }
}
inline void glFramebufferRenderbuffer(int target, int attachment,
                                      int renderbufferTarget,
                                      RenderBuffer *renderbuffer) {
  if (target == GL_FRAMEBUFFER && GLOBAL_STATE.RENDERBUFFER_BINDING) {
    if (attachment == GL_DEPTH_ATTACHMENT) {
      if (renderbufferTarget == GL_RENDERBUFFER) {
        GLOBAL_STATE.FRAMEBUFFER_BINDING->DEPTH_ATTACHMENT.attachment =
            renderbuffer->attachment;
      }
    }
  }
}
inline void glRenderbufferStorage(int target, int internalFormat, int width,
                                  int height) {
  if (target == GL_RENDERBUFFER) {
    GLOBAL_STATE.RENDERBUFFER_BINDING->format = internalFormat;
    GLOBAL_STATE.RENDERBUFFER_BINDING->width = width;
    GLOBAL_STATE.RENDERBUFFER_BINDING->height = height;

    if (internalFormat == GL_DEPTH_COMPONENT32F) {
      int length = sizeof(float) * width * height;
      auto texture = new Texture();
      texture->mips.push_back(new TextureBuffer{
          malloc(length), length, width, height, GL_DEPTH_COMPONENT32F, 0,
          GL_FLOAT, GL_DEPTH_COMPONENT32F});
      GLOBAL_STATE.RENDERBUFFER_BINDING->attachment = texture;
    }
  }
}
} // namespace CppGL