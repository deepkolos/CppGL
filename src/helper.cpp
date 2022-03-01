#include <CppGL/api.h>

namespace CppGL::Helper {
Texture *getTextureFrom(int location) {
  auto state = GLOBAL::GLOBAL_STATE;
  Texture *target = nullptr;
  if (location == GL_TEXTURE_2D) {
    target = state->textureUints[state->ACTIVE_TEXTURE - GL_TEXTURE0].map;
  }
  return target;
}

void setUniform(
    int location,
    std::function<void(rttr::property &, rttr::property &, ShaderSource *,
                       ShaderSource *, rttr::type &, rttr::type &)>
        fn) {
  auto state = GLOBAL::GLOBAL_STATE;
  Program *program = state->CURRENT_PROGRAM;

  if (program == nullptr)
    return;

  auto vertexShader = program->vertexShader->source;
  auto fragmentShader = program->fragmentShader->source;
  auto vertexTypeInfo = vertexShader->get_derived_info().m_type;
  auto fragmentTypeInfo = fragmentShader->get_derived_info().m_type;

  for (auto [name, info] : program->unifroms) {
    if (info.location == location) {
      auto vertexProp = vertexTypeInfo.get_property(name);
      auto fragmentProp = fragmentTypeInfo.get_property(name);
      fn(vertexProp, fragmentProp, vertexShader, fragmentShader, vertexTypeInfo,
         fragmentTypeInfo);
      return;
    }
  }
}

void draw(int mode, int first, int count, int dataType, const void *indices) {
  auto state = GLOBAL::GLOBAL_STATE;
  auto program = state->CURRENT_PROGRAM;
  auto vao = state->VERTEX_ARRAY_BINDING;
  auto fbo = state->FRAMEBUFFER_BINDING;
  auto vertexShader = program->vertexShader->source;
  auto fragmentShader = program->fragmentShader->source;
  auto vertexTypeInfo = vertexShader->get_derived_info().m_type;
  auto fragmentTypeInfo = fragmentShader->get_derived_info().m_type;
  const auto &viewport = state->VIEWPORT;
  const int width = (int)viewport.z;
  const int height = (int)viewport.w;
  std::vector<vec4> clipSpaceVertices(count);

  if (vao == nullptr)
    vao = GLOBAL::DEFAULT_VERTEX_ARRAY;
  if (fbo == nullptr)
    fbo = GLOBAL::DEFAULT_FRAMEBUFFER;

  const void *indicesPtr = nullptr;
  if (indices != nullptr)
    indicesPtr = indices;
  else if (vao->indexBuffer != nullptr)
    indicesPtr = vao->indexBuffer->data;

  const uint8_t *indicesU8Ptr = (uint8_t *)indicesPtr;
  const uint16_t *indicesU16Ptr = (uint16_t *)indicesPtr;

  /**
   * @brief 分配varying内存 count * (varying size 总和)
   * |               内存布局                |
   * | count0             count1            |
   * | varyingA varyingB  varyingA varyingB |
   */
  int varyingNum = 0;
  int varyingSizeSumU8 = 0;
  std::map<str, int> varyingOffsetMap;
  for (auto &prop : vertexTypeInfo.get_properties())
    if (prop.get_metadata(0).get_value<ShaderSourceMeta>() ==
        ShaderSourceMeta::Varying) {
      auto size = prop.get_metadata(1).get_value<size_t>();
      varyingOffsetMap[prop.get_name()] = varyingSizeSumU8;
      varyingSizeSumU8 += size;
      varyingNum++;
    }
  uint8_t *const varyingMemU8 = (uint8_t *)malloc(varyingSizeSumU8 * count);
  uint8_t *const varyingLerpedMemU8 = (uint8_t *)malloc(varyingSizeSumU8);

  /**
   * @brief 初始化frameBuffer zBuffer
   */
  if (nullptr == fbo->COLOR_ATTACHMENT0.attachment &&
      fbo == GLOBAL::DEFAULT_FRAMEBUFFER) {
    // 初始化framebuffer
    {
      int length = sizeof(vec4) * width * height;
      auto texture = new Texture();
      texture->mips.push_back(new TextureBuffer{malloc(length), length, width,
                                                height, GL_RGBA, 0, GL_FLOAT,
                                                GL_RGBA});
      fbo->COLOR_ATTACHMENT0 = {AttachmentType::COLOR_ATTACHMENT0, 0, 0,
                                texture};
    }
    // 初始化zbuffer
    {
      int length = sizeof(float) * width * height;
      auto texture = new Texture();
      texture->mips.push_back(new TextureBuffer{
          malloc(length), length, width, height, GL_DEPTH_COMPONENT32F, 0,
          GL_FLOAT, GL_DEPTH_COMPONENT32F});
      fbo->DEPTH_ATTACHMENT = {AttachmentType::DEPTH_ATTACHMENT, 0, 0, texture};
    }
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  }

  // TODO resize
  auto frameBufferTextureBuffer =
      fbo->COLOR_ATTACHMENT0.attachment->mips[fbo->COLOR_ATTACHMENT0.level];
  auto zBuffer = static_cast<float *>(const_cast<void *>(
      fbo->DEPTH_ATTACHMENT.attachment->mips[fbo->DEPTH_ATTACHMENT.level]
          ->data));

  /**
   * @brief 循环处理顶点
   * 0. 读取attribute 设置到vertex shader
   * 1. 执行vertex shader
   * 2. 收集varying gl_Position
   */
  for (int ii = 0; ii < count; ii++) {
    int i = ii;
    if (dataType == GL_UNSIGNED_SHORT)
      i = indicesU16Ptr[ii];
    if (dataType == GL_UNSIGNED_BYTE)
      i = indicesU8Ptr[ii];

    // 遍历shader里的attribute列表并更新每一轮的值
    for (auto &[name, attr] : program->attributes) {
      auto &info = vao->attributes[attr.location];
      if (!info.enabled)
        continue; // TODO 写入默认值

      size_t componentLen;
      size_t stride = info.stride;

      switch (info.type) {
      case GL_FLOAT:
        componentLen = sizeof(float);
        break;
      case GL_UNSIGNED_BYTE:
        componentLen = sizeof(uint8_t);
        break;
      }

      if (stride == 0)
        stride = info.size * componentLen;

      auto ptr = static_cast<const char *>(info.buffer->data);
      ptr += info.offset + stride * i;

      // 写入attribute到shader的变量
      auto prop = vertexTypeInfo.get_property(name);
      auto var = prop.get_value(*vertexShader);
      auto const varPtr = var.get_value<float *>();
      if (info.type == GL_UNSIGNED_BYTE && info.normalized) {
        auto varPtrI = varPtr;
        for (int j = 0; j < info.size; j++) {
          *varPtrI = static_cast<float>(*((uint8_t *)(ptr) + j)) / 255;
          varPtrI++;
        }
      } else {
        memcpy(varPtr, ptr, stride);
      }
      auto sizeU8 = prop.get_metadata(1).get_value<int>();
      if (sizeU8 == sizeof(vec4) && info.size == 3) {
        *(varPtr + 3) = 1;
      }
      if (sizeU8 == sizeof(vec4) && info.size == 2) {
        *(varPtr + 2) = 0;
        *(varPtr + 3) = 1;
      }
      if (sizeU8 == sizeof(vec4) && info.size == 1) {
        *(varPtr + 1) = 0;
        *(varPtr + 2) = 0;
        *(varPtr + 3) = 1;
      }
    }

    // 执行vertex shader
    vertexTypeInfo.get_method("main").invoke(vertexShader);

    // 收集gl_Position
    clipSpaceVertices[ii] = vertexShader->gl_Position;

    // 收集varying
    size_t offsetU8 = 0;
    for (auto &prop : vertexTypeInfo.get_properties()) {
      if (prop.get_metadata(0).get_value<ShaderSourceMeta>() ==
          ShaderSourceMeta::Varying) {
        auto src = prop.get_value(*vertexShader).get_value<uint8_t *>();
        auto dst = varyingMemU8 + (ii * varyingSizeSumU8) + offsetU8;
        auto sizeU8 = prop.get_metadata(1).get_value<int>();
        memcpy(dst, src, sizeU8);
        offsetU8 += sizeU8;
      }
    }
  }

  auto viewportMatrix = getViewportMatrix(viewport);

  if (mode == GL_TRIANGLES) {
    for (int vertexIndex = 0; vertexIndex < count; vertexIndex += 3) {
      triangle triangleClip{clipSpaceVertices[vertexIndex],
                            clipSpaceVertices[vertexIndex + 1],
                            clipSpaceVertices[vertexIndex + 2]};
      float *varyingA =
          (float *)(varyingMemU8 + vertexIndex * varyingSizeSumU8);
      float *varyingB =
          (float *)(varyingMemU8 + (vertexIndex + 1) * varyingSizeSumU8);
      float *varyingC =
          (float *)(varyingMemU8 + (vertexIndex + 2) * varyingSizeSumU8);
      /**
       * @brief 透视除法
       */
      if (triangleClip.a.w == 0)
        assert(triangleClip.a.w != 0);
      vec3 triangleClipVecW{if0Be1(triangleClip.a.w), if0Be1(triangleClip.b.w),
                            if0Be1(triangleClip.c.w)};
      vec3 triangleClipVecZ{triangleClip.a.z, triangleClip.b.z,
                            triangleClip.c.z};
      // 把齐次坐标系下转为正常坐标系 TODO 理解
      vec3 triangleClipVecZDivZ = triangleClipVecZ / triangleClipVecW;
      triangle triangleViewport = triangleClip * viewportMatrix;
      triangle triangleProjDiv =
          triangleViewport.perspectiveDivide(triangleClipVecW);
      /**
       * @brief 寻找三角形bounding box
       */
      box2 boundingBox = triangleProjDiv.viewportBoundingBox(viewport);
      // box2 boundingBox = {{0, 0}, {(float)width, (float)height}};
      // std::cout << boundingBox << std::endl;
      // std::cout << "triangleClipVecZ:" << triangleClipVecZ << std::endl;
      /**
       * @brief 光栅化rasterization
       */
      // #pragma omp parallel for
      for (int y = (int)boundingBox.min.y; y < (int)boundingBox.max.y; y++) {
        for (int x = (int)boundingBox.min.x; x < (int)boundingBox.max.x; x++) {
          int bufferIndex = x + y * width;
          vec2 positionViewport{(float)x + 0.5f, (float)y + 0.5f};
          vec3 bcScreen = triangleProjDiv.getBarycentric(positionViewport);

          // 不在三角形内 (TODO 理解)
          if (bcScreen.x < 0 || bcScreen.y < 0 || bcScreen.z < 0)
            continue;
          // if (!triangleViewport.contains(positionViewport))
          //   continue;

          vec3 bcClip = bcScreen / triangleClipVecW;
          // TODO 这里还是不懂
          bcClip = bcClip / (bcClip.x + bcClip.y + bcClip.z);

          // 插值得到深度 TODO 理解为什么需要1-z
          float positionDepth =
              1 - triangleClipVecZDivZ.lerpBarycentric(bcClip);
          float zBufferDepth = zBuffer[bufferIndex];

          // 近远平面裁剪 TODO 确认
          if (positionDepth < 0 || positionDepth > 1)
            continue;

          // 或者深度大于已绘制的
          if (state->DEPTH_TEST && zBufferDepth > positionDepth)
            continue;

          // 插值varying(内存区块按照float插值)
          for (int iF32 = 0, ilF32 = varyingSizeSumU8 / sizeof(float);
               iF32 < ilF32; iF32++) {
            vec3 v{*(varyingA + iF32), *(varyingB + iF32), *(varyingC + iF32)};
            *((float *)(varyingLerpedMemU8) + iF32) = v.lerpBarycentric(bcClip);
          }
          // 设置到varying
          int offsetU8 = 0;
          for (auto &prop : fragmentTypeInfo.get_properties())
            if (prop.get_metadata(0).get_value<ShaderSourceMeta>() ==
                ShaderSourceMeta::Varying) {
              auto var = prop.get_value(*fragmentShader);
              auto varPtr = var.get_value<uint8_t *>();
              auto sizeU8 = prop.get_metadata(1).get_value<int>();

              memcpy(varPtr, varyingLerpedMemU8 + offsetU8, sizeU8);
              offsetU8 += sizeU8;
            }

          // 执行fragment shader
          fragmentShader->_discarded = false;
          fragmentTypeInfo.get_method("main").invoke(*fragmentShader);
          if (fragmentShader->_discarded)
            continue;

          auto &color = fragmentShader->gl_FragColor;

          // 更新zBuffer frameBuffer
          zBuffer[bufferIndex] = positionDepth;

          if (frameBufferTextureBuffer->internalFormat == GL_RGBA) {
            if (frameBufferTextureBuffer->dataType == GL_FLOAT) {
              vec4 *frameBuffer = (vec4 *)frameBufferTextureBuffer->data;
              frameBuffer[bufferIndex] = color;
            } else if (frameBufferTextureBuffer->dataType == GL_UNSIGNED_BYTE) {
              uint8_t *frameBuffer = (uint8_t *)frameBufferTextureBuffer->data;
              frameBuffer[bufferIndex * 4] = (uint8_t)(color.r * 255);
              frameBuffer[bufferIndex * 4 + 1] = (uint8_t)(color.g * 255);
              frameBuffer[bufferIndex * 4 + 2] = (uint8_t)(color.b * 255);
              frameBuffer[bufferIndex * 4 + 3] = (uint8_t)(color.a * 255);
            }
          }
        }
      }
    }
  }

  delete varyingMemU8;
  delete varyingLerpedMemU8;
}
} // namespace CppGL::Helper