#pragma once

#include "math.h"
#include "rttr/string_view.h"
#include "shader.h"
#include <vector>

namespace CppGL {

struct Program {
  struct DataInfo {
    int location;
    rttr::string_view type;
    rttr::string_view name;
  };

  Shader *vertexShader = nullptr;
  Shader *fragmentShader = nullptr;
  std::map<rttr::string_view, DataInfo> unifroms{};
  std::map<rttr::string_view, DataInfo> attributes{};
  // std::map<rttr::string_view, DataInfo> varyings{};
};

} // namespace CppGL
