#include "CppGL/api.h"
#include "CppGL/attribute.h"
#include "CppGL/data-type.h"
#include "CppGL/math.h"
#include "CppGL/program.h"
#include "CppGL/shader.h"
#include "rttr.h"
#include "rttr/registration.h"
#include "rttr/registration_friend.h"
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

#define attribute [[attribute]]
#define uniform [[uniform]]
#define varying [[varying]]
#define discard return DISCARD()
static struct VertexShaderSource : ShaderSource {
  attribute vec4 position;
  attribute vec4 color;
  varying vec4 v_color;
  void main() {
    std::cout << "vert:" << position << std::endl;
    std::cout << "vert:" << color << std::endl;

    gl_Position = position;
    v_color = color;
  }
} vertexShaderSource;

static struct FragmentShaderSource : ShaderSource {
  varying vec4 v_color;
  void main() {
    // std::cout << "frag:" << v_color << std::endl;
    gl_FragColor = v_color;
  }
} fragmentShaderSource;


#undef attribute
#undef uniform
#undef varying
#undef discard

MY_RTTR_REGISTRATION {
  using S = VertexShaderSource;
  using M = ShaderSourceMeta;
  auto P = policy::prop::bind_as_ptr;
  registration::class_<S>("VertexShaderSource")
      .property("position", &S::position)(metadata(0, M::Attribute),
                                          metadata(1, sizeof(S::position)), P)
      .property("color", &S::color)(metadata(0, M::Attribute),
                                    metadata(1, sizeof(S::color)), P)
      .property("v_color", &S::v_color)(metadata(0, M::Varying),
                                        metadata(1, sizeof(S::v_color)), P)
      .method("main", &S::main);
}

MY_RTTR_REGISTRATION {
  using S = FragmentShaderSource;
  using M = ShaderSourceMeta;
  auto P = policy::prop::bind_as_ptr;
  registration::class_<S>("FragmentShaderSource")
      .property("v_color", &S::v_color)(metadata(0, M::Varying),
                                        metadata(1, sizeof(S::v_color)), P)
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
  glLinkProgram<VertexShaderSource, FragmentShaderSource>(program);

  auto positionLoc = glGetAttribLocation(program, "position");
  auto colorLoc = glGetAttribLocation(program, "color");

  static float vertexPositions[] = {0, 0.7, 0,    1,
                                    0.5, -0.7, 0, 1,   
                                    -.5, -0.7, 0,1};
  static uint8_t vertexColors[] = {255, 0,   0, 255, 0,   255,
                                   0,   255, 0, 0,   255, 255};

  auto positionBuffer = glCreateBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions,
               GL_STATIC_DRAW);

  auto colorBuffer = glCreateBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors,
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(positionLoc);
  glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);

  glVertexAttribPointer(positionLoc,
                        4,        // 2 values per vertex shader iteration
                        GL_FLOAT, // data is 32bit floats
                        GL_FALSE, // don't normalize
                        0,        // stride (0 = auto)
                        0         // offset into buffer
  );

  glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
  glEnableVertexAttribArray(colorLoc);
  glVertexAttribPointer(colorLoc,
                        4, // 4 values per vertex shader iteration
                        GL_UNSIGNED_BYTE, // data is 8bit unsigned bytes
                        GL_TRUE,          // do normalize
                        0,                // stride (0 = auto)
                        0                 // offset into buffer
  );

  glUseProgram(program);
  glDrawArrays<VertexShaderSource, FragmentShaderSource>(GL_TRIANGLES, 0, 3);
}