#pragma once
#include "shader.h"

namespace CppGL {
const int GL_ARRAY_BUFFER = 0;
const int GL_TRIANGLES = 1;
const int GL_STATIC_DRAW = 2;
const int GL_FLOAT = 3;
const int GL_UNSIGNED_BYTE = 4;
const bool GL_FALSE = false;
const bool GL_TRUE = true;
const auto GL_VERTEX_SHADER = Shader::VERTEX_SHADER;
const auto GL_FRAGMENT_SHADER = Shader::FRAGMENT_SHADER;
} // namespace CppGL