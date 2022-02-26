#include "CppGL/buffer.h"
#include "CppGL/vertex-array.h"
#include <CppGL/global-state.h>

namespace CppGL {
// bool GLOBAL::initialized = false;
// GlobalState *GLOBAL::GLOBAL_STATE = nullptr;
// FrameBuffer *GLOBAL::DEFAULT_FRAMEBUFFER = nullptr;
// RenderBuffer *GLOBAL::DEFAULT_RENDERBUFFER = nullptr;
// VertexArray *GLOBAL::DEFAULT_VERTEX_ARRAY = nullptr;

GlobalState *GLOBAL::GLOBAL_STATE = new GlobalState();
FrameBuffer *GLOBAL::DEFAULT_FRAMEBUFFER = new FrameBuffer();
RenderBuffer *GLOBAL::DEFAULT_RENDERBUFFER = new RenderBuffer();
VertexArray *GLOBAL::DEFAULT_VERTEX_ARRAY = new VertexArray();

// void GLOBAL::init() {
//   if (initialized)
//     return;
//   initialized = true;

//   // GLOBAL::GLOBAL_STATE = new GlobalState();
//   // GLOBAL::DEFAULT_FRAMEBUFFER = new FrameBuffer();
//   // GLOBAL::DEFAULT_RENDERBUFFER = new RenderBuffer();
//   // GLOBAL::DEFAULT_VERTEX_ARRAY = new VertexArray();
// }
} // namespace CppGL