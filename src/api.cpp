#include "CppGL/buffer.h"
#include "CppGL/global-state.h"
#include <CppGL/api.h>

namespace CppGL {

void glLinkProgram(Program *program) {
  // 收集shader上的attribute uniform varying 信息到program里
  int attributeIndex = 0;
  int uniformIndex = 0;
  int varyingIndex = 0;
  auto processTypeInfo = [&](rttr::type &&type) {
    for (auto &prop : type.get_properties()) {
      auto name = prop.get_name();
      auto type = prop.get_type().get_name();
      auto attr = prop.get_metadata(0).get_value<ShaderSourceMeta>();
      switch (attr) {
      case ShaderSourceMeta::Attribute:
        program->attributes[name] = {attributeIndex++, type, name};
        break;
      case ShaderSourceMeta::Uniform:
        program->unifroms[name] = {uniformIndex++, type, name};
        break;
      case ShaderSourceMeta::Varying:
        // program->varyings[name] = {varyingIndex++, type, name};
        break;
      }
    }
  };

  processTypeInfo(program->vertexShader->source->get_derived_info().m_type);
  processTypeInfo(program->fragmentShader->source->get_derived_info().m_type);
}

void glTexImage2D(int location, int mipLevel, int internalFormat, int width,
                  int height, int border, int format, int dataType,
                  const void *data) {
  auto state = GLOBAL::GLOBAL_STATE;
  Texture *target = nullptr;
  if (location == GL_TEXTURE_2D) {
    target = state->textureUints[state->ACTIVE_TEXTURE - GL_TEXTURE0].map;
  }
  if (target->mips.size() <= mipLevel)
    target->mips.resize(mipLevel + 1);

  if (data == nullptr) {
    int storeSize = width * height;
    if (internalFormat == GL_RGBA)
      storeSize *= 4;

    if (dataType == GL_UNSIGNED_BYTE)
      data = malloc(sizeof(uint8_t) * storeSize);
    if (dataType == GL_UNSIGNED_SHORT)
      data = malloc(sizeof(uint16_t) * storeSize);
  }

  target->mips[mipLevel] = new TextureBuffer{
      data, 0, width, height, format, border, dataType, internalFormat};
}

void glClear(int mask) {
  auto vao = GLOBAL::GLOBAL_STATE->VERTEX_ARRAY_BINDING;
  auto fbo = GLOBAL::GLOBAL_STATE->FRAMEBUFFER_BINDING;
  const auto &viewport = GLOBAL::GLOBAL_STATE->VIEWPORT;
  const int width = (int)viewport.z;
  const int height = (int)viewport.w;

  if (fbo == nullptr)
    fbo = GLOBAL::DEFAULT_FRAMEBUFFER;

  if (mask | GL_COLOR_BUFFER_BIT &&
      fbo->COLOR_ATTACHMENT0.attachment != nullptr &&
      fbo->COLOR_ATTACHMENT0.attachment->mips.size() != 0) {
    auto frameBufferTextureBuffer =
        fbo->COLOR_ATTACHMENT0.attachment->mips[fbo->COLOR_ATTACHMENT0.level];
    auto color = GLOBAL::GLOBAL_STATE->COLOR_CLEAR_VALUE;
    auto colorRedU8 = (uint8_t)(color.r * 255);
    auto colorGreenU8 = (uint8_t)(color.g * 255);
    auto colorBlueU8 = (uint8_t)(color.g * 255);
    auto colorAlphaU8 = (uint8_t)(color.g * 255);
    // clearColor
    for (int x = 0; x < width; x++) {
      for (int y = 0; y < height; y++) {
        int bufferIndex = x + y * width;
        if (frameBufferTextureBuffer->internalFormat == GL_RGBA) {
          if (frameBufferTextureBuffer->dataType == GL_FLOAT) {
            vec4 *frameBuffer = (vec4 *)frameBufferTextureBuffer->data;
            frameBuffer[bufferIndex] = color;
          } else if (frameBufferTextureBuffer->dataType == GL_UNSIGNED_BYTE) {
            uint8_t *frameBuffer = (uint8_t *)frameBufferTextureBuffer->data;
            frameBuffer[bufferIndex * 4] = colorRedU8;
            frameBuffer[bufferIndex * 4 + 1] = colorGreenU8;
            frameBuffer[bufferIndex * 4 + 2] = colorBlueU8;
            frameBuffer[bufferIndex * 4 + 3] = colorAlphaU8;
          }
        }
      }
    }
  }
  if (mask | GL_DEPTH_BUFFER_BIT &&
      fbo->DEPTH_ATTACHMENT.attachment != nullptr &&
      fbo->DEPTH_ATTACHMENT.attachment->mips.size() != 0) {
    auto zBuffer = static_cast<float *>(
        const_cast<void *>(fbo->DEPTH_ATTACHMENT.attachment->mips[0]->data));

    // 重置zBuffer
    std::fill_n(zBuffer, width * height, -std::numeric_limits<float>::max());
  }
}

void glUniform1i(int location, int value) {
  Helper::setUniform(
      location, [&](rttr::property &vertexProp, rttr::property &fragmentProp,
                    ShaderSource *vertexShader, ShaderSource *fragmentShader,
                    rttr::type &, rttr::type &) {
        if (vertexProp.is_valid()) {
          auto ptr = vertexProp.get_value(*vertexShader).get_value<int *>();
          *ptr = value;
        }
        if (fragmentProp.is_valid()) {
          auto ptr = fragmentProp.get_value(*fragmentShader).get_value<int *>();
          *ptr = value;
        }
      });
}

void glUniform1f(int location, float value) {
  Helper::setUniform(
      location, [&](rttr::property &vertexProp, rttr::property &fragmentProp,
                    ShaderSource *vertexShader, ShaderSource *fragmentShader,
                    rttr::type &, rttr::type &) {
        if (vertexProp.is_valid()) {
          auto ptr = vertexProp.get_value(*vertexShader).get_value<float *>();
          *ptr = value;
        }
        if (fragmentProp.is_valid()) {
          auto ptr =
              fragmentProp.get_value(*fragmentShader).get_value<float *>();
          *ptr = value;
        }
      });
}

void glUniform2fv(int location, int count, const void *data) {
  Helper::setUniform(
      location, [&](rttr::property &vertexProp, rttr::property &fragmentProp,
                    ShaderSource *vertexShader, ShaderSource *fragmentShader,
                    rttr::type &, rttr::type &) {
        if (vertexProp.is_valid()) {
          auto var = vertexProp.get_value(*vertexShader);
          auto varPtr = var.get_value<vec2 *>();
          auto size = vertexProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }

        if (fragmentProp.is_valid()) {
          auto var = fragmentProp.get_value(*fragmentShader);
          auto varPtr = var.get_value<vec2 *>();
          auto size = fragmentProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }
      });
}

void glUniform3fv(int location, int count, const void *data) {
  Helper::setUniform(
      location, [&](rttr::property &vertexProp, rttr::property &fragmentProp,
                    ShaderSource *vertexShader, ShaderSource *fragmentShader,
                    rttr::type &, rttr::type &) {
        if (vertexProp.is_valid()) {
          auto var = vertexProp.get_value(*vertexShader);
          auto varPtr = var.get_value<vec3 *>();
          auto size = vertexProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }

        if (fragmentProp.is_valid()) {
          auto var = fragmentProp.get_value(*fragmentShader);
          auto varPtr = var.get_value<vec3 *>();
          auto size = fragmentProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }
      });
}

void glUniform4fv(int location, int count, const void *data) {
  Helper::setUniform(
      location, [&](rttr::property &vertexProp, rttr::property &fragmentProp,
                    ShaderSource *vertexShader, ShaderSource *fragmentShader,
                    rttr::type &, rttr::type &) {
        if (vertexProp.is_valid()) {
          auto var = vertexProp.get_value(*vertexShader);
          auto varPtr = var.get_value<vec4 *>();
          auto size = vertexProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }

        if (fragmentProp.is_valid()) {
          auto var = fragmentProp.get_value(*fragmentShader);
          auto varPtr = var.get_value<vec4 *>();
          auto size = fragmentProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }
      });
}

void glUniformMatrix3fv(int location, int count, bool transpose,
                        const void *data) {
  Helper::setUniform(
      location, [&](rttr::property &vertexProp, rttr::property &fragmentProp,
                    ShaderSource *vertexShader, ShaderSource *fragmentShader,
                    rttr::type &, rttr::type &) {
        if (vertexProp.is_valid()) {
          auto var = vertexProp.get_value(*vertexShader);
          auto varPtr = var.get_value<mat3 *>();
          auto size = vertexProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }

        if (fragmentProp.is_valid()) {
          auto var = fragmentProp.get_value(*fragmentShader);
          auto varPtr = var.get_value<mat3 *>();
          auto size = fragmentProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }
      });
}

void glUniformMatrix4fv(int location, int count, bool transpose,
                        const void *data) {
  Helper::setUniform(
      location, [&](rttr::property &vertexProp, rttr::property &fragmentProp,
                    ShaderSource *vertexShader, ShaderSource *fragmentShader,
                    rttr::type &, rttr::type &) {
        if (vertexProp.is_valid()) {
          auto var = vertexProp.get_value(*vertexShader);
          auto varPtr = var.get_value<mat4 *>();
          auto size = vertexProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }

        if (fragmentProp.is_valid()) {
          auto var = fragmentProp.get_value(*fragmentShader);
          auto varPtr = var.get_value<mat4 *>();
          auto size = fragmentProp.get_metadata(1).get_value<size_t>();
          memcpy(varPtr, data, size);
        }
      });
}

void glFramebufferTexture2D(int target, int attachment, int textarget,
                            Texture *texture, int level) {
  if (target == GL_FRAMEBUFFER && GLOBAL::GLOBAL_STATE->RENDERBUFFER_BINDING) {
    if (attachment == GL_COLOR_ATTACHMENT0) {
      if (textarget == GL_TEXTURE_2D) {
        GLOBAL::GLOBAL_STATE->FRAMEBUFFER_BINDING->COLOR_ATTACHMENT0
            .attachment = texture;
        GLOBAL::GLOBAL_STATE->FRAMEBUFFER_BINDING->COLOR_ATTACHMENT0.level =
            level;
      }
    }
  }
}

void glFramebufferRenderbuffer(int target, int attachment,
                               int renderbufferTarget,
                               RenderBuffer *renderbuffer) {
  if (target == GL_FRAMEBUFFER && GLOBAL::GLOBAL_STATE->RENDERBUFFER_BINDING) {
    if (attachment == GL_DEPTH_ATTACHMENT) {
      if (renderbufferTarget == GL_RENDERBUFFER) {
        GLOBAL::GLOBAL_STATE->FRAMEBUFFER_BINDING->DEPTH_ATTACHMENT.attachment =
            renderbuffer->attachment;
      }
    }
  }
}

void glRenderbufferStorage(int target, int internalFormat, int width,
                           int height) {
  if (target == GL_RENDERBUFFER) {
    GLOBAL::GLOBAL_STATE->RENDERBUFFER_BINDING->format = internalFormat;
    GLOBAL::GLOBAL_STATE->RENDERBUFFER_BINDING->width = width;
    GLOBAL::GLOBAL_STATE->RENDERBUFFER_BINDING->height = height;

    if (internalFormat == GL_DEPTH_COMPONENT32F) {
      int length = sizeof(float) * width * height;
      auto texture = new Texture();
      texture->mips.push_back(new TextureBuffer{
          malloc(length), length, width, height, GL_DEPTH_COMPONENT32F, 0,
          GL_FLOAT, GL_DEPTH_COMPONENT32F});
      GLOBAL::GLOBAL_STATE->RENDERBUFFER_BINDING->attachment = texture;
    }
  }
}
} // namespace CppGL