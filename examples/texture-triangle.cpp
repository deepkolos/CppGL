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
  attribute vec2 uv;

  varying vec2 v_uv;

  uniform mat4 projection;
  uniform mat4 modelView;
  void main() {
    gl_Position = projection * modelView * position;
    v_uv = uv;
    // std::cout << gl_Position << std::endl;
  }

  RTTR_ENABLE(ShaderSource)
} vertexShaderSource;

static struct FragmentShaderSource : ShaderSource {
  varying vec2 v_uv;
  uniform sample2D texture;
  void main() { gl_FragColor = texture2D(texture, v_uv); }

  RTTR_ENABLE(ShaderSource)
} fragmentShaderSource;

CPPGL_RTTR_REGISTRATION {
  using S = VertexShaderSource;
  using M = ShaderSourceMeta;
  registration::class_<S>("VertexShaderSource")
      .CPPGL_RTTR_PROP(position, M::Attribute)
      .CPPGL_RTTR_PROP(uv, M::Attribute)
      .CPPGL_RTTR_PROP(v_uv, M::Varying)
      .CPPGL_RTTR_PROP(projection, M::Uniform)
      .CPPGL_RTTR_PROP(modelView, M::Uniform)
      .method("main", &S::main);
}

CPPGL_RTTR_REGISTRATION {
  using S = FragmentShaderSource;
  using M = ShaderSourceMeta;
  registration::class_<S>("FragmentShaderSource")
      .CPPGL_RTTR_PROP(v_uv, M::Varying)
      .CPPGL_RTTR_PROP(texture, M::Uniform)
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
  glUseProgram(program);
  glViewport(0, 0, 300, 150);

  auto positionLoc = glGetAttribLocation(program, "position");
  auto uvLoc = glGetAttribLocation(program, "uv");
  auto textureLoc = glGetUniformLocation(program, "texture");
  auto projectionLoc = glGetUniformLocation(program, "projection");
  auto modelViewLoc = glGetUniformLocation(program, "modelView");

  // 0 3
  // 1 2
  static float vertexPositions[] = {-1, 1,  // top left
                                    -1, -1, // bottom left
                                    1,  -1, // bottom right
                                    1,  1}; // top right
  static float vertexUVs[] = {0, 1,         // top left
                              0, 0,         // bottom left
                              1, 0,         // bottom right
                              1, 1};        // top right

  // static const uint16_t vertexIndices[] = {1, 2, 3};
  static const uint16_t vertexIndices[] = {0, 1, 2, 0, 2, 3};
  auto positionBuffer = glCreateBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions,
               GL_STATIC_DRAW);

  auto uvBuffer = glCreateBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexUVs), vertexUVs, GL_STATIC_DRAW);

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

  glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
  glEnableVertexAttribArray(uvLoc);
  glVertexAttribPointer(uvLoc,
                        2,        // 2 values per vertex shader iteration
                        GL_FLOAT, // data is 8bit unsigned bytes
                        GL_FALSE, // do normalize
                        0,        // stride (0 = auto)
                        0         // offset into buffer
  );

  static uint8_t checkerTextureData[] = {
      192, 128, 192, 128, 128, 192, 128, 192,
      192, 128, 192, 128, 128, 192, 128, 192,
  };
  const auto checkerTexture = glCreateTexture();
  glBindTexture(GL_TEXTURE_2D, checkerTexture);
  glTexImage2D(GL_TEXTURE_2D,
               0,                // mip level
               GL_LUMINANCE,     // internal format
               4,                // width
               4,                // height
               0,                // border
               GL_LUMINANCE,     // format
               GL_UNSIGNED_BYTE, // type
               checkerTextureData);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  int texUnit = 3;
  glActiveTexture(GL_TEXTURE0 + texUnit);
  glBindTexture(GL_TEXTURE_2D, checkerTexture);
  glUniform1i(textureLoc, texUnit);

  static mat4 projection = mat4::perspective(30 * M_PI / 180, // fov
                                             300.0f / 150,    // aspect
                                             0.1,             // near
                                             20               // far
  );
  glUniformMatrix4fv(projectionLoc, 1, false, &projection);

  // draw center cube
  mat4 modelView;
  modelView.translate(0, 0, -6);
  // modelView.translate(0, 0, -4).xRotate(M_PI_4);
  // modelView.translate(0, 0, -4).yRotate(M_PI_4);
  // modelView.translate(0, 0, -4).xRotate(M_PI_4).yRotate(M_PI_4);
  renderLoop([&]() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    modelView.xRotate(0.05).yRotate(0.1);
    glUniformMatrix4fv(modelViewLoc, 1, false, &modelView);
    // glDrawArrays(GL_TRIANGLES, 0, 3);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    // 显示渲染结果
    displayBuffers(false);
  });
}