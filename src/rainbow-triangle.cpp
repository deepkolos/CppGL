#include "CppGL/api.h"
#include "CppGL/attribute.h"
#include "CppGL/constant.h"
#include "CppGL/data-type.h"
#include "CppGL/math.h"
#include "CppGL/program.h"
#include "CppGL/shader.h"
#include "rttr.h"
#include "rttr/registration.h"
#include "rttr/registration_friend.h"
#include "utils.h"
#include <_types/_uint8_t.h>
#include <iostream>
#include <map>
#include <rttr/registration>
#include <rttr/type>

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

MY_RTTR_REGISTRATION {
  using S = VertexShaderSource;
  using M = ShaderSourceMeta;
  registration::class_<S>("VertexShaderSource")
      .MY_RTTR_PROP(position, M::Attribute)
      .MY_RTTR_PROP(color, M::Attribute)
      .MY_RTTR_PROP(v_color, M::Varying)
      .method("main", &S::main);
}

MY_RTTR_REGISTRATION {
  using S = FragmentShaderSource;
  using M = ShaderSourceMeta;
  registration::class_<S>("FragmentShaderSource")
      .MY_RTTR_PROP(v_color, M::Varying)
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