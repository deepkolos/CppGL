#pragma once
namespace CppGL {

struct vec2 {
  union {
    struct {
      float x;
      float y;
    };
    struct {
      float r;
      float g;
    };
  };
};

struct vec3 {
  union {
    struct {
      float x;
      float y;
      float z;
    };
    struct {
      float r;
      float g;
      float b;
    };
  };
};

struct vec4 {
  union {
    struct {
      float x;
      float y;
      float z;
      float w;
    };
    struct {
      float r;
      float g;
      float b;
      float a;
    };
  };
  vec4() : x(0), y(0), z(0), w(0) {}
  vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

struct mat3 {};
struct mat4 {};
} // namespace CppGL