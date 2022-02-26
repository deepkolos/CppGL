#include "utils.h"
#include <CppGL/api.h>
#include <iostream>

/**
 * @brief demo code from
 * @see
 * https://webglfundamentals.org/webgl/lessons/resources/webgl-state-diagram.html?exampleId=rainbow-triangle
 */

using namespace CppGL;
using namespace rttr;

#include "CppGL/marco.h" // 必须在所有include后面

static struct VertexShaderSource : ShaderSource {
  attribute vec4 position;
  attribute vec4 color;
  varying vec4 v_color;
  void main() {
    std::cout << "vert: position" << position << std::endl;
    std::cout << "vert: color   " << color << std::endl;

    gl_Position = position;
    v_color = color;
  }

  RTTR_ENABLE(ShaderSource)
} vertexShaderSource;

static struct FragmentShaderSource : ShaderSource {
  varying vec4 v_color;
  void main() {
    // std::cout << "frag:" << v_color << std::endl;
    gl_FragColor = v_color;
  }

  RTTR_ENABLE(ShaderSource)
} fragmentShaderSource;

CPPGL_RTTR_REGISTRATION {
  using S = VertexShaderSource;
  using M = ShaderSourceMeta;
  registration::class_<S>("VertexShaderSource")
      .CPPGL_RTTR_PROP(position, M::Attribute)
      .CPPGL_RTTR_PROP(color, M::Attribute)
      .CPPGL_RTTR_PROP(v_color, M::Varying)
      .method("main", &S::main);
}

CPPGL_RTTR_REGISTRATION {
  using S = FragmentShaderSource;
  using M = ShaderSourceMeta;
  registration::class_<S>("FragmentShaderSource")
      .CPPGL_RTTR_PROP(v_color, M::Varying)
      .method("main", &S::main);
}

int main(void) {
  auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, &vertexShaderSource);
  glCompileShader(vertexShader);

  auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, &fragmentShaderSource);
  glCompileShader(fragmentShader);

  auto program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  auto positionLoc = glGetAttribLocation(program, "position");
  auto colorLoc = glGetAttribLocation(program, "color");

  static float vertexPositions[] = {0, 0.7, 0.5, -0.7, -0.5, -0.7};
  static uint8_t vertexColors[] = {255, 0, 0, 0, 255, 0, 0, 0, 255};

  static const uint16_t vertexIndices[] = {0, 1, 2};
  auto positionBuffer = glCreateBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions,
               GL_STATIC_DRAW);

  auto colorBuffer = glCreateBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors,
               GL_STATIC_DRAW);

  const auto indexBuffer = glCreateBuffer();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertexIndices), vertexIndices,
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(positionLoc);
  glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);

  glVertexAttribPointer(positionLoc,
                        2,        // 2 values per vertex shader iteration
                        GL_FLOAT, // data is 32bit floats
                        GL_FALSE, // don't normalize
                        0,        // stride (0 = auto)
                        0         // offset into buffer
  );

  glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
  glEnableVertexAttribArray(colorLoc);
  glVertexAttribPointer(colorLoc,
                        3, // 3 values per vertex shader iteration
                        GL_UNSIGNED_BYTE, // data is 8bit unsigned bytes
                        GL_TRUE,          // do normalize
                        0,                // stride (0 = auto)
                        0                 // offset into buffer
  );
  glViewport(0, 0, 300, 150);
  glUseProgram(program);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  // glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0);

  // 显示渲染结果
  displayBuffers();
}