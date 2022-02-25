#pragma once

#include "buffer.h"
#include "constant.h"
#include "data-type.h"
#include "math.h"
#include "texture.h"
#include <vector>

namespace CppGL {
struct Program;
struct Buffer;
struct VertexArray;

struct GlobalState {
  // common state
  vec4 VIEWPORT{0, 0, 300, 150};
  Buffer *ARRAY_BUFFER_BINDING = nullptr;
  Program *CURRENT_PROGRAM = nullptr;
  VertexArray *VERTEX_ARRAY_BINDING = nullptr;
  RenderBuffer *RENDERBUFFER_BINDING = nullptr;
  FrameBuffer *FRAMEBUFFER_BINDING = nullptr;
  int ACTIVE_TEXTURE = GL_TEXTURE0;

  // texture units
  std::vector<TextureUnit> textureUints{10};

  // clear state
  vec4 COLOR_CLEAR_VALUE;
  float DEPATH_CLEAR_VALUE = 1;
  int STENCIL_CLEAR_VALUE = 0x00;

  // depth state
  bool DEPTH_TEST = false;
  DepthFunc DEPTH_FUNC;
  vec2 DEPTH_RANGE{0, 1};
  bool DEPTH_WRITEMASK = true;

  // blend state
  bool BLEND = false;
  BlendFunc BLEND_DST_RGB;
  BlendFunc BLEND_SRC_RGB;
  BlendFunc BLEND_DST_ALPHA;
  BlendFunc BLEND_SRC_ALPHA;
  vec4 BLEND_COLOR;
  BlendEquation BLEND_EQUATION_RGB;
  BlendEquation BLEND_EQUATION_ALPHA;

  // misc state
  int COLOR_WRITEMASK;
  bool SCISSOR_TEST;
  vec4 SCISSOR_BOX;
  int UNPACK_ALIGNMENT = 4;
  int PACK_ALIGNMENT = 4;

  // stencil state
  bool STENCIL_TEST = false;
  StencilFunc STENCIL_FUNC;
  StencilAction STENCIL_FAIL;
  StencilAction STENCIL_PASS_DEPTH_FAIL;
  StencilAction STENCIL_PASS_DEPTH_PASS;
  int STENCIL_REF = 0x00;
  int STENCIL_VALUE_MASK = 0xff;
  int STENCIL_WRITE_MASK = 0xff;
  StencilFunc STENCIL_BACK_FUNC;
  StencilAction STENCIL_BACK_FAIL;
  StencilAction STENCIL_BACK_PASS_DEPTH_FAIL;
  StencilAction STENCIL_BACK_PASS_DEPTH_PASS;
  int STENCIL_BACK_REF = 0x00;
  int STENCIL_BACK_VALUE_MASK = 0xff;
  int STENCIL_BACK_WRITE_MASK = 0xff;

  // polygon state
  bool CULL_FACE;
  CullFaceMode CULL_FACE_MODE;
  FrontFace FRONT_FACE;
  int POLYGON_OFFSET_UNITS = 0;
  int POLYGON_OFFSET_FACTOR = 0;
};

static GlobalState GLOBAL_STATE;
} // namespace CppGL