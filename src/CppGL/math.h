#pragma once
#include "../rttr.h"
#include <algorithm>
#include <iostream>
#include <utility>
namespace CppGL {

struct vec2;
inline float cross(vec2 a, vec2 b);
inline float dot(vec2 a, vec2 b);

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
  inline vec2(float x = 0, float y = 0) : x(x), y(y){};

  inline vec2 operator*(vec2 v) { return {x * v.x, y * v.y}; }
  inline vec2 operator-(vec2 v) { return {x - v.x, y - v.y}; }
  inline vec2 operator/(float f) { return {x / f, y / f}; }
  inline vec2 operator*(float f) { return {x * f, y * f}; }
  inline vec2 operator+(float f) { return {x + f, y + f}; }
  inline vec2 operator-(float f) { return {x - f, y - f}; }
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
  inline vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z){};

  inline vec3 operator/(vec3 v) { return {x / v.x, y / v.y, z / v.z}; }
  inline vec3 operator/(float f) { return {x / f, y / f, z / f}; }

  inline float lerpBarycentric(vec3 v) { return x * v.x + y * v.y + z * v.z; }
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
  vec4(float x = 0, float y = 0, float z = 0, float w = 0)
      : x(x), y(y), z(z), w(w) {}

  inline vec4 operator*(vec4 v) { return {x * v.x, y * v.y, z * v.z, w * v.w}; }
  inline vec4 operator/(float f) { return {x / f, y / f, z / f, w / f}; }
  inline vec4 operator*(float f) { return {x * f, y * f, z * f, w * f}; }
  inline vec4 operator+(float f) { return {x + f, y + f, z + f, w + f}; }
  inline vec4 operator-(float f) { return {x - f, y - f, z - f, w - f}; }
};

struct mat3 {
  union {
    vec3 col[3];
    float e[9];
    struct {
      float m0;
      float m1;
      float m2;
      float m3;
      float m4;
      float m5;
      float m6;
      float m7;
      float m8;
    };
  };
  inline mat3() {}
};
struct mat4 {
  union {
    vec4 col[4];
    float e[16];
    struct {
      vec4 col0;
      vec4 col1;
      vec4 col2;
      vec4 col3;
    };
    struct {
      float m0;
      float m1;
      float m2;
      float m3;
      float m4;
      float m5;
      float m6;
      float m7;
      float m8;
      float m9;
      float m10;
      float m11;
      float m12;
      float m13;
      float m14;
      float m15;
    };
  };

  inline mat4() {}
  inline mat4(vec4 &a, vec4 &b, vec4 &c, vec4 &d)
      : col0(a), col1(b), col2(c), col3(d) {}
  inline mat4(vec4 &&a, vec4 &&b, vec4 &&c, vec4 &&d)
      : col0(a), col1(b), col2(c), col3(d) {}

  inline vec4 operator*(vec4 v) {
    auto x = e[0] * v.x + e[4] * v.y + e[8] * v.z + e[12] * v.w;
    auto y = e[1] * v.x + e[5] * v.y + e[9] * v.z + e[13] * v.w;
    auto z = e[2] * v.x + e[6] * v.y + e[10] * v.z + e[14] * v.w;
    auto w = e[3] * v.x + e[7] * v.y + e[11] * v.z + e[15] * v.w;
    return {x, y, z, w};
  }
};

struct box2 {
  vec2 min{std::numeric_limits<float>::max(),
           std::numeric_limits<float>::max()};
  vec2 max{-std::numeric_limits<float>::max(),
           -std::numeric_limits<float>::max()};

  inline void expandByPoint(vec2 p) {
    min.x = std::min(min.x, p.x);
    min.y = std::min(min.y, p.y);
    max.x = std::max(max.x, p.x);
    max.y = std::max(max.y, p.y);
  }

  inline box2 &clamp(box2 box) {
    max.x = std::min(box.max.x, max.x);
    max.y = std::min(box.max.y, max.y);
    min.x = std::max(box.min.x, min.x);
    min.y = std::max(box.min.y, min.y);
    return *this;
  }
};

struct triangle {
  vec4 a;
  vec4 b;
  vec4 c;

  inline triangle perspectiveDivide(vec3 v) {
    return {a / v.x, b / v.y, c / v.z};
  }
  inline box2 viewportBoundingBox(vec4 viewport) {
    box2 boundingBox;
    boundingBox.expandByPoint({a.x, a.y});
    boundingBox.expandByPoint({b.x, b.y});
    boundingBox.expandByPoint({c.x, c.y});
    boundingBox.clamp({{0, 0}, {viewport.z, viewport.w}});
    return boundingBox;
  }

  inline vec3 getBarycentric(vec2 &p) {
    vec2 A = {a.x, a.y};
    vec2 B = {b.x, b.y};
    vec2 C = {c.x, c.y};
    auto v0 = C - A;
    auto v1 = B - A;
    auto v2 = p - A;
    auto dot00 = dot(v0, v0);
    auto dot01 = dot(v0, v1);
    auto dot02 = dot(v0, v2);
    auto dot11 = dot(v1, v1);
    auto dot12 = dot(v1, v2);
    auto denom = (dot00 * dot11 - dot01 * dot01);

    // collinear or singular triangle
    if (denom == 0)
      return {-2, -1, -1};

    auto invDenom = 1 / denom;
    auto u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    auto v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    // barycentric coordinates must always sum to 1
    return {1 - u - v, v, u};
  }

  inline bool contains(vec2 &p) {
    vec2 A = {a.x, a.y};
    vec2 B = {b.x, b.y};
    vec2 C = {c.x, c.y};
    auto signA = std::signbit(cross(p - A, B - A));
    auto signB = std::signbit(cross(p - B, C - B));
    auto signC = std::signbit(cross(p - C, A - C));
    return (signA && signB && signC) || (!signA && !signB && !signC);
  }

  inline vec4 lerpBarycentric(vec3 v) {
    return {
        a.x * v.x + b.x * v.y + c.x * v.z, a.y * v.x + b.y * v.y + c.y * v.z,
        a.z * v.x + b.z * v.y + c.z * v.z, a.w * v.x + b.w * v.y + c.w * v.z};
  }

  inline triangle operator*(mat4 m) { return {m * a, m * b, m * c}; }
  inline triangle operator/(float f) { return {a / f, b / f, c / f}; }
};

MY_RTTR_REGISTRATION {
  rttr::registration::class_<vec2>("vec2").constructor();
  rttr::registration::class_<vec3>("vec3").constructor();
  rttr::registration::class_<vec4>("vec4").constructor();
  rttr::registration::class_<mat3>("mat3").constructor();
  rttr::registration::class_<mat4>("mat4").constructor();
}

///// methods ////

inline mat4 getViewportMatrix(vec4 v) {
  auto x = v.x;
  auto y = v.y;
  auto w = v.z;
  auto h = v.w;
  return {vec4(w * 0.5, 0, 0, x + w * 0.5 - 0.5),
          vec4(0, h * 0.5, 0, y + h * 0.5 - 0.5),
          {0, 0, 1, 0},
          {0, 0, 0, 1}};
}

inline float cross(vec2 a, vec2 b) { return a.x * b.y - a.y * b.x; }
inline float dot(vec2 a, vec2 b) { return a.x * b.x + a.y * b.y; }

///// DEBUG /////
inline std::ostream &operator<<(std::ostream &s, const vec4 &v) {
  s << '[' << v.x << ',' << v.y << ',' << v.z << ',' << v.w << ']';
  return s;
}
inline std::ostream &operator<<(std::ostream &s, const vec3 &v) {
  s << '[' << v.x << ',' << v.y << ',' << v.z << ']';
  return s;
}
inline std::ostream &operator<<(std::ostream &s, const vec2 &v) {
  s << '[' << v.x << ',' << v.y << ']';
  return s;
}
inline std::ostream &operator<<(std::ostream &s, const mat3 &m) {
  s << "mat3:" << std::endl;
  s << m.col[0] << std::endl;
  s << m.col[1] << std::endl;
  s << m.col[2] << std::endl;
  return s;
}
inline std::ostream &operator<<(std::ostream &s, const mat4 &m) {
  s << "mat4:" << std::endl;
  s << m.col[0] << std::endl;
  s << m.col[1] << std::endl;
  s << m.col[2] << std::endl;
  s << m.col[3] << std::endl;
  return s;
}
inline std::ostream &operator<<(std::ostream &s, const box2 &b) {
  s << "box2:" << std::endl;
  s << b.min << std::endl;
  s << b.max << std::endl;
  return s;
}
inline std::ostream &operator<<(std::ostream &s, const triangle &t) {
  s << "triangle:" << std::endl;
  s << t.a << std::endl;
  s << t.b << std::endl;
  s << t.c << std::endl;
  return s;
}
} // namespace CppGL