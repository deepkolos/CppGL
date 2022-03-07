#pragma once

#include "math.h"
#include <cmath>
#include <functional>
#include <map>
#include <rttr/registration>
#include <string>
#include <vector>

namespace CppGL {

using sampler2D = int;
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
  inline void DISCARD() { _discarded = true; }
  static vec4 texture2D(sample2D textureUint, vec2 uv);

  inline static vec2 normalize(vec2 v) {
    return v / std::sqrtf(v.x * v.x + v.y * v.y);
  };
  inline static vec3 normalize(vec3 v) {
    return v / std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  };
  inline static vec4 normalize(vec4 v) {
    return v / std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
  };
  inline static float dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  };
  inline static float cross(vec2 a, vec2 b) { return a.x * b.y - a.y * b.x; }
  inline static float dot(vec2 a, vec2 b) { return a.x * b.x + a.y * b.y; }
  inline static float pow(float f, float a) { return std::powf(f, a); }
  inline static float max(float a, float b) { return std::max(a, b); }

  RTTR_ENABLE()
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