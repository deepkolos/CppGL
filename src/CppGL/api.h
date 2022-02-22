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
#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <malloc/_malloc.h>
#include <map>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <rttr/type>
#include <utility>
#include <vector>

namespace CppGL {
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
} // namespace helper

using str = rttr::string_view;

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
}
inline void glBufferData(int location, int length, const void *data,
                         int usage) {
  if (location == GL_ARRAY_BUFFER && GLOBAL_STATE.ARRAY_BUFFER_BINDING) {
    GLOBAL_STATE.ARRAY_BUFFER_BINDING->data = data;
    GLOBAL_STATE.ARRAY_BUFFER_BINDING->length = length;
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

inline void glDrawArrays(int mode, int first, int count) {
  auto program = GLOBAL_STATE.CURRENT_PROGRAM;
  auto vao = GLOBAL_STATE.VERTEX_ARRAY_BINDING;
  auto vertexShader = program->vertexShader->source;
  auto fragmentShader = program->fragmentShader->source;
  auto vertexTypeInfo = vertexShader->get_derived_info().m_type;
  auto fragmentTypeInfo = fragmentShader->get_derived_info().m_type;
  const auto &viewport = GLOBAL_STATE.VIEWPORT;
  const auto width = viewport.z;
  const auto height = viewport.w;
  std::vector<vec4> clipSpaceVertices(count);

  if (vao == nullptr)
    vao = &DEFAULT_VERTEX_ARRAY;

  /**
   * @brief 分配varying内存 count * (varying size 总和)
   * |               内存布局                |
   * | count0             count1            |
   * | varyingA varyingB  varyingA varyingB |
   */
  int varyingNum = 0;
  int varyingSizeSum = 0;
  std::map<str, int> varyingOffsetMap;
  for (auto &prop : vertexTypeInfo.get_properties())
    if (prop.get_metadata(0).get_value<ShaderSourceMeta>() ==
        ShaderSourceMeta::Varying) {
      auto size = prop.get_metadata(1).get_value<size_t>();
      varyingOffsetMap[prop.get_name()] = varyingSizeSum;
      varyingSizeSum += size;
      varyingNum++;
    }
  float *const varyingMem = (float *)malloc(varyingSizeSum * count);
  float *const varyingLerpedMem = (float *)malloc(varyingSizeSum);

  /**
   * @brief 分配frameBuffer zBuffer
   */
  std::vector<vec4> frameBuffer(width * height, {0, 0, 0, 0});
  std::vector<float> zBuffer(width * height,
                             -std::numeric_limits<float>::max());

  /**
   * @brief 顶点循环
   * 读取attribute 设置到
   */
  for (int i = 0; i < count; i++) {
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
      auto varPtr = var.get_value<float *>();

      if (info.type == GL_UNSIGNED_BYTE && info.normalized) {
        for (int j = 0; j < info.size; j++) {
          *varPtr = static_cast<float>(*((uint8_t *)(ptr) + j)) / 255;
          varPtr++;
        }
      } else {
        memcpy(varPtr, ptr, stride);
      }
    }

    // TODO 设置uniform

    // 执行vertex shader
    vertexTypeInfo.get_method("main").invoke(vertexShader);

    // 收集gl_Position
    clipSpaceVertices[i] = vertexShader->gl_Position;

    // 收集varying
    size_t offset = 0;
    for (auto &prop : vertexTypeInfo.get_properties()) {
      if (prop.get_metadata(0).get_value<ShaderSourceMeta>() ==
          ShaderSourceMeta::Varying) {
        auto src = prop.get_value(*vertexShader).get_value<char *>();
        auto dst = varyingMem + (i * varyingSizeSum) + offset;
        auto size = prop.get_metadata(1).get_value<size_t>();
        memcpy(dst, src, size);
        offset += size;
      }
    }
  }

  auto viewportMatrix = getViewportMatrix(viewport);

  if (mode == GL_TRIANGLES) {
    for (int vertexIndex = 0; vertexIndex < count; vertexIndex += 3) {
      triangle triangleClip{clipSpaceVertices[vertexIndex],
                            clipSpaceVertices[vertexIndex + 1],
                            clipSpaceVertices[vertexIndex + 2]};
      float *varyingA = varyingMem + vertexIndex * varyingSizeSum;
      float *varyingB = varyingMem + (vertexIndex + 1) * varyingSizeSum;
      float *varyingC = varyingMem + (vertexIndex + 2) * varyingSizeSum;

      /**
       * @brief 透视除法
       */
      vec3 triangleClipVecW{triangleClip.a.w, triangleClip.b.w,
                            triangleClip.c.w};
      vec3 triangleClipVecZ{triangleClip.a.z, triangleClip.b.z,
                            triangleClip.c.z};
      triangle triangleViewport = triangleClip * viewportMatrix;
      triangle triangleProjDiv =
          triangleViewport.perspectiveDivide(triangleClipVecW);
      /**
       * @brief 寻找三角形bounding box
       */
      box2 boundingBox = triangleProjDiv.viewportBoundingBox(viewport);
      // box2 boundingBox = {{0, 0}, {width, height}};
      /**
       * @brief 光栅化rasterization
       */
#pragma omp parallel for
      for (int x = (int)boundingBox.min.x; x < (int)boundingBox.max.x; x++) {
        for (int y = (int)boundingBox.min.y; y < (int)boundingBox.max.y; y++) {
          int bufferIndex = x + y * width;
          vec2 positionViewport{(float)x + 0.5f, (float)y + 0.5f};
          vec3 bcScreen = triangleProjDiv.getBarycentric(positionViewport);
          vec3 bcClip = bcScreen / triangleClipVecW;

          // 这里还是不懂
          bcClip = bcClip / (bcClip.x + bcClip.y + bcClip.z);

          // 不在三角形内
          if (bcScreen.x < 0 || bcScreen.y < 0 || bcScreen.z < 0)
            continue;
          // if (!triangleViewport.contains(positionViewport))
          //   continue;
          // 插值得到深度
          float positionDepth = triangleClipVecZ.lerpBarycentric(bcClip);
          float zBufferDepth = zBuffer[bufferIndex];
          // 或者深度大于已绘制的
          if (zBufferDepth > positionDepth)
            continue;

          // 插值varying(内存区块按照float插值)
          for (int i = 0, il = varyingSizeSum / sizeof(float); i < il; i++) {
            vec3 v{*(varyingA + i), *(varyingB + i), *(varyingC + i)};
            *(varyingLerpedMem + i) = v.lerpBarycentric(bcClip);
          }
          // 设置到varying
          for (auto &prop : fragmentTypeInfo.get_properties())
            if (prop.get_metadata(0).get_value<ShaderSourceMeta>() ==
                ShaderSourceMeta::Varying) {
              auto var = prop.get_value(*fragmentShader);
              auto varPtr = var.get_value<float *>();
              auto search = varyingOffsetMap.find(prop.get_name());
              auto size = prop.get_metadata(1).get_value<size_t>();

              if (search != varyingOffsetMap.end()) {
                char *dst = reinterpret_cast<char *>(varyingLerpedMem);
                memcpy(varPtr, dst + search->second, size);
              }
            }
          // todo 设置uniform

          // 执行fragment shader
          fragmentShader->_discarded = false;
          fragmentTypeInfo.get_method("main").invoke(*fragmentShader);
          if (fragmentShader->_discarded)
            continue;

          // 更新zBuffer frameBuffer
          zBuffer[bufferIndex] = positionDepth;
          frameBuffer[bufferIndex] = fragmentShader->gl_FragColor;
        }
      }
    }
  }

  using namespace cv;
  Mat image = Mat::zeros(height, width, CV_8UC3);
  Mat imageZ = Mat::zeros(height, width, CV_8UC3);

#pragma omp parallel for
  for (int x = 0; x < (int)width; x++)
    for (int y = 0; y < (int)height; y++) {
      int bufferIndex = x + y * width;
      vec4 &pixel = frameBuffer[bufferIndex];
      float &depth = zBuffer[bufferIndex];
      int pixelIndex = (x + (height - y) * width) * 3;
      image.data[pixelIndex] = (uchar)(pixel.x * 255);
      image.data[pixelIndex + 1] = (uchar)(pixel.y * 255);
      image.data[pixelIndex + 2] = (uchar)(pixel.z * 255);

      imageZ.data[pixelIndex] = (uchar)(depth * 255);
      imageZ.data[pixelIndex + 1] = (uchar)(depth * 255);
      imageZ.data[pixelIndex + 2] = (uchar)(depth * 255);
    }

  imshow("zBuffer", imageZ);
  imshow("frameBuffer", image);
  waitKey(0);

  delete varyingMem;
  delete varyingLerpedMem;
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
  target->mips[mipLevel] = {data,   0,      width,    height,
                            format, border, dataType, internalFormat};
}
inline void glTexParameteri(int location, int key, int value) {
  Texture *target = helper::getTextureFrom(location);

  if (target) {
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
  // TODO 应该情况目前的framebuffer,但是目前的framebuffer还没有
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
        if (vertexProp.is_valid())
          vertexProp.set_value(*vertexShader, value);
        if (fragmentProp.is_valid())
          fragmentProp.set_value(*fragmentShader, value);
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
inline void glDrawElements(int mode, int count, int type, const void *indices) {

}
} // namespace CppGL