#pragma once

#include "math.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace CppGL {
struct Program;
// typedef std::function<void(const Program &)> ShaderSource;

struct ShaderMeta {
  enum ShaderMetaType {
    attribute,
    uniform,
    varying,
  } type;

  union {
    vec4 vec4;
    vec3 vec3;
    vec2 vec2;
    mat3 mat3;
    mat4 mat4;
  };
};

struct ShaderSource {
  vec4 gl_Position;
  vec4 gl_FragColor;
};

struct Shader {
  enum Type {
    VERTEX_SHADER,
    FRAGMENT_SHADER,
  };

  Shader(Type type) : type(type) {}

  Type type;
  ShaderSource source;
  bool COMPILE_STATUS = false;
};
} // namespace CppGL