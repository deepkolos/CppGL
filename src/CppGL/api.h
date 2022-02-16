#pragma once

#include "attribute.h"
#include "buffer.h"
#include "constant.h"
#include "global-state.h"
#include "program.h"
#include "rttr/string_view.h"
#include "rttr/type.h"
#include "vertex-array.h"
#include <rttr/type>
#include <utility>

namespace CppGL {
using str = rttr::string_view;
inline Shader *glCreateShader(Shader::Type type) { return new Shader(type); }
inline void glShaderSource(Shader *shader, ShaderSource source) {
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
      auto attr = prop.get_metadata("attr").get_value<ShaderSourceMeta>();
      switch (attr) {
      case ShaderSourceMeta::attribute:
        program->attributes[name] = {attributeIndex++, type, name};
        break;
      case ShaderSourceMeta::uniform:
        program->unifroms[name] = {uniformIndex++, type, name};
        break;
      case ShaderSourceMeta::varying:
        // program->varyings[name] = {varyingIndex++, type, name};
        break;
      }
    }
  };

  processTypeInfo(rttr::type::get(*program->vertexShader));
  processTypeInfo(rttr::type::get(*program->fragmentShader));
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
  if (DEFAULT_VERTEX_ARRAY.attributes.size() < location + 1) {
    DEFAULT_VERTEX_ARRAY.attributes.resize(location + 1);
  }
  DEFAULT_VERTEX_ARRAY.attributes[location] = {true};
}
inline void glVertexAttribPointer(int location, int size, int type,
                                  bool normalized, int stride, int offset) {
  // 写入vertex array表格
  if (DEFAULT_VERTEX_ARRAY.attributes.size() < location + 1) {
    DEFAULT_VERTEX_ARRAY.attributes.resize(location + 1);
  }
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
inline void glDrawArrays(int type, int component, int size) {
  // TODO run program
}
} // namespace CppGL