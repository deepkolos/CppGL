#include <CppGL/math.h>
#include <CppGL/rttr.h>

namespace CppGL {
CPPGL_RTTR_REGISTRATION {
  rttr::registration::class_<vec2>("vec2").constructor();
  rttr::registration::class_<vec3>("vec3").constructor();
  rttr::registration::class_<vec4>("vec4").constructor();
  rttr::registration::class_<mat3>("mat3").constructor();
  rttr::registration::class_<mat4>("mat4").constructor();
}
} // namespace CppGL
