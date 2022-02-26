#include "utils.h"
#include <CppGL/api.h>
#include <iostream>

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
  uniform vec3 lightDir;

  void main() {
    vec3 normal = normalize(v_normal);
    float light = dot(normal, lightDir) * 0.5 + 0.5;
    vec4 color = texture2D(diffuse, v_texcoord);
    // gl_FragColor = vec4(color.rgb * light, color.a);
    gl_FragColor = vec4(vec3(color.r, color.g, color.b) * light, color.a);
  }

  RTTR_ENABLE(ShaderSource)
} fragmentShaderSource;

CPPGL_RTTR_REGISTRATION {
  using S = VertexShaderSource;
  using M = ShaderSourceMeta;
  registration::class_<S>("VertexShaderSource")
      .CPPGL_RTTR_PROP(position, M::Attribute)
      .CPPGL_RTTR_PROP(normal, M::Attribute)
      .CPPGL_RTTR_PROP(texcoord, M::Attribute)
      .CPPGL_RTTR_PROP(projection, M::Uniform)
      .CPPGL_RTTR_PROP(modelView, M::Uniform)
      .CPPGL_RTTR_PROP(v_normal, M::Varying)
      .CPPGL_RTTR_PROP(v_texcoord, M::Varying)
      .method("main", &S::main);
}

CPPGL_RTTR_REGISTRATION {
  using S = FragmentShaderSource;
  using M = ShaderSourceMeta;
  registration::class_<S>("FragmentShaderSource")
      .CPPGL_RTTR_PROP(v_normal, M::Varying)
      .CPPGL_RTTR_PROP(v_texcoord, M::Varying)
      .CPPGL_RTTR_PROP(diffuse, M::Uniform)
      .CPPGL_RTTR_PROP(lightDir, M::Uniform)
      .method("main", &S::main);
}

int main(void) {
  auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, &vertexShaderSource);
  glCompileShader(vertexShader);

  auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, &fragmentShaderSource);
  glCompileShader(fragmentShader);

  auto prg = glCreateProgram();
  glAttachShader(prg, vertexShader);
  glAttachShader(prg, fragmentShader);
  glLinkProgram(prg);

  auto positionLoc = glGetAttribLocation(prg, "position");
  auto normalLoc = glGetAttribLocation(prg, "normal");
  auto texcoordLoc = glGetAttribLocation(prg, "texcoord");

  auto projectionLoc = glGetUniformLocation(prg, "projection");
  auto modelViewLoc = glGetUniformLocation(prg, "modelView");
  auto diffuseLoc = glGetUniformLocation(prg, "diffuse");
  auto lightDirLoc = glGetUniformLocation(prg, "lightDir");

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

  auto positionBuffer = glCreateBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertexPositions),
               cubeVertexPositions, GL_STATIC_DRAW);

  auto normalBuffer = glCreateBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertexNormals), cubeVertexNormals,
               GL_STATIC_DRAW);

  auto texcoordBuffer = glCreateBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertexTexcoords),
               cubeVertexTexcoords, GL_STATIC_DRAW);

  auto indexBuffer = glCreateBuffer();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeVertexIndices),
               cubeVertexIndices, GL_STATIC_DRAW);

  static const uint8_t checkerTextureData[] = {
      192, 128, 192, 128, 128, 192, 128, 192,
      192, 128, 192, 128, 128, 192, 128, 192,
  };
  auto checkerTexture = glCreateTexture();
  glBindTexture(GL_TEXTURE_2D, checkerTexture);
  glTexImage2D(GL_TEXTURE_2D,
               0,                   // mip level
               GL_LUMINANCE,        // internal format
               4,                   // width
               4,                   // height
               0,                   // border
               GL_LUMINANCE,        // format
               GL_UNSIGNED_BYTE,    // type
               checkerTextureData); // data
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  const int fbTextureWidth = 128;
  const int fbTextureHeight = 128;
  const auto fbTexture = glCreateTexture();
  glBindTexture(GL_TEXTURE_2D, fbTexture);
  glTexImage2D(GL_TEXTURE_2D,
               0,                // mip level
               GL_RGBA,          // internal format
               fbTextureWidth,   // width
               fbTextureHeight,  // height
               0,                // border
               GL_RGBA,          // format
               GL_UNSIGNED_BYTE, // type
               nullptr           // data
  );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  auto depthRB = glCreateRenderbuffer();
  glBindRenderbuffer(GL_RENDERBUFFER, depthRB);
  glRenderbufferStorage(GL_RENDERBUFFER,
                        GL_DEPTH_COMPONENT32F, // format
                        fbTextureWidth,        // width,
                        fbTextureHeight        // height,
  );

  auto fb = glCreateFramebuffer();
  glBindFramebuffer(GL_FRAMEBUFFER, fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         fbTexture, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depthRB);

  // above this line is initialization code
  // --------------------------------------
  // below is rendering code.

  // --------------------------------------
  // First draw a cube to the texture attached to the framebuffer

  glBindFramebuffer(GL_FRAMEBUFFER, fb);
  glViewport(0, 0, fbTextureWidth, fbTextureHeight);

  glClearColor(1, 0, 0, 1);
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

  glUseProgram(prg);

  // Picking unit 6 just to be different. The default of 0
  // would render but would show less state changing.
  int texUnit = 6;
  glActiveTexture(GL_TEXTURE0 + texUnit);
  glBindTexture(GL_TEXTURE_2D, checkerTexture);
  glUniform1i(diffuseLoc, texUnit);

  vec3 lightDirLocData = normalize(vec3{1, 5, 8});
  glUniform3fv(lightDirLoc, 3, &lightDirLocData);

  // We need a perspective matrix that matches the
  // aspect of the framebuffer texture
  static mat4 projection =
      mat4::perspective(60 * M_PI / 180,                         // fov
                        (float)fbTextureWidth / fbTextureHeight, // aspect
                        1,                                       // near
                        10                                       // far
      );
  glUniformMatrix4fv(projectionLoc, 1, false, &projection);

  mat4 modelView;
  modelView.translate(0, 0, -4).xRotate(0.5).yRotate(0.5);
  glUniformMatrix4fv(modelViewLoc, 1, false, &modelView);

  glDrawElements(GL_TRIANGLES,
                 36,                // num vertices to process
                 GL_UNSIGNED_SHORT, // type of indices
                 0                  // offset on bytes to indices
  );

  // --------------------------------------
  // Now draw a cube to the canvas using
  // the texture attached to the framebuffer

  glBindFramebuffer(GL_FRAMEBUFFER, nullptr);
  glViewport(0, 0, 300, 150);

  glClearColor(0, 0, 1, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // We need a perspective matrix that matches the
  // aspect of the canvas
  projection = mat4::perspective(60 * M_PI / 180,  // fov
                                 (float)300 / 150, // aspect
                                 1,                // near
                                 10                // far
  );
  glUniformMatrix4fv(projectionLoc, 1, false, &projection);

  // Picking unit 3 just to be different. We could have
  // stated with the same as above.
  texUnit = 3;
  glActiveTexture(GL_TEXTURE0 + texUnit);
  glBindTexture(GL_TEXTURE_2D, fbTexture);
  glUniform1i(diffuseLoc, texUnit);

  // We didn't have to change attributes or other
  // uniforms as we're just drawing the same cube vertices
  // at the same place in front of the camera.

  glDrawElements(GL_TRIANGLES,
                 36,                // num vertices to process
                 GL_UNSIGNED_SHORT, // type of indices
                 0                  // offset on bytes to indices
  );

  displayBuffers();
}