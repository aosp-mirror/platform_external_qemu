// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/device-3d-widget.h"

#include "android/opengles.h"

#include <assert.h>                                   // for assert
#include <glm/gtc/quaternion.hpp>                     // for tquat
#include <qimage.h>                                   // for QImage::Format_...
#include <qiodevice.h>                                // for operator|, QIOD...
#include <qloggingcategory.h>                         // for qCWarning
#include <qnamespace.h>                               // for ClickFocus
#include <QByteArray>                                 // for QByteArray
#include <QFile>                                      // for QFile
#include <QImage>                                     // for QImage
#include <QMouseEvent>                                // for QMouseEvent
#include <QPoint>                                     // for QPoint
#include <QTextStream>                                // for QTextStream
#include <QWheelEvent>                                // for QWheelEvent
#include <algorithm>                                  // for max, min
#include <vector>                                     // for vector

#include "OpenGLESDispatch/GLESv2Dispatch.h"          // for GLESv2Dispatch
#include "android/emulation/control/sensors_agent.h"  // for QAndroidSensors...
#include "android/globals.h"                          // for QAndroidSensors...
#include "android/hw-sensors.h"                       // for PHYSICAL_PARAME...
#include "android/physics/GlmHelpers.h"               // for fromEulerAnglesXYZ
#include "android/physics/Physics.h"                  // for PARAMETER_VALUE...
#include "android/skin/qt/gl-common.h"                // for createShader
#include "android/skin/qt/logging-category.h"         // for emu
#include "android/skin/qt/wavefront-obj-parser.h"     // for parseWavefrontOBJ

using android::base::System;

class QMouseEvent;
class QWheelEvent;
class QWidget;

static glm::vec3 clampPosition(glm::vec3 position) {
    return glm::clamp(
            position,
            glm::vec3(Device3DWidget::MinX, Device3DWidget::MinY,
                      Device3DWidget::MinZ),
            glm::vec3(Device3DWidget::MaxX, Device3DWidget::MaxY,
                      Device3DWidget::MaxZ));
}

static constexpr int kAnimationIntervalMs = 33;

Device3DWidget::Device3DWidget(QWidget* parent)
    : GLWidget(parent),
      mUseAbstractDevice(
          android_hw->hw_sensor_hinge) {

    toggleAA();
    setFocusPolicy(Qt::ClickFocus);

    if (mUseAbstractDevice) {
        mCurrentFoldableStatePtr = android_foldable_get_state_ptr();
        connect(&mAnimationTimer, SIGNAL(timeout()),
                this, SLOT(animate()));
        mAnimationTimer.setInterval(kAnimationIntervalMs);
        mAnimationTimer.start();
    }
}

Device3DWidget::~Device3DWidget() {
    if (mUseAbstractDevice) {
        mAnimationTimer.stop();
    }

    // Free up allocated resources.
    if (!mGLES2) {
        return;
    }
    if (readyForRendering()) {
        if(makeContextCurrent()) {
            mGLES2->glDeleteProgram(mProgram);
            mGLES2->glDeleteBuffers(1, &mVertexDataBuffer);
            mGLES2->glDeleteBuffers(1, &mVertexIndexBuffer);
            mGLES2->glDeleteTextures(1, &mGlossMap);
            mGLES2->glDeleteTextures(1, &mDiffuseMap);
            mGLES2->glDeleteTextures(1, &mSpecularMap);
            mGLES2->glDeleteTextures(1, &mEnvMap);
        }
    }
}

void Device3DWidget::setSensorsAgent(const QAndroidSensorsAgent* agent) {
    mSensorsAgent = agent;
}

void Device3DWidget::setTargetRotation(const glm::quat& rotation) {
    mTargetRotation = rotation;
}

void Device3DWidget::setTargetPosition(const glm::vec3& position) {
    mTargetPosition = clampPosition(position);
}

// Helper function that uploads the data from texture in the file
// specified by |source_filename| into the texture object bound
// at |target| using the given |format|. |format| should be
// either GL_RGBA or GL_LUMINANCE.
// Returns true on success, false on failure.
static bool loadTexture(const GLESv2Dispatch* gles2,
                        const char* source_filename,
                        GLenum target,
                        GLenum format) {
    QImage image =
        QImage(source_filename).convertToFormat(format == GL_RGBA ?
                                                    QImage::Format_RGBA8888 :
                                                    QImage::Format_Grayscale8);
    if (image.isNull()) {
        qCWarning(emu, "Failed to load image \"%s\"", source_filename);
        return false;
    }
    gles2->glTexImage2D(target,
                        0,
                        format,
                        image.width(),
                        image.height(),
                        0,
                        format,
                        GL_UNSIGNED_BYTE,
                        image.bits());
    return gles2->glGetError() == GL_NO_ERROR;
}

// Helper function to create a 2D texture from a given file
// with the given parameters.
static GLuint create2DTexture(const GLESv2Dispatch* gles2,
                              const char* source_filename,
                              GLenum target,
                              GLenum format,
                              GLenum min_filter = GL_LINEAR,
                              GLenum mag_filter = GL_LINEAR,
                              GLenum wrap_s = GL_CLAMP_TO_EDGE,
                              GLenum wrap_t = GL_CLAMP_TO_EDGE) {
    GLuint texture = 0;
    gles2->glGenTextures(1, &texture);

    // Upload texture data and set parameters.
    gles2->glBindTexture(GL_TEXTURE_2D, texture);

    // If blank, have a blank texture (black)
    if (!source_filename) {
        uint32_t black = 0;
        gles2->glTexImage2D(
            GL_TEXTURE_2D,
            0, format, 1, 1, 0,
            format, GL_UNSIGNED_BYTE, &black);
    } else {
        if(!loadTexture(gles2,
                        source_filename,
                        target,
                        format)) return 0;
    }

    gles2->glTexParameteri(target, GL_TEXTURE_MIN_FILTER, min_filter);
    gles2->glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mag_filter);
    gles2->glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap_s);
    gles2->glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap_t);

    // Generate mipmaps if necessary.
    if (min_filter == GL_NEAREST_MIPMAP_NEAREST ||
        min_filter == GL_NEAREST_MIPMAP_LINEAR ||
        min_filter == GL_LINEAR_MIPMAP_NEAREST ||
        min_filter == GL_LINEAR_MIPMAP_LINEAR) {
        gles2->glGenerateMipmap(GL_TEXTURE_2D);
    }

    return gles2->glGetError() == GL_NO_ERROR ? texture : 0;
}

bool Device3DWidget::initGL() {
    if (!mGLES2) {
        return false;
    }

    // Perform initial set-up.

    // Enable depth testing and blending.
    mGLES2->glEnable(GL_DEPTH_TEST);
    mGLES2->glEnable(GL_BLEND);

    // Set the blend function.
    mGLES2->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Clear screen to gray.
    mGLES2->glClearColor(0.5, 0.5, 0.5, 1.0);

    // Set up the camera transformation.
    mCameraTransform = glm::lookAt(
        glm::vec3(0.0f, 0.0f, CameraDistance), // Camera position
        glm::vec3(0.0f, 0.0f, 0.0f), // Point at which the camera is looking
        glm::vec3(0.0f, 1.0f, 0.0f)); // "up" vector
    mCameraTransformInverse = glm::inverse(mCameraTransform);

    if (!initProgram() ||
        !initModel() ||
        !initTextures()) {
        return false;
    }

    resizeGL(width(), height());
    return true;
}

bool Device3DWidget::initProgram() {
    // Compile & link shaders.
    QFile vertex_shader_file(":/phone-model/vert.glsl");
    QFile fragment_shader_file(":/phone-model/frag.glsl");
    vertex_shader_file.open(QFile::ReadOnly | QFile::Text);
    fragment_shader_file.open(QFile::ReadOnly | QFile::Text);
    QByteArray vertex_shader_code = vertex_shader_file.readAll();
    QByteArray fragment_shader_code = fragment_shader_file.readAll();
    GLuint vertex_shader =
        createShader(mGLES2, GL_VERTEX_SHADER, vertex_shader_code.constData());
    GLuint fragment_shader =
        createShader(mGLES2, GL_FRAGMENT_SHADER, fragment_shader_code.constData());
    if (vertex_shader == 0 || fragment_shader == 0) {
        return false;
    }
    mProgram = mGLES2->glCreateProgram();
    mGLES2->glAttachShader(mProgram, vertex_shader);
    mGLES2->glAttachShader(mProgram, fragment_shader);
    mGLES2->glLinkProgram(mProgram);
    GLint compile_status = 0;
    mGLES2->glGetProgramiv(mProgram, GL_LINK_STATUS, &compile_status);
    if (compile_status == GL_FALSE) {
        qCWarning(emu, "Failed to link program");
        return false;
    }
    CHECK_GL_ERROR_RETURN("Failed to initialize shaders", false);

    // We no longer need shader objects after successful program linkage.
    mGLES2->glDetachShader(mProgram, vertex_shader);
    mGLES2->glDetachShader(mProgram, fragment_shader);
    mGLES2->glDeleteShader(vertex_shader);
    mGLES2->glDeleteShader(fragment_shader);
    return true;
}

bool Device3DWidget::initModel() {
    if (mUseAbstractDevice) {
        // Create an abstract parameterized model
        // std::vector<float> model_vertex_data;
        // std::vector<GLuint> indices;

        // Size it to the aspect ratio
        float aspect =
            (float)(android_hw->hw_lcd_width) /
            (float)(android_hw->hw_lcd_height);

        std::vector<float> pos = {
            -1.0, -1.0, 0.0,
            +1.0, -1.0, 0.0,
            +1.0, +1.0, 0.0,
            -1.0, -1.0, 0.0,
            +1.0, +1.0, 0.0,
            -1.0, +1.0, 0.0,
        };

        for (size_t i = 0; i < pos.size(); ++i) {
            if (i % 3 == 1) {
                pos[i] *= 1.0f / aspect;
            }
            pos[i] *= 2.0f;
        }

        std::vector<float> model_vertex_data = {
            pos[0], pos[1], pos[2],    0.0, 0.0, 1.0,    0.0, 0.0,
            pos[3], pos[4], pos[5],    0.0, 0.0, 1.0,    1.0, 0.0,
            pos[6], pos[7], pos[8],    0.0, 0.0, 1.0,    1.0, 1.0,
            pos[9], pos[10], pos[11],    0.0, 0.0, 1.0,    0.0, 0.0,
            pos[12], pos[13], pos[14],    0.0, 0.0, 1.0,    1.0, 1.0,
            pos[15], pos[16], pos[17],    0.0, 0.0, 1.0,    0.0, 1.0,
        };

        std::vector<GLuint> indices = {
            0, 1, 2, 3, 4, 5,
        };


        mElementsCount = indices.size();

        mVertexPositionAttribute = mGLES2->glGetAttribLocation(mProgram, "vtx_pos");
        mVertexNormalAttribute   = mGLES2->glGetAttribLocation(mProgram, "vtx_normal");
        mVertexUVAttribute   = mGLES2->glGetAttribLocation(mProgram, "vtx_uv");
        mGLES2->glGenBuffers(1, &mVertexDataBuffer);
        mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mVertexDataBuffer);
        mGLES2->glBufferData(GL_ARRAY_BUFFER,
                             model_vertex_data.size() * sizeof(float),
                             &model_vertex_data[0],
                             GL_STATIC_DRAW);
        mGLES2->glGenBuffers(1, &mVertexIndexBuffer);
        mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexIndexBuffer);
        mGLES2->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                             mElementsCount * sizeof(GLuint),
                             &indices[0],
                             GL_STATIC_DRAW);

        mCachedFoldableState = *mCurrentFoldableStatePtr;
        std::pair<std::vector<float>, std::vector<uint32_t>> newData = 
            generateModelVerticesFromFoldableState(mCachedFoldableState);
        updateModelVertices(
            newData.first.data(), newData.first.size() * sizeof(float),
            newData.second.data(), newData.second.size() * sizeof(uint32_t));

        CHECK_GL_ERROR_RETURN("Failed to load model", false);
        return true;
    } else {
        // Load the model and set up buffers.
        std::vector<float> model_vertex_data;
        std::vector<GLuint> indices;
        QFile model_file(":/phone-model/model.obj");
        if (model_file.open(QFile::ReadOnly)) {
            QTextStream file_stream(&model_file);
            if(!parseWavefrontOBJ(file_stream, model_vertex_data, indices)) {
                qCWarning(emu, "Failed to load model");
                return false;
            }
        } else {
            qCWarning(emu, "Failed to open model file for reading");
            return false;
        }
        mElementsCount = indices.size();

        mVertexPositionAttribute = mGLES2->glGetAttribLocation(mProgram, "vtx_pos");
        mVertexNormalAttribute   = mGLES2->glGetAttribLocation(mProgram, "vtx_normal");
        mVertexUVAttribute   = mGLES2->glGetAttribLocation(mProgram, "vtx_uv");
        mGLES2->glGenBuffers(1, &mVertexDataBuffer);
        mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mVertexDataBuffer);
        mGLES2->glBufferData(GL_ARRAY_BUFFER,
                             model_vertex_data.size() * sizeof(float),
                             &model_vertex_data[0],
                             GL_STATIC_DRAW);
        mGLES2->glGenBuffers(1, &mVertexIndexBuffer);
        mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexIndexBuffer);
        mGLES2->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                             mElementsCount * sizeof(GLuint),
                             &indices[0],
                             GL_STATIC_DRAW);
        CHECK_GL_ERROR_RETURN("Failed to load model", false);
        return true;
    }
}

bool Device3DWidget::initTextures() {
    // Create textures.
    mDiffuseMap = create2DTexture(mGLES2,
                                  ":/phone-model/diffuse-map.png",
                                  GL_TEXTURE_2D,
                                  GL_RGBA,
                                  GL_LINEAR_MIPMAP_LINEAR);
    if (mUseAbstractDevice) {
        mSpecularMap = create2DTexture(
            mGLES2, nullptr /* blank */, GL_TEXTURE_2D, GL_LUMINANCE);
        mGlossMap = create2DTexture(
            mGLES2, nullptr /* blank */, GL_TEXTURE_2D, GL_LUMINANCE);
    } else {
        mSpecularMap = create2DTexture(mGLES2,
                                       ":/phone-model/specular-map.png",
                                       GL_TEXTURE_2D,
                                       GL_LUMINANCE);
        mGlossMap = create2DTexture(mGLES2,
                                    ":/phone-model/gloss-map.png",
                                    GL_TEXTURE_2D,
                                    GL_LUMINANCE);
    }
    if (!mDiffuseMap || !mSpecularMap || !mGlossMap) {
        return false;
    }
    mGLES2->glGenTextures(1, &mEnvMap);
    mGLES2->glBindTexture(GL_TEXTURE_CUBE_MAP, mEnvMap);
    loadTexture(mGLES2, ":/phone-model/env-map-front.png", GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_RGBA);
    loadTexture(mGLES2, ":/phone-model/env-map-back.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, GL_RGBA);
    loadTexture(mGLES2, ":/phone-model/env-map-right.png", GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_RGBA);
    loadTexture(mGLES2, ":/phone-model/env-map-left.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_RGBA);
    loadTexture(mGLES2, ":/phone-model/env-map-top.png", GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_RGBA);
    loadTexture(mGLES2, ":/phone-model/env-map-bottom.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_RGBA);
    mGLES2->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    mGLES2->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    mGLES2->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    mGLES2->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    mGLES2->glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    CHECK_GL_ERROR_RETURN("Failed to initialize cubemap", false);
    return true;
}

void Device3DWidget::updateModelVertices(
    const void* vertexData, size_t vertexDataBytes,
    const void* indexData, size_t indexDataBytes) {

    if (vertexDataBytes == 0) return;

    mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mVertexDataBuffer);
    mGLES2->glBufferData(GL_ARRAY_BUFFER,
                         vertexDataBytes,
                         vertexData,
                         GL_STATIC_DRAW);
    mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexIndexBuffer);
    mGLES2->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         indexDataBytes,
                         indexData,
                         GL_STATIC_DRAW);
    mElementsCount = indexDataBytes / sizeof(uint32_t);
}

std::pair<
    std::vector<float>,
    std::vector<uint32_t>>
Device3DWidget::generateModelVerticesFromFoldableState(const FoldableState& state) {

    // TODO: Generalize
    // Assume split 0.5 for now, with 1 hinge
    if (state.config.numHinges < 1) return {};

    float hingeRadians = state.currentHingeDegrees[0] * M_PI / 180.0f;

    float swingy = cos(hingeRadians - M_PI);
    float swingz = sin(hingeRadians - M_PI);

    float swingyn = cos(hingeRadians - M_PI * 0.5f);
    float swingzn = sin(hingeRadians - M_PI * 0.5f);

    std::vector<float> attribs = {
        -1.0, -1.0, 0.0,   0.0, 0.0, 1.0,  0.0, 0.0, // 0
        +1.0, -1.0, 0.0,   0.0, 0.0, 1.0,  1.0, 0.0, // 1
        // Hinge, attached to non-swinging part
        -1.0, 0.0, 0.0,   0.0, 0.0, 1.0,  0.0, 0.5, // 2
        +1.0, 0.0, 0.0,   0.0, 0.0, 1.0,  1.0, 0.5, // 3
        // Hinge, attached to swinging part
        -1.0, 0.0, 0.0,   0.0, swingyn, swingzn,  0.0, 0.5, // 4
        +1.0, 0.0, 0.0,   0.0, swingyn, swingzn,  1.0, 0.5, // 5
        // Swinging part
        +1.0, 1.0, 0.0,   0.0, swingyn, swingzn,  1.0, 1.0, // 6
        -1.0, 1.0, 0.0,   0.0, swingyn, swingzn,  0.0, 1.0, // 7
    };

    // Size it to the aspect ratio
    float aspect =
        (float)(android_hw->hw_lcd_width) /
        (float)(android_hw->hw_lcd_height);

    float yheight = 1.0f;

    // TODO: Remove hacky computation for the swung-out part
    for (size_t i = 0; i < attribs.size(); ++i) {
        if (i % 8 == 1) {
            attribs[i] *= 1.0f / aspect;
        }
        if (i % 8 < 4) {
            attribs[i] *= 2.0f;
        }

        if (i >= 6 * 8 && i < 8 * 8) {
            if (i % 8 == 1) {
                yheight = attribs[i];
            }
        }
    }

    for (size_t i = 0; i < attribs.size(); ++i) {
        if (i >= 6 * 8 && i < 8 * 8) {

            if (i % 8 == 2) {
                attribs[i] = sin(hingeRadians - M_PI) * yheight;
            }
            if (i % 8 == 1) {
                attribs[i] = cos(hingeRadians - M_PI) * yheight;
            }
        }

    }

    std::vector<uint32_t> indices = {
        0, 1, 2,
        1, 3, 2,
        4, 5, 7,
        7, 6, 5,
    };

    return {attribs, indices};
}

void Device3DWidget::resizeGL(int w, int h) {
    if (!mGLES2) {
        return;
    }

    // Adjust for new viewport
    mGLES2->glViewport(0, 0, w, h);

    float aspect_ratio = static_cast<float>(w) / static_cast<float>(h);
    float min_move_aspect_ratio = (MaxX - MinX) / (MaxY - MinY);
    float plane_width = aspect_ratio > min_move_aspect_ratio ?
            (MaxY - MinY) * aspect_ratio :
            MaxX - MinY;
    float plane_height = aspect_ratio > min_move_aspect_ratio ?
            MaxY - MinY :
            (MaxX - MinX) / min_move_aspect_ratio;

    // Moving the phone should not clip the near/far planes.  Note that the
    // camera is looking in the -z direction so MinZ is the farthest the phone
    // model can move from the camera.
    assert(NearClip < CameraDistance - MaxZ);
    assert(FarClip > CameraDistance - MinZ);

    // Recompute the perspective projection transform.
    mPerspective = glm::frustum(
            -plane_width * NearClip / (2.0f * CameraDistance),
            plane_width * NearClip / (2.0f * CameraDistance),
            -plane_height * NearClip / (2.0f * CameraDistance),
            plane_height * NearClip / (2.0f * CameraDistance),
            NearClip,
            FarClip);
    mPerspectiveInverse = glm::inverse(mPerspective);
}

constexpr float kMetersPerInch = 0.0254f;

void Device3DWidget::repaintGL() {
    if (!mGLES2) {
        return;
    }

    // Handle repaint event.
    glm::vec3 position = glm::vec3();
    glm::mat4 rotation = glm::mat4();

    if (mSensorsAgent) {
        mSensorsAgent->getPhysicalParameter(
                PHYSICAL_PARAMETER_POSITION, &position.x, &position.y,
                &position.z, PARAMETER_VALUE_TYPE_CURRENT);
        position = (1.f / kMetersPerInch) * position;

        glm::vec3 eulerDegrees;
        mSensorsAgent->getPhysicalParameter(
                PHYSICAL_PARAMETER_ROTATION, &eulerDegrees.x, &eulerDegrees.y,
                &eulerDegrees.z, PARAMETER_VALUE_TYPE_CURRENT);

        // Clamp the position to the visible range.
        position = clampPosition(position);

        rotation = glm::mat4(fromEulerAnglesXYZ(glm::radians(eulerDegrees)));
    }

    // Upload new vertex data from foldable state, if necessary
    if (mUseAbstractDevice) {
        if (memcmp(&mCachedFoldableState, mCurrentFoldableStatePtr, sizeof(FoldableState))) {
            mCachedFoldableState = *mCurrentFoldableStatePtr;
            std::pair<std::vector<float>, std::vector<uint32_t>> newData = 
                generateModelVerticesFromFoldableState(mCachedFoldableState);
            updateModelVertices(
                newData.first.data(), newData.first.size() * sizeof(float),
                newData.second.data(), newData.second.size() * sizeof(uint32_t));
        }
    }

    if (mOperationMode == OperationMode::Rotate &&
        quaternionNearEqual(mLastTargetRotation, mTargetRotation)) {
        rotation = glm::mat4_cast(mLastTargetRotation);
    } else if (mOperationMode == OperationMode::Move &&
               vecNearEqual(mLastTargetPosition, mTargetPosition)) {
        position = mLastTargetPosition;
    }

    // Recompute the model transformation matrix using the given rotations.
    const glm::mat4 model_transform =
            glm::translate(glm::mat4(), position) * rotation;
    const glm::mat4 model_view_transform =
            mCameraTransform * model_transform;

    // Normals need to be transformed using the inverse transpose of modelview matrix.
    glm::mat4 normal_transform = glm::transpose(glm::inverse(model_view_transform));

    // Recompute the model-view-projection matrix using the new model transform.
    glm::mat4 model_view_projection = mPerspective * model_view_transform;

    // Give the new MVP matrix to the shader.
    mGLES2->glUseProgram(mProgram);

    // Upload matrices.
    GLuint mv_matrix_uniform = mGLES2->glGetUniformLocation(mProgram, "model_view");
    mGLES2->glUniformMatrix4fv(mv_matrix_uniform, 1, GL_FALSE, &model_view_transform[0][0]);

    GLuint mvit_matrix_uniform = mGLES2->glGetUniformLocation(mProgram, "model_view_inverse_transpose");
    mGLES2->glUniformMatrix4fv(mvit_matrix_uniform, 1, GL_FALSE, &normal_transform[0][0]);

    GLuint mvp_matrix_uniform = mGLES2->glGetUniformLocation(mProgram, "model_view_projection");
    mGLES2->glUniformMatrix4fv(mvp_matrix_uniform, 1, GL_FALSE, &model_view_projection[0][0]);

    // Set textures.
    mGLES2->glActiveTexture(GL_TEXTURE0);
    mGLES2->glBindTexture(GL_TEXTURE_2D, mDiffuseMap);

    // Replace the current diffuse map w/ screen contents
    // if we are using the abstract device
    if (mUseAbstractDevice) {
        struct AndroidVirtioGpuOps* ops = android_getVirtioGpuOps();
        ops->bind_color_buffer_to_texture(ops->get_last_posted_color_buffer());
    }

    mGLES2->glActiveTexture(GL_TEXTURE1);
    mGLES2->glBindTexture(GL_TEXTURE_2D, mSpecularMap);
    mGLES2->glActiveTexture(GL_TEXTURE2);
    mGLES2->glBindTexture(GL_TEXTURE_2D, mGlossMap);
    mGLES2->glActiveTexture(GL_TEXTURE3);
    mGLES2->glBindTexture(GL_TEXTURE_CUBE_MAP, mEnvMap);

    GLuint diffuse_map_uniform = mGLES2->glGetUniformLocation(mProgram, "diffuse_map");
    mGLES2->glUniform1i(diffuse_map_uniform, 0);
    GLuint specular_map_uniform = mGLES2->glGetUniformLocation(mProgram, "specular_map");
    mGLES2->glUniform1i(specular_map_uniform, 1);
    GLuint gloss_map_uniform = mGLES2->glGetUniformLocation(mProgram, "gloss_map");
    mGLES2->glUniform1i(gloss_map_uniform, 2);
    GLuint env_map_uniform = mGLES2->glGetUniformLocation(mProgram, "env_map");
    mGLES2->glUniform1i(env_map_uniform, 3);
    GLuint no_light_uniform = mGLES2->glGetUniformLocation(mProgram, "no_light");
    if (mUseAbstractDevice) {
        mGLES2->glUniform1f(no_light_uniform, 1.0f);
    } else {
        mGLES2->glUniform1f(no_light_uniform, 0.0f);
    }

    // Set up attribute pointers.
    mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mVertexDataBuffer);
    mGLES2->glEnableVertexAttribArray(mVertexPositionAttribute);
    mGLES2->glEnableVertexAttribArray(mVertexNormalAttribute);
    mGLES2->glEnableVertexAttribArray(mVertexUVAttribute);
    mGLES2->glVertexAttribPointer(mVertexPositionAttribute,
                                  3,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(float) * 8,
                                  0);
    mGLES2->glVertexAttribPointer(mVertexNormalAttribute,
                                  3,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(float) * 8,
                                  (void*)(sizeof(float) * 3));
    mGLES2->glVertexAttribPointer(mVertexUVAttribute,
                                  2,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(float) * 8,
                                  (void*)(sizeof(float) * 6));
    mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexIndexBuffer);

    // Draw the model.
    mGLES2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mGLES2->glDrawElements(GL_TRIANGLES, mElementsCount, GL_UNSIGNED_INT, 0);
}

static float clamp(float a, float b, float x) {
    return std::max(a, std::min(b, x));
}

void Device3DWidget::mouseMoveEvent(QMouseEvent* event) {
    if (mTracking && mOperationMode == OperationMode::Rotate) {
        float diff_x = event->x() - mPrevMouseX,
              diff_y = event->y() - mPrevMouseY;
        const glm::quat q = glm::angleAxis(glm::radians(diff_y),
                                     glm::vec3(1.0f, 0.0f, 0.0f)) *
                      glm::angleAxis(glm::radians(diff_x),
                                     glm::vec3(0.0f, 1.0f, 0.0f));
        mTargetRotation = q * mTargetRotation;
        mLastTargetRotation = mTargetRotation;
        renderFrame();
        emit targetRotationChanged();
        mPrevMouseX = event->x();
        mPrevMouseY = event->y();
    } else if (mTracking && mOperationMode == OperationMode::Move) {
        const glm::vec3 vec = screenToWorldCoordinate(event->x(), event->y());
        const glm::vec3 newLocation = mTargetPosition + vec - mPrevDragOrigin;
        mTargetPosition.x = clamp(MinX, MaxX, newLocation.x);
        mTargetPosition.y = clamp(MinY, MaxY, newLocation.y);
        mLastTargetPosition = mTargetPosition;
        renderFrame();
        emit targetPositionChanged();
        mPrevDragOrigin = vec;
    }
}

void Device3DWidget::wheelEvent(QWheelEvent* event) {
    if (mOperationMode == OperationMode::Move) {
        // Note: angleDelta is given in 1/8 degree increments.
        const int angleDeltaDegrees = event->angleDelta().y() / 8;
        mTargetPosition.z = clamp(
                MinZ, MaxZ,
                mTargetPosition.z + angleDeltaDegrees * InchesPerWheelDegree);
        mLastTargetPosition = mTargetPosition;
        renderFrame();
        emit targetPositionChanged();
    }
}

void Device3DWidget::mousePressEvent(QMouseEvent* event) {
    mPrevMouseX = event->x();
    mPrevMouseY = event->y();
    mPrevDragOrigin = screenToWorldCoordinate(event->x(), event->y());
    mTracking = true;
    emit dragStarted();
}

void Device3DWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (mTracking) {
        mTracking = false;
        emit dragStopped();
    }

    emit targetRotationChanged();
}

glm::vec3 Device3DWidget::screenToWorldCoordinate(int x, int y) const {
    const glm::vec4 screenSpace((2.0f * x / static_cast<float>(width())) - 1.0f,
                  1.0f - (2.0f * y / static_cast<float>(height())),
                  -1.f,
                  1.f);

    // Now, by applying the inverse perspective transform, move vec into camera
    // space.
    const glm::vec4 cameraSpaceNearPlaneCoordinate =
            mPerspectiveInverse * screenSpace;

    // Move to the current z-translation plane in camera space.
    const glm::vec4 cameraSpacePhonePlaneCoordinate =
            cameraSpaceNearPlaneCoordinate *
            ((CameraDistance - mTargetPosition.z) / NearClip);

    // Move plane position to world space.
    return mCameraTransformInverse * cameraSpacePhonePlaneCoordinate;
}
