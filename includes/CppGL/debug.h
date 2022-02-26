#include <CppGL/math.h>

namespace CppGL {
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