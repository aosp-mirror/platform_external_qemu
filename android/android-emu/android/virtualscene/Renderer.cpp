/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/virtualscene/Renderer.h"

#include "android/base/ArraySize.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"
#include "android/virtualscene/RenderTarget.h"
#include "android/virtualscene/SceneObject.h"
#include "android/virtualscene/TextureUtils.h"

#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace android::base;

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

static constexpr int kSuperSampleMultiple = 2;

static constexpr char kTexturedVertexShader[] = R"(
attribute vec3 in_position;
attribute vec2 in_uv;

uniform mat4 u_modelViewProj;

varying vec2 uv;

void main() {
    uv = in_uv;
    gl_Position = u_modelViewProj * vec4(in_position, 1.0);
}
)";

static constexpr char kTexturedFragmentShader[] = R"(
precision mediump float;
varying vec2 uv;

uniform sampler2D tex_sampler;

void main() {
  gl_FragColor = texture2D(tex_sampler, uv);
}
)";

static constexpr char kScreenSpaceVertexShader[] = R"(
attribute vec3 in_position;
attribute vec2 in_uv;

varying vec2 uv;

void main() {
    uv = in_uv;
    gl_Position = vec4(in_position, 1.0);
}
)";

static constexpr char kBlitFragmentShader[] = R"(
precision mediump float;
uniform vec2 resolution;

uniform sampler2D tex_sampler;

void main() {
  gl_FragColor = texture2D(tex_sampler, gl_FragCoord.xy * resolution);
}
)";


static constexpr char kCheckerboardFragmentShader[] = R"(
precision mediump float;
varying vec2 uv;
uniform float u_time;

#define CHECKERBOARD_SQUARE_WIDTH 16.0 // Number of squares across the board.
#define MOVING_SQUARE_SIZE 2.5 // Width of moving square in checkerboard squares.
#define VISIBLE_HEIGHT 9.0 // Height where we want the moving square to bounce.
                           // in checkerboard squares.
#define COLOR_BLACK vec3(0.0,0.0,0.0)
#define COLOR_GREY vec3(0.5,0.5,0.5)
#define COLOR_RED vec3(0.82422,0.03125,0.0)
#define COLOR_GREEN vec3(0.01172,0.65625,0.0)
#define MOVING_SQUARE_VELOCITY vec2(1.5,1.5) // Velocity of the moving
                                             // square expressed in
                                             // checkerboard squares
                                             // per second.
#define CHECKERBOARD_VELOCITY vec2(1.0,1.0/3.0) // Velocity of the checkerboard
                                                // expressed in checkerboard
                                                // squares per second.
#define COLOR_CHANGE_TIME 5.0 // Time between red/green color changes.

void main() {
  vec2 staticBoardSpacePosition =
        vec2(uv.x * CHECKERBOARD_SQUARE_WIDTH, uv.y * CHECKERBOARD_SQUARE_WIDTH);
  vec2 movingBoardSpacePosition =
        staticBoardSpacePosition +
        u_time * CHECKERBOARD_VELOCITY;
  vec3 checkerboardColor = COLOR_BLACK + (mod(
        floor(mod(movingBoardSpacePosition.x, 2.0)) +
        floor(mod(movingBoardSpacePosition.y, 2.0)), 2.0)) *
        (COLOR_GREY - COLOR_BLACK);
  vec2 totalSquareMovement = MOVING_SQUARE_VELOCITY * u_time;
  float squareXTravel = CHECKERBOARD_SQUARE_WIDTH - MOVING_SQUARE_SIZE;
  float squareYTravel = VISIBLE_HEIGHT - MOVING_SQUARE_SIZE;
  vec2 squarePosition = vec2(
        mod(totalSquareMovement.x, squareXTravel * 2.0) > squareXTravel ?
        squareXTravel - mod(totalSquareMovement.x, squareXTravel) :
        mod(totalSquareMovement.x, squareXTravel),
        ((CHECKERBOARD_SQUARE_WIDTH - VISIBLE_HEIGHT) / 2.0) +
        (mod(totalSquareMovement.y, squareYTravel * 2.0) > squareYTravel ?
        squareYTravel - mod(totalSquareMovement.y, squareYTravel) :
        mod(totalSquareMovement.y, squareYTravel)));
  vec3 squareColor = COLOR_GREEN +
        floor(mod(u_time, COLOR_CHANGE_TIME * 2.0) / COLOR_CHANGE_TIME) *
        (COLOR_RED - COLOR_GREEN);
  vec2 posFromSquareOrigin = staticBoardSpacePosition - squarePosition;

  if (posFromSquareOrigin.x >= 0.0 &&
      posFromSquareOrigin.y >= 0.0 &&
      posFromSquareOrigin.x < MOVING_SQUARE_SIZE &&
      posFromSquareOrigin.y < MOVING_SQUARE_SIZE) {
    gl_FragColor = vec4(squareColor, 1.0);
  } else {
    gl_FragColor = vec4(checkerboardColor, 1.0);
  }
}
)";

static constexpr char kFxaaFragmentShader[] = R"(
precision mediump float;
uniform sampler2D tex_sampler;
uniform vec2 resolution;

#define FXAA_MUL  (1.0/8.0)
#define FXAA_MIN  (1.0/32.0)
#define FXAA_SPAN 8.0

void main() {
  vec3 rgbNW = texture2D(tex_sampler, (gl_FragCoord.xy + vec2(-1.0, -1.0)) * resolution).xyz;
  vec3 rgbNE = texture2D(tex_sampler, (gl_FragCoord.xy + vec2( 1.0, -1.0)) * resolution).xyz;
  vec3 rgbM = texture2D(tex_sampler, gl_FragCoord.xy * resolution).xyz;
  vec3 rgbSW = texture2D(tex_sampler, (gl_FragCoord.xy + vec2(-1.0,  1.0)) * resolution).xyz;
  vec3 rgbSE = texture2D(tex_sampler, (gl_FragCoord.xy + vec2( 1.0,  1.0)) * resolution).xyz;

  vec3 luma = vec3(0.299, 0.587, 0.114);
  float lumaNW = dot(rgbNW, luma);
  float lumaNE = dot(rgbNE, luma);
  float lumaM = dot(rgbM, luma);
  float lumaSW = dot(rgbSW, luma);
  float lumaSE = dot(rgbSE, luma);

  vec2 dir = vec2(-((lumaNW + lumaNE) - (lumaSW + lumaSE)),
                  ((lumaNW + lumaSW) - (lumaNE + lumaSE)));

  float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_MUL), FXAA_MIN);

  float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
  float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

  float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

  dir = min(vec2( FXAA_SPAN, FXAA_SPAN), max(vec2(-FXAA_SPAN, -FXAA_SPAN), dir * rcpDirMin)) * resolution;

  vec4 rgbA = 0.5 * (texture2D(tex_sampler, gl_FragCoord.xy * resolution + dir * (1.0 / 3.0 - 0.5)) +
                     texture2D(tex_sampler, gl_FragCoord.xy * resolution + dir * (2.0 / 3.0 - 0.5)));
  vec4 rgbB = rgbA * 0.5 + 0.25 * (texture2D(tex_sampler, gl_FragCoord.xy * resolution + dir * (0.0 / 3.0 - 0.5)) +
                                   texture2D(tex_sampler, gl_FragCoord.xy * resolution + dir * (3.0 / 3.0 - 0.5)));
  float lumaB = dot(rgbB.xyz, luma);

  if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
      gl_FragColor = rgbA;
  } else {
      gl_FragColor = rgbB;
  }
}
)";

static constexpr android::virtualscene::VertexPositionUV kScreenQuadVerts[] = {
    { glm::vec3(-1.f, -1.f, 0.f), glm::vec2(0.f,  0.f) },
    { glm::vec3( 3.f, -1.f, 0.f), glm::vec2(2.f,  0.f) },
    { glm::vec3(-1.f,  3.f, 0.f), glm::vec2(0.f,  2.f) },
};

static constexpr GLuint kScreenQuadIndices[] = {
    0, 1, 2,
};

namespace android {
namespace virtualscene {

struct MaterialData {
    GLuint program = 0;

    GLint positionLocation = -1;
    GLint uvLocation = -1;
    GLint texSamplerLocation = -1;
    GLint mvpLocation = -1;
    GLint resolutionLocation = -1;
    GLint timeLocation = -1;
};

struct MeshData {
    VertexInfo mVertexInfo;
    GLuint mVertexBuffer = 0;
    GLuint mIndexBuffer = 0;
    size_t mIndexCount = 0;
};

struct SceneObjectData {
    std::vector<Material> materials;
    std::vector<Mesh> meshes;
};

using RenderableParameterCallback =
        std::function<void(const MaterialData& material)>;

/*******************************************************************************
 *                     Renderer routines
 ******************************************************************************/

class RendererImpl : public Renderer, private SceneObject {
    DISALLOW_COPY_AND_ASSIGN(RendererImpl);

public:
    RendererImpl(const GLESv2Dispatch* gles2, int width, int height);
    ~RendererImpl();

    bool initialize();

    // Renderer public API.
    void releaseObjectResources(const SceneObject* sceneObject) override;

    Material createMaterialCheckerboard(const SceneObject* parent) override;
    Material createMaterialTextured() override;
    Material createMaterialScreenSpace(const SceneObject* parent,
                                       const char* frag) override;

    Mesh createMesh(const SceneObject* parent,
                    const VertexPositionUV* vertices,
                    size_t verticesSize,
                    const GLuint* indices,
                    size_t indicesSize) override;

    Texture loadTexture(const SceneObject* parent,
                        const char* filename) override;

    void render(const std::vector<RenderableObject>& renderables,
                float time) override;

private:
    void releaseMaterial(Material material);
    void releaseMesh(Mesh mesh);
    void releaseTexture(Texture texture);

    // Helper to determine if GLES supports a given texture size.
    bool isTextureSizeValid(uint32_t width, uint32_t height);

    // Create an OpenGL texture with the given width and height with storage
    // allocated, but no data loaded.
    Texture createEmptyTexture(uint32_t width, uint32_t height);

    // Compile a shader from source.
    // |type| - GL shader type, such as GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
    // |shaderSource| - Shader source code.
    //
    // Returns a non-zero shader handle on success, or zero on failure.
    // The result must be released with glDeleteShader.
    GLuint compileShader(GLenum type, const char* shaderSource);

    // Link a vertex and fragment shader and return a program.
    // |vertexId| - GL vertex shader handle, must be non-zero.
    // |fragmentId| - GL fragment shader handle, must be non-zero.
    //
    // After this call, it is valid to call glDeleteShader; if the program was
    // successfully linked the program will keep them alive.
    //
    // Returns a non-zero program handle on success, or zero on failure.
    // The result must be released with glDeleteProgram.
    GLuint linkShaders(GLuint vertexId, GLuint fragmentId);

    GLint getAttribLocation(GLuint program, const char* name);
    GLint getUniformLocation(GLuint program, const char* name);

    // Executes the given renderable, used internally by Renderer::render.
    void processRenderable(const Renderable& renderable,
                           RenderableParameterCallback parameterCallback);

    const GLESv2Dispatch* const mGles2;

    const int mRenderWidth;
    const int mRenderHeight;

    std::unique_ptr<RenderTarget> mRenderTargets[2];
    std::unique_ptr<RenderTarget> mScreenRenderTarget;
    Mesh mEffectsMesh;
    std::vector<Material> mEffectsChain;

    int mNextResourceId = 0;
    std::unordered_map<int, MaterialData> mMaterials;
    std::unordered_map<int, MeshData> mMeshes;
    std::unordered_set<GLuint> mTextures;

    std::unordered_map<const SceneObject*, SceneObjectData> mObjectData;

    // Standard materials.
    Material mMaterialTextured;
};

std::unique_ptr<Renderer> Renderer::create(const GLESv2Dispatch* gles2,
                                           int width,
                                           int height) {
    std::unique_ptr<RendererImpl> renderer;
    renderer.reset(new RendererImpl(gles2, width, height));
    if (!renderer->initialize()) {
        return nullptr;
    }
    return std::move(renderer);
}

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

RendererImpl::RendererImpl(const GLESv2Dispatch* gles2, int width, int height)
    : mGles2(gles2), mRenderWidth(width), mRenderHeight(height) {}

bool RendererImpl::initialize() {
    mScreenRenderTarget =
            RenderTarget::createDefault(mGles2, mRenderWidth, mRenderHeight);
    if (!mScreenRenderTarget) {
        E("%s: Failed creating screen render target.", __FUNCTION__);
        return false;
    }

    mRenderTargets[0] = RenderTarget::createTextureTarget(
            mGles2,
            createEmptyTexture(mRenderWidth * kSuperSampleMultiple,
                               mRenderHeight * kSuperSampleMultiple),
            mRenderWidth * kSuperSampleMultiple,
            mRenderHeight * kSuperSampleMultiple);
    mRenderTargets[1] = RenderTarget::createTextureTarget(
            mGles2,
            createEmptyTexture(mRenderWidth * kSuperSampleMultiple,
                               mRenderHeight * kSuperSampleMultiple),
            mRenderWidth * kSuperSampleMultiple,
            mRenderHeight * kSuperSampleMultiple);
    if (!mRenderTargets[0] || !mRenderTargets[1]) {
        E("%s: Failed creating texture render targets.", __FUNCTION__);
        return false;
    }

    Material fxaaEffect = createMaterialScreenSpace(this, kFxaaFragmentShader);
    if (!fxaaEffect.isValid()) {
        E("%s: Failed creating fxaa effect.", __FUNCTION__);
        return false;
    }
    mEffectsChain.push_back(std::move(fxaaEffect));

    Material blitEffect = createMaterialScreenSpace(this, kBlitFragmentShader);
    if (!blitEffect.isValid()) {
        E("%s: Failed creating blit effect.", __FUNCTION__);
        return false;
    }
    mEffectsChain.push_back(std::move(blitEffect));

    mEffectsMesh =
            Renderer::createMesh(this, kScreenQuadVerts, kScreenQuadIndices);
    if (!mEffectsMesh.isValid()) {
        E("%s: Failed creating effects mesh.", __FUNCTION__);
        return false;
    }

    return true;
}

RendererImpl::~RendererImpl() {
    // Release Renderer-internal objects.
    releaseObjectResources(this);
    if (mMaterialTextured.isValid()) {
        releaseMaterial(mMaterialTextured);
    }

    for (const auto& objectDataIt : mObjectData) {
        W("%s: SceneObject 0x%08" PRIXPTR
          " was not unregistered, resources will be leaked.",
          __FUNCTION__, objectDataIt.first);
    }

    for (const auto& materialIt : mMaterials) {
        W("%s: Leaked material with id %d", __FUNCTION__, materialIt.first);
    }

    for (const auto& meshIt : mMeshes) {
        W("%s: Leaked mesh with id %d", __FUNCTION__, meshIt.first);
    }
}

void RendererImpl::releaseObjectResources(const SceneObject* sceneObject) {
    auto objectDataIt = mObjectData.find(sceneObject);

    if (objectDataIt == mObjectData.end()) {
        D("%s: Scene object 0x08%" PRIXPTR " has no resources to release.",
          __FUNCTION__, sceneObject);
        return;
    }

    SceneObjectData& data = objectDataIt->second;

    // Unregister object resources.
    for (auto& material : data.materials) {
        releaseMaterial(material);
    }

    for (auto& mesh : data.meshes) {
        releaseMesh(mesh);
    }

    mObjectData.erase(objectDataIt);
}

Material RendererImpl::createMaterialCheckerboard(const SceneObject* parent) {
    // Compile and setup shaders.
    const GLuint vertexId =
            compileShader(GL_VERTEX_SHADER, kTexturedVertexShader);
    const GLuint fragmentId =
            compileShader(GL_FRAGMENT_SHADER, kCheckerboardFragmentShader);

    GLuint program = 0;
    if (vertexId && fragmentId) {
        program = linkShaders(vertexId, fragmentId);
    }

    // Release the shaders, the program will keep their reference alive.
    mGles2->glDeleteShader(vertexId);
    mGles2->glDeleteShader(fragmentId);

    if (!program) {
        // Initialization failed, no program loaded.
        return Material();
    }

    MaterialData material;
    material.program = program;
    material.positionLocation = getAttribLocation(program, "in_position");
    material.uvLocation = getAttribLocation(program, "in_uv");
    material.mvpLocation = getUniformLocation(program, "u_modelViewProj");
    material.timeLocation = getUniformLocation(program, "u_time");

    const int id = mNextResourceId++;
    mMaterials[id] = material;

    Material materialHandle;
    materialHandle.id = id;
    mObjectData[parent].materials.push_back(materialHandle);
    return materialHandle;
}

Material RendererImpl::createMaterialTextured() {
    // Return cached instance if it exists.
    if (mMaterialTextured.isValid()) {
        return mMaterialTextured;
    }

    // Compile and setup shaders.
    const GLuint vertexId =
            compileShader(GL_VERTEX_SHADER, kTexturedVertexShader);
    const GLuint fragmentId =
            compileShader(GL_FRAGMENT_SHADER, kTexturedFragmentShader);

    GLuint program = 0;
    if (vertexId && fragmentId) {
        program = linkShaders(vertexId, fragmentId);
    }

    // Release the shaders, the program will keep their reference alive.
    mGles2->glDeleteShader(vertexId);
    mGles2->glDeleteShader(fragmentId);

    if (!program) {
        // Initialization failed, no program loaded.
        return Material();
    }

    MaterialData material;
    material.program = program;
    material.positionLocation = getAttribLocation(program, "in_position");
    material.uvLocation = getAttribLocation(program, "in_uv");
    material.texSamplerLocation = getUniformLocation(program, "tex_sampler");
    material.mvpLocation = getUniformLocation(program, "u_modelViewProj");

    const int id = mNextResourceId++;
    mMaterials[id] = material;

    mMaterialTextured.id = id;
    return mMaterialTextured;
}

Material RendererImpl::createMaterialScreenSpace(const SceneObject* parent,
                                                 const char* frag) {
    // Compile and setup shaders.
    const GLuint vertexId =
            compileShader(GL_VERTEX_SHADER, kScreenSpaceVertexShader);
    const GLuint fragmentId = compileShader(GL_FRAGMENT_SHADER, frag);

    GLuint program = 0;
    if (vertexId && fragmentId) {
        program = linkShaders(vertexId, fragmentId);
    }

    // Release the shaders, the program will keep their reference alive.
    mGles2->glDeleteShader(vertexId);
    mGles2->glDeleteShader(fragmentId);

    if (!program) {
        // Initialization failed, no program loaded.
        return Material();
    }

    MaterialData material;
    material.program = program;
    material.positionLocation = getAttribLocation(program, "in_position");
    material.uvLocation = getAttribLocation(program, "in_uv");
    material.texSamplerLocation = getUniformLocation(program, "tex_sampler");
    material.resolutionLocation = getUniformLocation(program, "resolution");

    const int id = mNextResourceId++;
    mMaterials[id] = material;

    Material materialHandle;
    materialHandle.id = id;
    mObjectData[parent].materials.push_back(materialHandle);
    return materialHandle;
}

Mesh RendererImpl::createMesh(const SceneObject* parent,
                              const VertexPositionUV* vertices,
                              size_t verticesSize,
                              const GLuint* indices,
                              size_t indicesSize) {
    GLuint vertexBuffer = 0;
    GLuint indexBuffer = 0;

    mGles2->glGenBuffers(1, &vertexBuffer);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    mGles2->glBufferData(GL_ARRAY_BUFFER,
                         verticesSize * sizeof(VertexPositionUV), vertices,
                         GL_STATIC_DRAW);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, 0);

    mGles2->glGenBuffers(1, &indexBuffer);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    mGles2->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize * sizeof(GLuint),
                         indices, GL_STATIC_DRAW);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    VertexInfo info;
    info.hasUV = true;
    info.vertexSize = sizeof(VertexPositionUV);
    info.posOffset = offsetof(VertexPositionUV, pos);
    info.uvOffset = offsetof(VertexPositionUV, uv);

    MeshData mesh;
    mesh.mVertexInfo = info;
    mesh.mVertexBuffer = vertexBuffer;
    mesh.mIndexBuffer = indexBuffer;
    mesh.mIndexCount = indicesSize;

    const int id = mNextResourceId++;
    mMeshes[id] = mesh;

    Mesh meshHandle;
    meshHandle.id = id;
    mObjectData[parent].meshes.push_back(meshHandle);
    return meshHandle;
}

Texture RendererImpl::loadTexture(const SceneObject* parent,
                                  const char* filename) {
    std::vector<uint8_t> buffer;
    uint32_t width = 0;
    uint32_t height = 0;
    if (!TextureUtils::loadPNG(filename, buffer, &width, &height)) {
        return Texture();
    }

    if (!isTextureSizeValid(width, height)) {
        E("%s: Invalid texture size, %d x %d, GL_MAX_TEXTURE_SIZE = %d", width,
          height, GL_MAX_TEXTURE_SIZE);
        return Texture();
    }

    GLuint textureId;
    mGles2->glGenTextures(1, &textureId);
    mGles2->glBindTexture(GL_TEXTURE_2D, textureId);
    mGles2->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, buffer.data());
    mGles2->glGenerateMipmap(GL_TEXTURE_2D);
    mGles2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    mGles2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR);
    mGles2->glBindTexture(GL_TEXTURE_2D, 0);

#ifdef DEBUG
    // Only check for GL error on debug builds to avoid a potential synchronous
    // flush.
    const GLenum error = mGles2->glGetError();
    if (error != GL_NO_ERROR) {
        E("%s: GL error %d", __FUNCTION__, error);
        return Texture();
    }
#endif

    const auto insertResult = mTextures.insert(textureId);
    if (!insertResult.second) {
        W("%s: Texture id %d already exists.", __FUNCTION__, textureId);
    }

    Texture texture;
    texture.id = static_cast<int>(textureId);
    return texture;
}

void RendererImpl::render(const std::vector<RenderableObject>& renderables,
                          float time) {
    mRenderTargets[0]->bind();

    mGles2->glEnable(GL_DEPTH_TEST);
    mGles2->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    mGles2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render scene objects.
    for (auto& renderObject : renderables) {
        processRenderable(renderObject.renderable,
                          [&](const MaterialData& material) {
                              mGles2->glUniformMatrix4fv(
                                      material.mvpLocation, 1, GL_FALSE,
                                      &renderObject.modelViewProj[0][0]);
                              mGles2->glUniform1f(material.timeLocation, time);
                          });
    }

    mGles2->glDisable(GL_DEPTH_TEST);

    assert(!mEffectsChain.empty());
    for (size_t i = 0; i < mEffectsChain.size(); i++) {
        int superSampleMultiple = kSuperSampleMultiple;
        if (i == mEffectsChain.size() - 1) {
            superSampleMultiple = 1;
            mScreenRenderTarget->bind();
        } else {
            mRenderTargets[(i + 1) % 2]->bind();
        }

        Renderable renderable;
        renderable.texture = mRenderTargets[i % 2]->getTexture();
        renderable.material = mEffectsChain[i];
        renderable.mesh = mEffectsMesh;

        processRenderable(renderable, [&](const MaterialData& material) {
            mGles2->glUniform2f(material.resolutionLocation,
                                1.f / (superSampleMultiple * mRenderWidth),
                                1.f / (superSampleMultiple * mRenderHeight));
            mGles2->glUniform1f(material.timeLocation, time);
        });
    }

    mGles2->glBindBuffer(GL_ARRAY_BUFFER, 0);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    mGles2->glBindTexture(GL_TEXTURE_2D, 0);
    mGles2->glUseProgram(0);
}

void RendererImpl::releaseMaterial(Material material) {
    const auto materialIt = mMaterials.find(material.id);

    if (materialIt == mMaterials.end()) {
        E("%s: Could not find material id %d", __FUNCTION__, material.id);
        return;
    }

    const MaterialData& materialData = materialIt->second;

    // Clean up outstanding OpenGL handles; releasing a 0 handle no-ops.
    mGles2->glDeleteProgram(materialData.program);

    mMaterials.erase(materialIt);
}

void RendererImpl::releaseMesh(Mesh mesh) {
    const auto meshIt = mMeshes.find(mesh.id);

    if (meshIt == mMeshes.end()) {
        E("%s: Could not find mesh id %d", __FUNCTION__, mesh.id);
        return;
    }

    const MeshData& meshData = meshIt->second;

    // Clean up outstanding OpenGL handles; releasing a 0 handle no-ops.
    mGles2->glDeleteBuffers(1, &meshData.mVertexBuffer);
    mGles2->glDeleteBuffers(1, &meshData.mIndexBuffer);

    mMeshes.erase(meshIt);
}

void RendererImpl::releaseTexture(Texture texture) {
    if (texture.id <= 0) {
        E("%s: Invalid texture id %d", __FUNCTION__, texture.id);
    }

    const GLuint textureGL = static_cast<GLuint>(texture.id);
    const auto textureIt = mTextures.find(textureGL);

    if (textureIt == mTextures.end()) {
        E("%s: Could not find texture id %d", __FUNCTION__, texture.id);
        return;
    }

    // Clean up outstanding OpenGL handles; releasing a 0 handle no-ops.
    mGles2->glDeleteTextures(1, &textureGL);

    mTextures.erase(textureIt);
}

bool RendererImpl::isTextureSizeValid(uint32_t width, uint32_t height) {
    GLint maxTextureSize = 0;
    mGles2->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    return width != 0 && height != 0 && width <= maxTextureSize &&
           height <= maxTextureSize;
}

Texture RendererImpl::createEmptyTexture(uint32_t width, uint32_t height) {
    if (!isTextureSizeValid(width, height)) {
        E("%s: Invalid texture size, %d x %d, GL_MAX_TEXTURE_SIZE = %d", width,
          height, GL_MAX_TEXTURE_SIZE);
        return Texture();
    }

    GLuint textureId;
    mGles2->glGenTextures(1, &textureId);
    mGles2->glBindTexture(GL_TEXTURE_2D, textureId);
    mGles2->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, 0);
    mGles2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    mGles2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    mGles2->glBindTexture(GL_TEXTURE_2D, 0);

#ifdef DEBUG
    // Only check for GL error on debug builds to avoid a potential synchronous
    // flush.
    const GLenum error = mGles2->glGetError();
    if (error != GL_NO_ERROR) {
        E("%s: GL error %d", __FUNCTION__, error);
        return Texture();
    }
#endif

    const auto insertResult = mTextures.insert(textureId);
    if (!insertResult.second) {
        W("%s: Texture id %d already exists.", __FUNCTION__, textureId);
    }

    Texture texture;
    texture.id = static_cast<int>(textureId);
    return texture;
}

GLuint RendererImpl::compileShader(GLenum type, const char* shaderSource) {
    const GLuint shaderId = mGles2->glCreateShader(type);

    mGles2->glShaderSource(shaderId, 1, &shaderSource, nullptr);
    mGles2->glCompileShader(shaderId);

    // Output the error message if the compile failed.
    GLint result = GL_FALSE;
    mGles2->glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
    if (result != GL_TRUE) {
        int logLength = 0;
        mGles2->glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength) {
            std::vector<char> message(logLength + 1);
            mGles2->glGetShaderInfoLog(shaderId, logLength, nullptr,
                                       message.data());
            E("%s: Failed to compile shader: %s", __FUNCTION__, message.data());
        } else {
            E("%s: Failed to compile shader", __FUNCTION__);
        }

        mGles2->glDeleteShader(shaderId);
        return 0;
    }

    return shaderId;
}

GLuint RendererImpl::linkShaders(GLuint vertexId, GLuint fragmentId) {
    const GLuint programId = mGles2->glCreateProgram();
    mGles2->glAttachShader(programId, vertexId);
    mGles2->glAttachShader(programId, fragmentId);
    mGles2->glLinkProgram(programId);

    // Output the error message if the program failed to link.
    GLint result = GL_FALSE;
    mGles2->glGetProgramiv(programId, GL_LINK_STATUS, &result);
    if (result != GL_TRUE) {
        int logLength = 0;
        mGles2->glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength) {
            std::vector<char> message(logLength + 1);
            mGles2->glGetProgramInfoLog(programId, logLength, nullptr,
                                        message.data());
            E("%s: Failed to link program: %s", __FUNCTION__, message.data());
        } else {
            E("%s: Failed to link program", __FUNCTION__);
        }

        mGles2->glDeleteProgram(programId);
        return 0;
    }

    return programId;
}

GLint RendererImpl::getAttribLocation(GLuint program, const char* name) {
    GLint location = mGles2->glGetAttribLocation(program, name);
    if (location < 0) {
        W("%s: Program attrib '%s' not found", __FUNCTION__, name);
    }

    return location;
}

GLint RendererImpl::getUniformLocation(GLuint program, const char* name) {
    GLint location = mGles2->glGetUniformLocation(program, name);
    if (location < 0) {
        W("%s: Program uniform '%s' not found", __FUNCTION__, name);
    }

    return location;
}

void RendererImpl::processRenderable(
        const Renderable& renderable,
        RenderableParameterCallback parameterCallback) {
    const auto materialIt = mMaterials.find(renderable.material.id);
    const auto meshIt = mMeshes.find(renderable.mesh.id);

    if (materialIt == mMaterials.end()) {
        E("%s: Could not find material id %d", __FUNCTION__,
          renderable.material.id);
        return;
    }

    if (meshIt == mMeshes.end()) {
        E("%s: Could not find mesh id %d", __FUNCTION__, renderable.mesh.id);
        return;
    }

    const MaterialData& material = materialIt->second;
    const MeshData& mesh = meshIt->second;
    const VertexInfo& vertexInfo = mesh.mVertexInfo;

    mGles2->glUseProgram(material.program);

    // Bind the texture if it exists.
    if (renderable.texture.isValid()) {
        mGles2->glActiveTexture(GL_TEXTURE0);
        mGles2->glUniform1i(material.texSamplerLocation, 0);

        mGles2->glBindTexture(GL_TEXTURE_2D,
                              static_cast<GLuint>(renderable.texture.id));
    } else {
        mGles2->glBindTexture(GL_TEXTURE_2D, 0);
    }

    parameterCallback(material);

    mGles2->glBindBuffer(GL_ARRAY_BUFFER, mesh.mVertexBuffer);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.mIndexBuffer);

    // glVertexAttribPointer requires the GL_ARRAY_BUFFER to be bound, keep
    // it bound until all of the vertex attribs are initialized.
    if (material.positionLocation >= 0) {
        mGles2->glEnableVertexAttribArray(material.positionLocation);
        mGles2->glVertexAttribPointer(
                material.positionLocation,
                3,                                               // size
                GL_FLOAT,                                        // type
                GL_FALSE,                                        // normalized?
                vertexInfo.vertexSize,                           // stride
                reinterpret_cast<void*>(vertexInfo.posOffset));  // offset
    }

    if (material.uvLocation >= 0 && vertexInfo.hasUV) {
        mGles2->glEnableVertexAttribArray(material.uvLocation);
        mGles2->glVertexAttribPointer(
                material.uvLocation,
                2,                                              // size
                GL_FLOAT,                                       // type
                GL_FALSE,                                       // normalized?
                vertexInfo.vertexSize,                          // stride
                reinterpret_cast<void*>(vertexInfo.uvOffset));  // offset
    }

    mGles2->glDrawElements(GL_TRIANGLES, mesh.mIndexCount, GL_UNSIGNED_INT,
                           nullptr);

    if (material.positionLocation >= 0) {
        mGles2->glDisableVertexAttribArray(material.positionLocation);
    }
    if (material.uvLocation >= 0 && vertexInfo.hasUV) {
        mGles2->glDisableVertexAttribArray(material.uvLocation);
    }
}

}  // namespace virtualscene
}  // namespace android
