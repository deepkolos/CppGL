#pragma once

#include "attribute.h"
#include "buffer.h"
#include "constant.h"
#include "global-state.h"
#include "math.h"
#include "program.h"
#include "rttr/string_view.h"
#include "rttr/type.h"
#include "rttr/variant.h"
#include "shader.h"
#include "vertex-array.h"
#include <_types/_uint8_t.h>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <limits>
#include <malloc/_malloc.h>
#include <map>
#include <rttr/type>
#include <utility>
#include <vector>

namespace CppGL {
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
template <class T, class N> inline void glLinkProgram(Program *program) {
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

  processTypeInfo(
      rttr::type::get(*reinterpret_cast<T *>(program->vertexShader->source)));
  processTypeInfo(
      rttr::type::get(*reinterpret_cast<N *>(program->fragmentShader->source)));
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

template <class T, class N>
inline void glDrawArrays(int mode, int first, int count) {
  auto program = GLOBAL_STATE.CURRENT_PROGRAM;
  auto vao = GLOBAL_STATE.VERTEX_ARRAY_BINDING;
  auto vertexShader = reinterpret_cast<T *>(program->vertexShader->source);
  auto fragmentShader = reinterpret_cast<N *>(program->fragmentShader->source);
  auto vertexTypeInfo = rttr::type::get(*vertexShader);
  auto fragmentTypeInfo = rttr::type::get(*fragmentShader);
  auto &viewport = GLOBAL_STATE.VIEWPORT;
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
    if (prop.get_metadata(0).template get_value<ShaderSourceMeta>() ==
        ShaderSourceMeta::Varying) {
      auto size = prop.get_metadata(1).template get_value<size_t>();
      varyingOffsetMap[prop.get_name()] = varyingSizeSum;
      varyingSizeSum += size;
      varyingNum++;
    }
  float *const varyingMem = (float *)malloc(varyingSizeSum * count);
  float *const varyingLerpedMem = (float *)malloc(varyingSizeSum);

  /**
   * @brief 分配frameBuffer zBuffer
   */
  std::vector<vec4> frameBuffer(viewport.z * viewport.w, {0, 0, 0, 0});
  std::vector<float> zBuffer(viewport.z * viewport.w,
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
      auto varPtr = var.template get_value<float *>();

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
      if (prop.get_metadata(0).template get_value<ShaderSourceMeta>() ==
          ShaderSourceMeta::Varying) {
        auto src = prop.get_value(*vertexShader).template get_value<char *>();
        auto dst = varyingMem + (i * varyingSizeSum) + offset;
        auto size = prop.get_metadata(1).template get_value<size_t>();
        memcpy(dst, src, size);
        offset += size;
      }
    }
  }

  auto viewportMatrix = getViewportMatrix(GLOBAL_STATE.VIEWPORT);

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
      box2 boundingBox =
          triangleViewport.viewportBoundingBox(GLOBAL_STATE.VIEWPORT);
      /**
       * @brief 光栅化rasterization
       */
      for (int x = (int)boundingBox.min.x; x < (int)boundingBox.max.y; x++) {
        for (int y = (int)boundingBox.min.y; y < (int)boundingBox.max.y; y++) {
          int bufferIndex = x + y * viewport.z;
          vec2 positionViewport{(float)x, (float)y};
          vec3 bcScreen = triangleProjDiv.getBarycentric(positionViewport);
          vec3 bcClip = bcScreen / triangleClipVecW;

          // 这里还是不懂
          bcClip = bcClip / (bcClip.x + bcClip.y + bcClip.z);

          // 插值得到深度
          float positionDepth = triangleClipVecZ.lerpBarycentric(bcClip);
          float zBufferDepth = zBuffer[bufferIndex];

          // 不在三角形内或者深度大于已绘制的
          if (bcScreen.x < 0 || bcScreen.y < 0 || bcScreen.z < 0 ||
              zBufferDepth > positionDepth)
            continue;

          // 插值varying(内存区块按照float插值)
          for (int i = 0, il = varyingSizeSum / sizeof(float); i < il; i++) {
            vec3 v{*(varyingA + i), *(varyingB + i), *(varyingC + i)};
            *(varyingLerpedMem + i) = v.lerpBarycentric(bcClip);
          }
          // 设置到varying
          for (auto &prop : fragmentTypeInfo.get_properties())
            if (prop.get_metadata(0).template get_value<ShaderSourceMeta>() ==
                ShaderSourceMeta::Varying) {
              auto var = prop.get_value(*fragmentShader);
              auto varPtr = var.template get_value<float *>();
              auto search = varyingOffsetMap.find(prop.get_name());
              auto size = prop.get_metadata(1).template get_value<size_t>();

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

  delete varyingMem;
  delete varyingLerpedMem;
}
} // namespace CppGL