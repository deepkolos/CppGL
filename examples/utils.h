#pragma once
#include <CppGL/buffer.h>
#include <CppGL/constant.h>
#include <CppGL/global-state.h>
#include <CppGL/math.h>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <thread>

using namespace cv;
using namespace CppGL;

inline void displayBuffers(bool wait = true) {
  const auto &viewport = GLOBAL::GLOBAL_STATE->VIEWPORT;
  const auto width = std::ceilf(viewport.z);
  const auto height = std::ceilf(viewport.w);
  auto fbo = GLOBAL::GLOBAL_STATE->FRAMEBUFFER_BINDING;
  if (fbo == nullptr)
    fbo = GLOBAL::DEFAULT_FRAMEBUFFER;
  auto frameBufferTextureBuffer =
      fbo->COLOR_ATTACHMENT0.attachment->mips[fbo->COLOR_ATTACHMENT0.level];
  auto zBuffer = static_cast<float *>(
      const_cast<void *>(fbo->DEPTH_ATTACHMENT.attachment->mips[0]->data));

  Mat image = Mat::zeros(height, width, CV_8UC3);
  Mat imageZ = Mat::zeros(height, width, CV_8UC3);

#pragma omp parallel for
  for (int x = 0; x < (int)width; x++)
    for (int y = 0; y < (int)height; y++) {
      int bufferIndex = x + y * width;
      float depth = clamp(zBuffer[bufferIndex], 0, 1);
      int pixelIndex = (x + (height - y) * width) * 3;
      if (frameBufferTextureBuffer->internalFormat == GL_RGBA) {
        if (frameBufferTextureBuffer->dataType == GL_FLOAT) {
          vec4 *frameBuffer = (vec4 *)frameBufferTextureBuffer->data;
          vec4 &pixel = frameBuffer[bufferIndex];
          image.data[pixelIndex] = (uchar)(pixel.b * 255);
          image.data[pixelIndex + 1] = (uchar)(pixel.g * 255);
          image.data[pixelIndex + 2] = (uchar)(pixel.r * 255);
        } else if (frameBufferTextureBuffer->dataType == GL_UNSIGNED_BYTE) {
          uint8_t *frameBuffer = (uint8_t *)frameBufferTextureBuffer->data;
          image.data[pixelIndex] = frameBuffer[bufferIndex * 4 + 2];
          image.data[pixelIndex + 1] = frameBuffer[bufferIndex * 4 + 1];
          image.data[pixelIndex + 2] = frameBuffer[bufferIndex * 4];
        }
      }

      // image.data[pixelIndex] = 255;   // b
      // image.data[pixelIndex + 1] = 0; // g
      // image.data[pixelIndex + 2] = 0; // r

      imageZ.data[pixelIndex] = (uchar)(depth * 255);
      imageZ.data[pixelIndex + 1] = (uchar)(depth * 255);
      imageZ.data[pixelIndex + 2] = (uchar)(depth * 255);
    }

  imshow("zBuffer", imageZ);
  imshow("frameBuffer", image);
  if (wait)
    waitKey(0);
}

inline void renderLoop(std::function<void(void)> fn) {
  while (1) {
    fn();
    // Press  ESC on keyboard to exit
    char c = (char)waitKey(25);
    if (c == 27)
      break;

    std::this_thread::sleep_for(std::chrono::microseconds(16));
  }
}