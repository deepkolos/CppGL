#pragma once
#include "shader.h"

namespace CppGL {
const int GL_ARRAY_BUFFER = 0;
const int GL_TRIANGLES = 1;
const int GL_STATIC_DRAW = 2;
const int GL_FLOAT = 3;
const int GL_UNSIGNED_BYTE = 4;
const int GL_ELEMENT_ARRAY_BUFFER = 5;
const int GL_TEXTURE_2D = 6;
const int GL_LUMINANCE = 7;
const int GL_TEXTURE_MIN_FILTER = 8;
const int GL_TEXTURE_MAG_FILTER = 9;
const int GL_NEAREST = 10;
const int GL_TEXTURE_WRAP_S = 11;
const int GL_TEXTURE_WRAP_T = 12;
const int GL_CLAMP_TO_EDGE = 13;
const int GL_RGBA = 14;
const int GL_DEPTH_COMPONENT32F = 15;
const int GL_COLOR_BUFFER_BIT = 1;
const int GL_DEPTH_BUFFER_BIT = 2;
const int GL_DEPTH_TEST = 17;
const int GL_CULL_FACE = 18;
const int GL_TEXTURE0 = 19;
const int GL_TEXTURE1 = 20;
const int GL_TEXTURE2 = 21;
const int GL_TEXTURE3 = 22;
const int GL_TEXTURE4 = 23;
const int GL_TEXTURE5 = 24;
const int GL_TEXTURE6 = 25;
const int GL_TEXTURE7 = 26;
const int GL_TEXTURE8 = 27;
const int GL_TEXTURE9 = 28;
const int GL_UNSIGNED_SHORT = 29;
const bool GL_FALSE = false;
const bool GL_TRUE = true;
const auto GL_VERTEX_SHADER = Shader::VERTEX_SHADER;
const auto GL_FRAGMENT_SHADER = Shader::FRAGMENT_SHADER;
} // namespace CppGL