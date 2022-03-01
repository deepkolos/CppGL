#pragma once
#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>
#include <vector>
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
  inline vec3 operator*(vec3 v) { return {x * v.x, y * v.y, z * v.z}; }
  inline vec3 operator+(vec3 v) { return {x + v.x, y + v.y, z + v.z}; }
  inline vec3 operator-(vec3 v) { return {x - v.x, y - v.y, z - v.z}; }
  inline vec3 operator/(float f) { return {x / f, y / f, z / f}; }
  inline vec3 operator*(float f) { return {x * f, y * f, z * f}; }
  inline vec3 operator+(float f) { return {x + f, y + f, z + f}; }
  inline vec3 operator-(float f) { return {x - f, y - f, z - f}; }

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
  vec4(vec3 v, float w = 0) : x(v.x), y(v.y), z(v.z), w(w) {}

  inline vec4 operator*(vec4 v) { return {x * v.x, y * v.y, z * v.z, w * v.w}; }
  inline vec4 operator/(vec4 v) { return {x / v.x, y / v.y, z / v.z, w / v.w}; }
  inline vec4 operator+(vec4 v) { return {x + v.x, y + v.y, z + v.z, w + v.w}; }
  inline vec4 operator-(vec4 v) { return {x - v.x, y - v.y, z - v.z, w - v.w}; }
  inline vec4 operator/(float f) { return {x / f, y / f, z / f, w / f}; }
  inline vec4 operator*(float f) { return {x * f, y * f, z * f, w * f}; }
  inline vec4 operator+(float f) { return {x + f, y + f, z + f, w + f}; }
  inline vec4 operator-(float f) { return {x - f, y - f, z - f, w - f}; }
  inline vec4 &operator*=(float f) {
    *this = {x * f, y * f, z * f, w * f};
    return *this;
  }
};
struct mat4;
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
  mat3(mat4 m);

  inline vec3 operator*(vec3 v) {
    return {
        m0 * v.x + m3 * v.y + m6 * v.z,
        m1 * v.x + m4 * v.y + m7 * v.z,
        m2 * v.x + m5 * v.y + m8 * v.z,
    };
  }
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

  inline mat4() { identity(); }
  inline mat4(float m0, float m1, float m2, float m3, float m4, float m5,
              float m6, float m7, float m8, float m9, float m10, float m11,
              float m12, float m13, float m14, float m15)
      : m0(m0), m1(m4), m2(m8), m3(m12), m4(m1), m5(m5), m6(m9), m7(m13),
        m8(m2), m9(m6), m10(m10), m11(m14), m12(m3), m13(m7), m14(m11),
        m15(m15) {}
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

  inline mat4 operator*(mat4 m) {
    const float a11 = e[0], a12 = e[4], a13 = e[8], a14 = e[12];
    const float a21 = e[1], a22 = e[5], a23 = e[9], a24 = e[13];
    const float a31 = e[2], a32 = e[6], a33 = e[10], a34 = e[14];
    const float a41 = e[3], a42 = e[7], a43 = e[11], a44 = e[15];
    const float b11 = m.e[0], b12 = m.e[4], b13 = m.e[8], b14 = m.e[12];
    const float b21 = m.e[1], b22 = m.e[5], b23 = m.e[9], b24 = m.e[13];
    const float b31 = m.e[2], b32 = m.e[6], b33 = m.e[10], b34 = m.e[14];
    const float b41 = m.e[3], b42 = m.e[7], b43 = m.e[11], b44 = m.e[15];
    mat4 out;
    out.e[0] = a11 * b11 + a12 * b21 + a13 * b31 + a14 * b41;
    out.e[1] = a21 * b11 + a22 * b21 + a23 * b31 + a24 * b41;
    out.e[2] = a31 * b11 + a32 * b21 + a33 * b31 + a34 * b41;
    out.e[3] = a41 * b11 + a42 * b21 + a43 * b31 + a44 * b41;

    out.e[4] = a11 * b12 + a12 * b22 + a13 * b32 + a14 * b42;
    out.e[5] = a21 * b12 + a22 * b22 + a23 * b32 + a24 * b42;
    out.e[6] = a31 * b12 + a32 * b22 + a33 * b32 + a34 * b42;
    out.e[7] = a41 * b12 + a42 * b22 + a43 * b32 + a44 * b42;

    out.e[8] = a11 * b13 + a12 * b23 + a13 * b33 + a14 * b43;
    out.e[9] = a21 * b13 + a22 * b23 + a23 * b33 + a24 * b43;
    out.e[10] = a31 * b13 + a32 * b23 + a33 * b33 + a34 * b43;
    out.e[12] = a11 * b14 + a12 * b24 + a13 * b34 + a14 * b44;

    out.e[13] = a21 * b14 + a22 * b24 + a23 * b34 + a24 * b44;
    out.e[14] = a31 * b14 + a32 * b24 + a33 * b34 + a34 * b44;
    out.e[11] = a41 * b13 + a42 * b23 + a43 * b33 + a44 * b43;
    out.e[15] = a41 * b14 + a42 * b24 + a43 * b34 + a44 * b44;
    return out;
  }

  inline static mat4 perspective(float fieldOfViewInRadians, float aspect,
                                 float near, float far) {
    float f = std::tanf(M_PI * 0.5 - 0.5 * fieldOfViewInRadians);
    float rangeInv = 1.0 / (near - far);

    return {vec4{f / aspect, 0, 0, 0}, vec4{0, f, 0, 0},
            vec4{0, 0, (near + far) * rangeInv, -1},
            vec4{0, 0, near * far * rangeInv * 2, 0}};
  }
  inline mat4 &identity() {
    *this = {vec4{1, 0, 0, 0}, vec4{0, 1, 0, 0}, vec4{0, 0, 1, 0},
             vec4{0, 0, 0, 1}};
    return *this;
  }
  inline mat4 &translate(float x, float y, float z) {
    e[12] += x;
    e[13] += y;
    e[14] += z;
    return *this;
  }
  inline mat4 &yRotate(float angleInRadians) {
    float c = std::cosf(angleInRadians);
    float s = std::sinf(angleInRadians);

    mat4 rotate{vec4{c, 0, -s, 0}, vec4{0, 1, 0, 0}, vec4{s, 0, c, 0},
                vec4{0, 0, 0, 1}};
    *this = *this * rotate;
    return *this;
  }
  inline mat4 &xRotate(float angleInRadians) {
    float c = std::cosf(angleInRadians);
    float s = std::sinf(angleInRadians);

    mat4 rotate{vec4{1, 0, 0, 0}, vec4{0, c, s, 0}, vec4{0, -s, c, 0},
                vec4{0, 0, 0, 1}};
    *this = *this * rotate;
    return *this;
  }
  inline mat4 &zRotate(float angleInRadians) {
    float c = std::cosf(angleInRadians);
    float s = std::sinf(angleInRadians);

    mat4 rotate{vec4{1, 0, 0, 0}, vec4{0, c, s, 0}, vec4{0, -s, c, 0},
                vec4{0, 0, 0, 1}};
    *this = *this * rotate;
    return *this;
  }
  inline mat4 &scale(float x) {
    col0 *= x;
    col1 *= x;
    col2 *= x;
    return *this;
  }

  inline mat4 &from(std::vector<double> &arr) {
    if (arr.size() == 16) {
      e[0] = arr[0];
      e[1] = arr[1];
      e[2] = arr[2];
      e[3] = arr[3];
      e[4] = arr[4];
      e[5] = arr[5];
      e[6] = arr[6];
      e[7] = arr[7];
      e[8] = arr[8];
      e[9] = arr[9];
      e[10] = arr[10];
      e[11] = arr[11];
      e[12] = arr[12];
      e[13] = arr[13];
      e[14] = arr[14];
      e[15] = arr[15];
    }
    return *this;
  }

  mat4 &from(std::vector<double> &translation, std::vector<double> &quaternion,
             std::vector<double> &scale);
  mat4 &invert();
};

inline mat3::mat3(mat4 m)
    : m0(m.m0), m1(m.m1), m2(m.m2), m3(m.m4), m4(m.m5), m5(m.m6), m6(m.m8),
      m7(m.m9), m8(m.m10){};

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

///// methods ////

inline mat4 getViewportMatrix(vec4 v) {
  auto x = v.x;
  auto y = v.y;
  auto w = v.z;
  auto h = v.w;
  return {w * 0.5f, 0,        0, x + w * 0.5f - 0.5f,
          0,        h * 0.5f, 0, y + h * 0.5f - 0.5f,
          0,        0,        1, 0,
          0,        0,        0, 1};
}

inline float clamp(float a, float min, float max) {
  return std::min(std::max(a, min), max);
}
inline vec2 clamp(vec2 v, float min, float max) {
  return {clamp(v.x, min, max), clamp(v.y, min, max)};
}
inline vec3 clamp(vec3 v, float min, float max) {
  return {clamp(v.x, min, max), clamp(v.y, min, max), clamp(v.z, min, max)};
}
inline vec4 clamp(vec4 v, float min, float max) {
  return {clamp(v.x, min, max), clamp(v.y, min, max), clamp(v.z, min, max),
          clamp(v.w, min, max)};
}

inline float length(vec2 v) { return std::sqrtf(v.x * v.x + v.y * v.y); }
inline float length(vec3 v) {
  return std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}
inline float length(vec4 v) {
  return std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

inline vec2 normalize(vec2 v) { return v / length(v); }
inline vec3 normalize(vec3 v) { return v / length(v); }
inline vec4 normalize(vec4 v) { return v / length(v); }

inline float cross(vec2 a, vec2 b) { return a.x * b.y - a.y * b.x; }
inline float dot(vec2 a, vec2 b) { return a.x * b.x + a.y * b.y; }

} // namespace CppGL