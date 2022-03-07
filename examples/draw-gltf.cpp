#include "CppGL/api.h"
#include "CppGL/constant.h"
#include "utils.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <unordered_map>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include <tiny_gltf.h>

using namespace CppGL;
using namespace rttr;

const int ARRAY_BUFFER = 34962;
const int ELEMENT_ARRAY_BUFFER = 34963;
const int UNSIGNED_SHORT = 5123;
const int UNSIGNED_BYTE = 5121;
const int FLOAT = 5126;

#include "CppGL/marco.h" // 必须在所有include后面

static struct VertexShaderSource : ShaderSource {
  attribute vec4 position;
  attribute vec3 normal;
  attribute vec2 texcoord;

  uniform mat4 projection;
  uniform mat4 modelView;
  uniform mat4 modelToWorld;

  varying vec3 v_normal;
  varying vec2 v_texcoord;
  varying vec3 v_position; // world space

  void main() {
    gl_Position = projection * modelView * position;
    v_position = vec3(modelToWorld * position);
    v_normal = mat3(modelView) * normal;
    v_texcoord = texcoord;

    // std::cout << texcoord << std::endl;
    // std::cout << gl_Position << std::endl;
  }

  RTTR_ENABLE(ShaderSource)
} vertexShaderSource;

static struct FragmentShaderSource : ShaderSource {
  varying vec3 v_normal;
  varying vec2 v_texcoord;
  varying vec3 v_position; // world space

  uniform sampler2D diffuse;

  uniform vec3 ambientLightColor;
  uniform float ambientLightIntensity;

  uniform vec3 directionalLightColor;
  uniform float directionalLightIntensity;
  uniform vec3 directionalLightDirection;

  uniform vec3 pointLightColor;
  uniform float pointLightIntensity;
  uniform vec3 pointLightPosition;

  uniform vec3 cameraPosition; // world space

  void main() {
    vec3 normal = normalize(v_normal);
    vec4 baseColor = texture2D(diffuse, v_texcoord);
    vec3 ambientColor = ambientLightColor * ambientLightIntensity;
    vec3 directionalDiffuseColor =
        directionalLightColor * max(dot(normal, directionalLightDirection), 0) *
        directionalLightIntensity;
    vec3 pointLightDirection = pointLightPosition - v_position;
    vec3 pointDiffuseColor = pointLightColor *
                             max(dot(normal, pointLightDirection), 0) *
                             pointLightIntensity;

    // 还缺高光, 视角和出射角度接近时表现为高光, 只有平行光和点光源会出现高光
    // 但是计算时候是使视角与入射光的半角与平面的法线对比,
    // 然后接近程度使用一个指数来区分
    vec3 viewDirection = normalize(cameraPosition - v_position);
    vec3 directionalHalfAngle = normalize(directionalLightDirection + viewDirection);
    vec3 pointHalfAngle = normalize(pointLightDirection + viewDirection);

    float directionalSpecular =
        pow(max(dot(viewDirection, directionalHalfAngle), 0), 128);
    float pointSpecular = pow(max(dot(viewDirection, pointHalfAngle), 0), 128);
    vec3 directionalSpecularColor = directionalLightColor * directionalSpecular * directionalLightIntensity;
    vec3 pointSpecularColor = pointLightColor * pointSpecular * pointLightIntensity;

    vec3 light = pointSpecularColor + directionalSpecularColor +
                 pointDiffuseColor + directionalDiffuseColor + ambientColor;
    // vec3 light = pointDiffuseColor + directionalDiffuseColor + ambientColor;
    gl_FragColor = vec4(vec3(baseColor) * light, baseColor.a);
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
      .CPPGL_RTTR_PROP(modelToWorld, M::Uniform)
      .CPPGL_RTTR_PROP(v_normal, M::Varying)
      .CPPGL_RTTR_PROP(v_texcoord, M::Varying)
      .CPPGL_RTTR_PROP(v_position, M::Varying)
      .method("main", &S::main);
}

CPPGL_RTTR_REGISTRATION {
  using S = FragmentShaderSource;
  using M = ShaderSourceMeta;
  registration::class_<S>("FragmentShaderSource")
      .CPPGL_RTTR_PROP(v_normal, M::Varying)
      .CPPGL_RTTR_PROP(v_texcoord, M::Varying)
      .CPPGL_RTTR_PROP(v_position, M::Varying)
      .CPPGL_RTTR_PROP(diffuse, M::Uniform)
      .CPPGL_RTTR_PROP(ambientLightColor, M::Uniform)
      .CPPGL_RTTR_PROP(ambientLightIntensity, M::Uniform)
      .CPPGL_RTTR_PROP(directionalLightColor, M::Uniform)
      .CPPGL_RTTR_PROP(directionalLightDirection, M::Uniform)
      .CPPGL_RTTR_PROP(directionalLightIntensity, M::Uniform)
      .CPPGL_RTTR_PROP(pointLightColor, M::Uniform)
      .CPPGL_RTTR_PROP(pointLightIntensity, M::Uniform)
      .CPPGL_RTTR_PROP(pointLightPosition, M::Uniform)
      .CPPGL_RTTR_PROP(cameraPosition, M::Uniform)
      .method("main", &S::main);
}

bool loadModel(tinygltf::Model &model, const char *filename);
void dbgModel(tinygltf::Model &model);
Program *initGLProgram();

// 0: cube 1:bloomBox
#define DRAW_OBJECT 0

int main(int argc, char **argv) {
#if DRAW_OBJECT == 0
  std::string filename = "../models/Cube/Cube.gltf";
#else
  std::string filename = "../models/BoomBox/glTF/BoomBox.gltf";
#endif

  if (argc > 1)
    filename = argv[1];

  tinygltf::Model model;
  if (!loadModel(model, filename.c_str()))
    return 1;
  // dbgModel(model);

  vec3 ambientLightColor{1, 1, 1};
  float ambientLightIntensity = 0.3;
  vec3 directionalLightColor{1, 1, 1};
  float directionalLightIntensity = 0.3;
  vec3 directionalLightDirection{normalize(vec3{1, 5, 8})};
  vec3 pointLightColor{1, 1, 1};
  float pointLightIntensity = 0.3;
  vec3 pointLightPosition{0, 0, -1.5}; // world space
  vec3 cameraPosition{0, 0, 0};        // world space

  Program *glProgram = initGLProgram();
  std::unordered_map<int, Buffer *> glBufferCache;
  std::unordered_map<int, Texture *> glTextureCache;
  int dpr = 1;
  glUseProgram(glProgram);
  glViewport(0, 0, 300 * dpr, 150 * dpr);
  glEnable(GL_DEPTH_TEST);

  auto projectionLoc = glGetUniformLocation(glProgram, "projection");
  auto modelViewLoc = glGetUniformLocation(glProgram, "modelView");
  auto modelToWorldLoc = glGetUniformLocation(glProgram, "modelToWorld");
  auto diffuseLoc = glGetUniformLocation(glProgram, "diffuse");
  auto cameraPositionLoc = glGetUniformLocation(glProgram, "diffuse");
  auto ambientLightColorLoc =
      glGetUniformLocation(glProgram, "ambientLightColor");
  auto ambientLightIntensityLoc =
      glGetUniformLocation(glProgram, "ambientLightIntensity");
  auto directionalLightColorLoc =
      glGetUniformLocation(glProgram, "directionalLightColor");
  auto directionalLightDirectionLoc =
      glGetUniformLocation(glProgram, "directionalLightDirection");
  auto directionalLightIntensityLoc =
      glGetUniformLocation(glProgram, "directionalLightIntensity");
  auto pointLightColorLoc = glGetUniformLocation(glProgram, "pointLightColor");
  auto pointLightIntensityLoc =
      glGetUniformLocation(glProgram, "pointLightIntensity");
  auto pointLightPositionLoc =
      glGetUniformLocation(glProgram, "pointLightPosition");

  glUniform3fv(ambientLightColorLoc, 3, &ambientLightColor);
  glUniform3fv(directionalLightColorLoc, 3, &directionalLightColor);
  glUniform3fv(pointLightColorLoc, 3, &pointLightColor);
  glUniform3fv(directionalLightDirectionLoc, 3, &directionalLightDirection);
  glUniform3fv(pointLightPositionLoc, 3, &pointLightPosition);
  glUniform3fv(cameraPositionLoc, 3, &cameraPosition);
  glUniform1f(ambientLightIntensityLoc, ambientLightIntensity);
  glUniform1f(directionalLightIntensityLoc, directionalLightIntensity);
  glUniform1f(pointLightIntensityLoc, pointLightIntensity);

  mat4 projection = mat4::perspective(60 * M_PI / 180,  // fov
                                      (float)300 / 150, // aspect
                                      1,                // near
                                      10                // far
  );
  glUniformMatrix4fv(projectionLoc, 1, false, &projection);

  mat4 cameraWorldMatrixInvert;
  mat4 modelWorldMatrix;
#if DRAW_OBJECT == 0
  modelWorldMatrix.translate(0, 0, -4).xRotate(0.5).yRotate(0.5);
#else
  modelWorldMatrix.translate(0, 0, -4).yRotate(M_PI_4 + M_PI_2).scale(160);
#endif
  // TODO 计算模型合适的scale

  auto uploadTexture = [&](int textureIndex) -> Texture * {
    auto &texture = model.textures[textureIndex];
    auto &image = model.images[texture.source];
    // auto &sampler = model.samplers[texture.sampler];
    auto search = glTextureCache.find(textureIndex);

    if (search != glTextureCache.end()) {
      glBindTexture(GL_TEXTURE_2D, search->second);
      return search->second;
    }

    auto glTexture = glCreateTexture();
    glTextureCache.emplace(textureIndex, glTexture);

    int dataType = GL_UNSIGNED_BYTE;
    // if (image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
    //   dataType = GL_UNSIGNED_BYTE;
    // if (image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
    //   dataType = GL_UNSIGNED_SHORT;

    glBindTexture(GL_TEXTURE_2D, glTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
                 GL_RGBA, dataType, &image.image.at(0));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return glTexture;
  };

  auto uploadBuffer = [&](int bufferViewIndex) {
    auto &bufferView = model.bufferViews[bufferViewIndex];
    auto &buffer = model.buffers[bufferView.buffer];

    auto search = glBufferCache.find(bufferViewIndex);
    Buffer *glBuffer = nullptr;

    int bufferTarget = GL_ARRAY_BUFFER;
    if (bufferView.target == ELEMENT_ARRAY_BUFFER)
      bufferTarget = GL_ELEMENT_ARRAY_BUFFER;

    if (search != glBufferCache.end()) {
      glBindBuffer(bufferTarget, search->second);
      return search->second;
    }

    glBuffer = glCreateBuffer();
    glBufferCache.emplace(bufferViewIndex, glBuffer);

    glBindBuffer(bufferTarget, glBuffer);
    glBufferData(bufferTarget, bufferView.byteLength,
                 &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);

    return glBuffer;
  };

  auto uploadAttribute = [&](int accessorIndex, int attributeLocation) {
    auto &accessor = model.accessors[accessorIndex];
    auto &bufferView = model.bufferViews[accessor.bufferView];
    auto &buffer = model.buffers[bufferView.buffer];
    int byteStride = accessor.ByteStride(bufferView);
    auto byteOffset = accessor.byteOffset;

    uploadBuffer(accessor.bufferView);

    int size = 1;
    if (accessor.type != TINYGLTF_TYPE_SCALAR)
      size = accessor.type;

    int componentType = 0;
    if (accessor.componentType == FLOAT)
      componentType = GL_FLOAT;
    if (accessor.componentType == UNSIGNED_BYTE)
      componentType = GL_UNSIGNED_BYTE;
    if (accessor.componentType == UNSIGNED_SHORT)
      componentType = GL_UNSIGNED_SHORT;

    glEnableVertexAttribArray(attributeLocation);
    glVertexAttribPointer(attributeLocation, size, componentType,
                          accessor.normalized, byteStride, accessor.byteOffset);
  };

  auto renderMesh = [&](int meshIndex, mat4 meshWorldMatrix) {
    auto &mesh = model.meshes[meshIndex];

    mat4 modelView = cameraWorldMatrixInvert * meshWorldMatrix;
    glUniformMatrix4fv(modelViewLoc, 1, false, &modelView);
    glUniformMatrix4fv(modelToWorldLoc, 1, false, &meshWorldMatrix);

    for (auto &primitive : mesh.primitives) {
      auto &indciesAccessor = model.accessors[primitive.indices];
      uploadBuffer(indciesAccessor.bufferView);

      uploadAttribute(primitive.attributes["POSITION"],
                      glGetAttribLocation(glProgram, "position"));
      uploadAttribute(primitive.attributes["TEXCOORD_0"],
                      glGetAttribLocation(glProgram, "texcoord"));
      uploadAttribute(primitive.attributes["NORMAL"],
                      glGetAttribLocation(glProgram, "normal"));

      auto &material = model.materials[primitive.material];

      auto baseColorTexture =
          uploadTexture(material.pbrMetallicRoughness.baseColorTexture.index);

      glActiveTexture(GL_TEXTURE0 + 0);
      glBindTexture(GL_TEXTURE_2D, baseColorTexture);
      glUniform1i(diffuseLoc, 0);

      int componentType = 0;
      if (indciesAccessor.componentType == FLOAT)
        componentType = GL_FLOAT;
      if (indciesAccessor.componentType == UNSIGNED_BYTE)
        componentType = GL_UNSIGNED_BYTE;
      if (indciesAccessor.componentType == UNSIGNED_SHORT)
        componentType = GL_UNSIGNED_SHORT;

      int mode = GL_TRIANGLES;
      glDrawElements(mode, indciesAccessor.count, componentType, 0);
      // glDrawElements(mode, 3, componentType, 0);
    }
  };

  auto renderNode = [&](auto &&self, int nodeIndex, mat4 parentMatrix) -> void {
    auto &node = model.nodes[nodeIndex];
    mat4 worldMatrix;
    mat4 localMatrix;

    localMatrix.from(node.matrix);
    localMatrix.from(node.translation, node.rotation, node.scale);
    worldMatrix = localMatrix * parentMatrix; // TODO 理解 & 区分左乘右乘

    if (node.mesh >= 0)
      renderMesh(node.mesh, worldMatrix);

    for (auto childNodeIndex : node.children)
      self(self, childNodeIndex, worldMatrix);
  };

  auto renderScene = [&](int sceneIndex, mat4 parentMatrix) {
    auto &scene = model.scenes[sceneIndex];
    for (auto nodeIndex : scene.nodes) {
      renderNode(renderNode, nodeIndex, parentMatrix);
    }
  };

  // renderScene(model.defaultScene, modelWorldMatrix);
  // // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // // renderScene(model.defaultScene, modelWorldMatrix);
  // displayBuffers();

  renderLoop([&]() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // modelWorldMatrix.xRotate(0.05).yRotate(0.1);
    modelWorldMatrix.yRotate(0.1);
    renderScene(model.defaultScene, modelWorldMatrix);

    // 显示渲染结果
    displayBuffers(false);
  });
}

bool loadModel(tinygltf::Model &model, const char *filename) {
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
  if (!warn.empty())
    std::cout << "WARN: " << warn << std::endl;

  if (!err.empty())
    std::cout << "ERR: " << err << std::endl;

  if (!res)
    std::cout << "Failed to load glTF: " << filename << std::endl;
  else
    std::cout << "Loaded glTF: " << filename << std::endl;

  return res;
}

void dbgModel(tinygltf::Model &model) {
  for (auto &mesh : model.meshes) {
    std::cout << "mesh : " << mesh.name << std::endl;
    for (auto &primitive : mesh.primitives) {
      const tinygltf::Accessor &indexAccessor =
          model.accessors[primitive.indices];

      std::cout << "indexaccessor: count " << indexAccessor.count << ", type "
                << indexAccessor.componentType << std::endl;

      tinygltf::Material &mat = model.materials[primitive.material];
      for (auto &mats : mat.values) {
        std::cout << "mat : " << mats.first.c_str() << std::endl;
      }

      for (auto &image : model.images) {
        std::cout << "image name : " << image.uri << std::endl;
        std::cout << "  size : " << image.image.size() << std::endl;
        std::cout << "  w/h : " << image.width << "/" << image.height
                  << std::endl;
      }

      std::cout << "indices : " << primitive.indices << std::endl;
      std::cout << "mode     : "
                << "(" << primitive.mode << ")" << std::endl;

      for (auto &attrib : primitive.attributes) {
        std::cout << "attribute : " << attrib.first.c_str() << std::endl;
      }
    }
  }
}

Program *initGLProgram() {
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
  return program;
};