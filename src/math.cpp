#include <CppGL/math.h>
#include <CppGL/rttr.h>

namespace CppGL {

mat4 &mat4::from(std::vector<double> &translation,
                 std::vector<double> &quaternion, std::vector<double> &scale) {
  if (translation.size() == 3 && quaternion.size() == 4 && scale.size() == 3) {

    const float x = quaternion[0], y = quaternion[1], z = quaternion[2],
                w = quaternion[3];
    const float x2 = x + x, y2 = y + y, z2 = z + z;
    const float xx = x * x2, xy = x * y2, xz = x * z2;
    const float yy = y * y2, yz = y * z2, zz = z * z2;
    const float wx = w * x2, wy = w * y2, wz = w * z2;

    const float sx = scale[0], sy = scale[1], sz = scale[2];

    e[0] = (1 - (yy + zz)) * sx;
    e[1] = (xy + wz) * sx;
    e[2] = (xz - wy) * sx;
    e[3] = 0;

    e[4] = (xy - wz) * sy;
    e[5] = (1 - (xx + zz)) * sy;
    e[6] = (yz + wx) * sy;
    e[7] = 0;

    e[8] = (xz + wy) * sz;
    e[9] = (yz - wx) * sz;
    e[10] = (1 - (xx + yy)) * sz;
    e[11] = 0;

    e[12] = translation[0];
    e[13] = translation[1];
    e[14] = translation[2];
    e[15] = 1;
  }
  return *this;
}

mat4 &mat4::invert() {
  float a00 = e[0], a01 = e[1], a02 = e[2], a03 = e[3];
  float a10 = e[4], a11 = e[5], a12 = e[6], a13 = e[7];
  float a20 = e[8], a21 = e[9], a22 = e[10], a23 = e[11];
  float a30 = e[12], a31 = e[13], a32 = e[14], a33 = e[15];

  float b00 = a00 * a11 - a01 * a10;
  float b01 = a00 * a12 - a02 * a10;
  float b02 = a00 * a13 - a03 * a10;
  float b03 = a01 * a12 - a02 * a11;
  float b04 = a01 * a13 - a03 * a11;
  float b05 = a02 * a13 - a03 * a12;
  float b06 = a20 * a31 - a21 * a30;
  float b07 = a20 * a32 - a22 * a30;
  float b08 = a20 * a33 - a23 * a30;
  float b09 = a21 * a32 - a22 * a31;
  float b10 = a21 * a33 - a23 * a31;
  float b11 = a22 * a33 - a23 * a32;

  // Calculate the determinant
  float det =
      b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;

  if (!det)
    return *this;

  det = 1.0 / det;

  e[0] = (a11 * b11 - a12 * b10 + a13 * b09) * det;
  e[1] = (a02 * b10 - a01 * b11 - a03 * b09) * det;
  e[2] = (a31 * b05 - a32 * b04 + a33 * b03) * det;
  e[3] = (a22 * b04 - a21 * b05 - a23 * b03) * det;
  e[4] = (a12 * b08 - a10 * b11 - a13 * b07) * det;
  e[5] = (a00 * b11 - a02 * b08 + a03 * b07) * det;
  e[6] = (a32 * b02 - a30 * b05 - a33 * b01) * det;
  e[7] = (a20 * b05 - a22 * b02 + a23 * b01) * det;
  e[8] = (a10 * b10 - a11 * b08 + a13 * b06) * det;
  e[9] = (a01 * b08 - a00 * b10 - a03 * b06) * det;
  e[10] = (a30 * b04 - a31 * b02 + a33 * b00) * det;
  e[11] = (a21 * b02 - a20 * b04 - a23 * b00) * det;
  e[12] = (a11 * b07 - a10 * b09 - a12 * b06) * det;
  e[13] = (a00 * b09 - a01 * b07 + a02 * b06) * det;
  e[14] = (a31 * b01 - a30 * b03 - a32 * b00) * det;
  e[15] = (a20 * b03 - a21 * b01 + a22 * b00) * det;
  return *this;
}

CPPGL_RTTR_REGISTRATION {
  rttr::registration::class_<vec2>("vec2").constructor();
  rttr::registration::class_<vec3>("vec3").constructor();
  rttr::registration::class_<vec4>("vec4").constructor();
  rttr::registration::class_<mat3>("mat3").constructor();
  rttr::registration::class_<mat4>("mat4").constructor();
}
} // namespace CppGL
