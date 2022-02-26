#pragma once

#include <vector>
namespace CppGL {
struct Texture;
struct Buffer {
  const void *data;
  int length;
};

enum AttachmentType { COLOR_ATTACHMENT0, DEPTH_ATTACHMENT };
struct AttachmentInfo {
  AttachmentType type;
  int level;
  int face;
  Texture *attachment;
};
struct FrameBuffer {
  AttachmentInfo COLOR_ATTACHMENT0;
  AttachmentInfo DEPTH_ATTACHMENT;
};

struct RenderBuffer {
  int width;
  int height;
  int format;
  Texture *attachment;
};
} // namespace CppGL