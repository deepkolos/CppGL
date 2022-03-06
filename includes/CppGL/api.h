#pragma once

#include "buffer.h"
#include "constant.h"
#include "debug.h"
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
#include <CppGL/rttr.h>
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

namespace Helper {
Texture *getTextureFrom(int location);
void setUniform(
    int location,
    std::function<void(rttr::property &, rttr::property &, ShaderSource *,
                       ShaderSource *, rttr::type &, rttr::type &)>
        fn);
void draw(int mode, int first, int count, int dataType, const void *indices);
inline float if0Be1(float a) { return a == 0 ? 1 : a; }
} // namespace Helper

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
inline void glDetachShader(Program *program, Shader *shader) {}
inline void glUseProgram(Program *program) {
  GLOBAL::GLOBAL_STATE->CURRENT_PROGRAM = program;
}
inline Buffer *glCreateBuffer() { return new Buffer(); }
inline void glBindBuffer(int location, Buffer *buffer) {
  if (location == GL_ARRAY_BUFFER)
    GLOBAL::GLOBAL_STATE->ARRAY_BUFFER_BINDING = buffer;
  if (location == GL_ELEMENT_ARRAY_BUFFER) {
    auto vao = GLOBAL::GLOBAL_STATE->VERTEX_ARRAY_BINDING;
    if (vao == nullptr)
      vao = GLOBAL::DEFAULT_VERTEX_ARRAY;
    vao->indexBuffer = buffer;
  }
}
inline void glBufferData(int location, int length, const void *data,
                         int usage) {
  Buffer *target = nullptr;

  if (location == GL_ARRAY_BUFFER)
    target = GLOBAL::GLOBAL_STATE->ARRAY_BUFFER_BINDING;
  if (location == GL_ELEMENT_ARRAY_BUFFER) {
    auto vao = GLOBAL::GLOBAL_STATE->VERTEX_ARRAY_BINDING;
    if (vao == nullptr)
      vao = GLOBAL::DEFAULT_VERTEX_ARRAY;
    target = vao->indexBuffer;
  }

  if (target != nullptr) {
    target->data = data;
    target->length = length;
  }
}
inline void glEnableVertexAttribArray(int location) {
  auto vao = GLOBAL::DEFAULT_VERTEX_ARRAY;
  // 设置默认的vao
  if (vao->attributes.size() < location + 1)
    vao->attributes.resize(location + 1);

  vao->attributes[location] = {true};
}
inline void glVertexAttribPointer(int location, int size, int type,
                                  bool normalized, int stride, int offset) {
  auto vao = GLOBAL::DEFAULT_VERTEX_ARRAY;
  if (vao->attributes.size() < location + 1)
    vao->attributes.resize(location + 1);

  auto &attributeInfo = vao->attributes[location];
  attributeInfo.buffer = GLOBAL::GLOBAL_STATE->ARRAY_BUFFER_BINDING;
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
  auto state = GLOBAL::GLOBAL_STATE;
  if (location == GL_TEXTURE_2D)
    state->textureUints[state->ACTIVE_TEXTURE - GL_TEXTURE0].map = tex;
}
inline void glTexParameteri(int location, int key, int value) {
  Texture *target = Helper::getTextureFrom(location);

  if (target != nullptr) {
    if (key == GL_TEXTURE_MIN_FILTER)
      target->TEXTURE_MIN_FILTER = value;
    if (key == GL_TEXTURE_MAG_FILTER)
      target->TEXTURE_MAG_FILTER = value;
    if (key == GL_TEXTURE_WRAP_S)
      target->TEXTURE_WRAP_S = value;
    if (key == GL_TEXTURE_WRAP_T)
      target->TEXTURE_WRAP_T = value;
  }
}
inline void glGenerateMipmap(int location) {
  Texture *target = Helper::getTextureFrom(location);
  // TODO
}
inline void glViewport(float x, float y, float w, float h) {
  GLOBAL::GLOBAL_STATE->VIEWPORT.x = x;
  GLOBAL::GLOBAL_STATE->VIEWPORT.y = y;
  GLOBAL::GLOBAL_STATE->VIEWPORT.z = w;
  GLOBAL::GLOBAL_STATE->VIEWPORT.w = h;
}
inline void glClearColor(float r, float g, float b, float a) {
  GLOBAL::GLOBAL_STATE->COLOR_CLEAR_VALUE = {r, g, b, a};
}
inline void glEnable(int feature) {
  if (feature == GL_CULL_FACE)
    GLOBAL::GLOBAL_STATE->CULL_FACE = GL_TRUE;
  if (feature == GL_DEPTH_TEST)
    GLOBAL::GLOBAL_STATE->DEPTH_TEST = GL_TRUE;
}
inline void glActiveTexture(int textureUint) {
  GLOBAL::GLOBAL_STATE->ACTIVE_TEXTURE = textureUint;
}
inline void glDrawElements(int mode, int count, int dataType,
                           const void *indices) {
  Helper::draw(mode, 0, count, dataType, indices);
}
inline void glDrawArrays(int mode, int first, int count) {
  Helper::draw(mode, first, count, 0, 0);
}
inline FrameBuffer *glCreateFramebuffer() { return new FrameBuffer(); }
inline RenderBuffer *glCreateRenderbuffer() { return new RenderBuffer(); }
inline void glBindFramebuffer(int location, FrameBuffer *buffer) {
  GLOBAL::GLOBAL_STATE->FRAMEBUFFER_BINDING = buffer;
}
inline void glBindRenderbuffer(int location, RenderBuffer *buffer) {
  GLOBAL::GLOBAL_STATE->RENDERBUFFER_BINDING = buffer;
}

void glLinkProgram(Program *program);
void glTexImage2D(int location, int mipLevel, int internalFormat, int width,
                  int height, int border, int format, int dataType,
                  const void *data);
void glClear(int mask);
void glUniform1i(int location, int value);
void glUniform1f(int location, float value);
void glUniform2fv(int location, int count, const void *data);
void glUniform3fv(int location, int count, const void *data);
void glUniform4fv(int location, int count, const void *data);
void glUniformMatrix4fv(int location, int count, bool transpose,
                        const void *data);
void glUniformMatrix3fv(int location, int count, bool transpose,
                        const void *data);
void glFramebufferTexture2D(int target, int attachment, int textarget,
                            Texture *texture, int level);
void glFramebufferRenderbuffer(int target, int attachment,
                               int renderbufferTarget,
                               RenderBuffer *renderbuffer);
void glRenderbufferStorage(int target, int internalFormat, int width,
                           int height);
} // namespace CppGL