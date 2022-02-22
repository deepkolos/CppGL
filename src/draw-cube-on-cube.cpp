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
 * https://webglfundamentals.org/webgl/lessons/resources/webgl-state-diagram.html?exampleId=draw-cube-on-cube#no-help
 */

using namespace CppGL;
using namespace rttr;

#include "CppGL/marco.h" // 必须在所有include后面

static struct VertexShaderSource : ShaderSource {
  attribute vec4 position;
  attribute vec3 normal;
  attribute vec2 texcoord;

  uniform mat4 projection;
  uniform mat4 modelView;

  varying vec3 v_normal;
  varying vec2 v_texcoord;
  void main() {
    gl_Position = projection * modelView * position;
    v_normal = mat3(modelView) * normal;
    v_texcoord = texcoord;
  }

  RTTR_ENABLE(ShaderSource)
} vertexShaderSource;

static struct FragmentShaderSource : ShaderSource {
  varying vec3 v_normal;
  varying vec2 v_texcoord;

  uniform sampler2D diffuse;
  uniform sampler2D decal;
  uniform vec4 diffuseMult;
  uniform vec3 lightDir;
  void main() {
    vec3 normal = normalize(v_normal);
    float light = dot(normal, lightDir) * 0.5f + 0.5f;
    vec4 color = texture2D(diffuse, v_texcoord) * diffuseMult;
    vec4 decalColor = texture2D(decal, v_texcoord);
    // decalColor.rgb *= decalColor.a;
    decalColor.r *= decalColor.a;
    decalColor.g *= decalColor.a;
    decalColor.b *= decalColor.a;
    color = color * (1.0 - decalColor.a) + decalColor;
    // gl_FragColor = vec4(color.rgb * light, color.a);
    gl_FragColor = vec4(vec3(color.r, color.g, color.b) * light, color.a);
  }

  RTTR_ENABLE(ShaderSource)
} fragmentShaderSource;

MY_RTTR_REGISTRATION {
  using S = VertexShaderSource;
  using M = ShaderSourceMeta;
  registration::class_<S>("VertexShaderSource")
      .MY_RTTR_PROP(position, M::Attribute)
      .MY_RTTR_PROP(normal, M::Attribute)
      .MY_RTTR_PROP(texcoord, M::Attribute)
      .MY_RTTR_PROP(projection, M::Uniform)
      .MY_RTTR_PROP(modelView, M::Uniform)
      .MY_RTTR_PROP(v_normal, M::Varying)
      .MY_RTTR_PROP(v_texcoord, M::Varying)
      .method("main", &S::main);
}

MY_RTTR_REGISTRATION {
  using S = FragmentShaderSource;
  using M = ShaderSourceMeta;
  registration::class_<S>("FragmentShaderSource")
      .MY_RTTR_PROP(v_normal, M::Varying)
      .MY_RTTR_PROP(v_texcoord, M::Varying)
      .MY_RTTR_PROP(diffuse, M::Uniform)
      .MY_RTTR_PROP(decal, M::Uniform)
      .MY_RTTR_PROP(diffuseMult, M::Uniform)
      .MY_RTTR_PROP(lightDir, M::Uniform)
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
  auto normalLoc = glGetAttribLocation(program, "normal");
  auto texcoordLoc = glGetAttribLocation(program, "texcoord");

  auto projectionLoc = glGetUniformLocation(program, "projection");
  auto modelViewLoc = glGetUniformLocation(program, "modelView");
  auto diffuseLoc = glGetUniformLocation(program, "diffuse");
  auto decalLoc = glGetUniformLocation(program, "decal");
  auto diffuseMultLoc = glGetUniformLocation(program, "diffuseMult");
  auto lightDirLoc = glGetUniformLocation(program, "lightDir");

  // vertex positions for a cube
  static const float cubeVertexPositions[] = {
      1,  1,  -1, 1,  1,  1,  1,  -1, 1,  1,  -1, -1, -1, 1,  1,  -1, 1,  -1,
      -1, -1, -1, -1, -1, 1,  -1, 1,  1,  1,  1,  1,  1,  1,  -1, -1, 1,  -1,
      -1, -1, -1, 1,  -1, -1, 1,  -1, 1,  -1, -1, 1,  1,  1,  1,  -1, 1,  1,
      -1, -1, 1,  1,  -1, 1,  -1, 1,  -1, 1,  1,  -1, 1,  -1, -1, -1, -1, -1,
  };
  // vertex normals for a cube
  static const float cubeVertexNormals[] = {
      1,  0,  0, 1,  0,  0, 1, 0,  0,  1, 0,  0,  -1, 0, 0,  -1, 0, 0,
      -1, 0,  0, -1, 0,  0, 0, 1,  0,  0, 1,  0,  0,  1, 0,  0,  1, 0,
      0,  -1, 0, 0,  -1, 0, 0, -1, 0,  0, -1, 0,  0,  0, 1,  0,  0, 1,
      0,  0,  1, 0,  0,  1, 0, 0,  -1, 0, 0,  -1, 0,  0, -1, 0,  0, -1,
  };
  // vertex texture coordinates for a cube
  static const float cubeVertexTexcoords[] = {
      1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1,
      1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1,
  };
  // vertex indices for the triangles of a cube
  // the data above defines 24 vertices. We need to draw 12
  // triangles, 2 for each size, each triangle needs
  // 3 vertices so 12 * 3 = 36
  static const uint16_t cubeVertexIndices[] = {
      0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
      12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
  };

  const auto positionBuffer = glCreateBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertexPositions),
               cubeVertexPositions, GL_STATIC_DRAW);

  const auto normalBuffer = glCreateBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertexNormals), cubeVertexNormals,
               GL_STATIC_DRAW);

  const auto texcoordBuffer = glCreateBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertexTexcoords),
               cubeVertexTexcoords, GL_STATIC_DRAW);

  const auto indexBuffer = glCreateBuffer();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeVertexTexcoords),
               cubeVertexIndices, GL_STATIC_DRAW);

  static uint8_t checkerTextureData[] = {
      // data
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

  static uint8_t decalTextureData[32 * 32 * 4] = {0};
  const auto decalTexture = glCreateTexture();
  glBindTexture(GL_TEXTURE_2D, decalTexture);
  glTexImage2D(GL_TEXTURE_2D,
               0,                // mip level
               GL_RGBA,          // internal format
               32,               // width
               32,               // height
               0,                // border
               GL_RGBA,          // format
               GL_UNSIGNED_BYTE, // type
               decalTextureData);
  glGenerateMipmap(GL_TEXTURE_2D);

  // above this line is initialization code
  // --------------------------------------
  // below is rendering code.

  glViewport(0, 0, 300, 150);

  glClearColor(0.5, 0.7, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
  glEnableVertexAttribArray(positionLoc);
  glVertexAttribPointer(positionLoc, // location
                        3,           // size (components per iteration)
                        GL_FLOAT,    // type of to get from buffer
                        false,       // normalize
                        0,           // stride (bytes to advance each iteration)
                        0            // offset (bytes from start of buffer)
  );

  glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
  glEnableVertexAttribArray(normalLoc);
  glVertexAttribPointer(normalLoc, // location
                        3,         // size (components per iteration)
                        GL_FLOAT,  // type of to get from buffer
                        false,     // normalize
                        0,         // stride (bytes to advance each iteration)
                        0          // offset (bytes from start of buffer)
  );

  glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
  glEnableVertexAttribArray(texcoordLoc);
  glVertexAttribPointer(texcoordLoc, // location
                        2,           // size (components per iteration)
                        GL_FLOAT,    // type of to get from buffer
                        false,       // normalize
                        0,           // stride (bytes to advance each iteration)
                        0            // offset (bytes from start of buffer)
  );

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

  glUseProgram(program);

  // Picking unit 6 just to be different. The default of 0
  // would render but would show less state changing.
  int texUnit = 6;
  glActiveTexture(GL_TEXTURE0 + texUnit);
  glBindTexture(GL_TEXTURE_2D, checkerTexture);
  glUniform1i(diffuseLoc, texUnit);

  texUnit = 3;
  glActiveTexture(GL_TEXTURE0 + texUnit);
  glBindTexture(GL_TEXTURE_2D, decalTexture);
  glUniform1i(decalLoc, texUnit);

  static const float lightDirLocData[3] = {};
  glUniform3fv(lightDirLoc, 3, lightDirLocData);

  // const projection =
  //     m4.perspective(60 * Math.PI / 180,                             // fov
  //                    gl.canvas.clientWidth / gl.canvas.clientHeight, //
  //                    aspect 0.1, // near 10, // far
  //     );
  static mat4 projection;
  glUniformMatrix4fv(projectionLoc, 1, false, &projection);

  // draw center cube

  mat4 modelView;
  // modelView = m4.translate(modelView, 0, 0, -4);
  // modelView = m4.xRotate(modelView, 0.5);
  // modelView = m4.yRotate(modelView, 0.5);

  glUniformMatrix4fv(modelViewLoc, 1, false, &modelView);
  vec4 diffuseMult{0.7, 1, 0.7, 1};
  glUniform4fv(diffuseMultLoc, 4, &diffuseMult);

  glDrawElements(GL_TRIANGLES,
                 36,                // num vertices to process
                 GL_UNSIGNED_SHORT, // type of indices
                 0                  // offset on bytes to indices
  );

  // // draw left cube

  // modelView = m4.identity();
  // modelView = m4.translate(modelView, -3, 0, -4);
  // modelView = m4.xRotate(modelView, 0.5);
  // modelView = m4.yRotate(modelView, 0.8);

  // glUniformMatrix4fv(modelViewLoc, false, modelView);

  // glUniform4fv(diffuseMultLoc, [ 1, 0.7, 0.7, 1 ]);

  // glDrawElements(GL_TRIANGLES,
  //                36,                // num vertices to process
  //                GL_UNSIGNED_SHORT, // type of indices
  //                0,                 // offset on bytes to indices
  // );

  // // draw right cube

  // modelView = m4.identity();
  // modelView = m4.translate(modelView, 3, 0, -4);
  // modelView = m4.xRotate(modelView, 0.6);
  // modelView = m4.yRotate(modelView, -0.6);

  // glUniformMatrix4fv(modelViewLoc, false, modelView);

  // glUniform4fv(diffuseMultLoc, [ 0.7, 0.7, 1, 1 ]);

  // glDrawElements(GL_TRIANGLES,
  //                36,                // num vertices to process
  //                GL_UNSIGNED_SHORT, // type of indices
  //                0,                 // offset on bytes to indices
  // );
}