
#pragma once

#include "constant.h"
#include "attribute.h"
#include "buffer.h"
#include <vector>

namespace CppGL {
struct AttributeInfo {
  bool enabled = false;
  int size = 4;
  int type = GL_FLOAT;
  bool normalized = false;
  int stride = 0;
  int offset = 0;
  int divisor = 0;
  Buffer *buffer = nullptr;
};

struct VertexArray {
  std::vector<AttributeInfo> attributes{};
  Buffer *indexBuffer = nullptr;
};

static VertexArray DEFAULT_VERTEX_ARRAY;
} // namespace CppGL