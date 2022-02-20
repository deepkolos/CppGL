#pragma once

#include "math.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace CppGL {

struct Program;

struct ShaderMeta {
  enum ShaderMetaType {
    Attribute,
    Uniform,
    Varying,
  } type;

  union {
    vec4 vec4;
    vec3 vec3;
    vec2 vec2;
    mat3 mat3;
    mat4 mat4;
  };
};

typedef int sample2D;

struct ShaderSource {
  vec4 gl_Position;
  vec4 gl_FragColor;
  bool _discarded = false;
  void DISCARD() { _discarded = true; }
  vec4 texture2D(sample2D textureUint, vec2 uv) { return {}; }
};

struct Shader {
  enum Type {
    VERTEX_SHADER,
    FRAGMENT_SHADER,
  };

  Shader(Type type) : type(type) {}

  Type type;
  ShaderSource *source;
  bool COMPILE_STATUS = false;
};
} // namespace CppGL