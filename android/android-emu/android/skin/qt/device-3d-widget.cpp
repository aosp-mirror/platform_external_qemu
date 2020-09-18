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

#include <assert.h>            // for assert
#include <qimage.h>            // for QImage::Format_...
#include <qiodevice.h>         // for operator|, QIOD...
#include <qloggingcategory.h>  // for qCWarning
#include <qnamespace.h>        // for ClickFocus

#include <QBitmap>
#include <QByteArray>   // for QByteArray
#include <QFile>        // for QFile
#include <QImage>       // for QImage
#include <QMouseEvent>  // for QMouseEvent
#include <QPixmap>
#include <QPoint>                  // for QPoint
#include <QTextStream>             // for QTextStream
#include <QWheelEvent>             // for QWheelEvent
#include <algorithm>               // for max, min
#include <glm/gtc/quaternion.hpp>  // for tquat
#include <vector>                  // for vector

#include "OpenGLESDispatch/GLESv2Dispatch.h"  // for GLESv2Dispatch
#include "android/base/files/PathUtils.h"
#include "android/emulation/control/sensors_agent.h"  // for QAndroidSensors...
#include "android/globals.h"                          // for QAndroidSensors...
#include "android/hw-sensors.h"                       // for PHYSICAL_PARAME...
#include "android/opengles.h"
#include "android/physics/GlmHelpers.h"  // for fromEulerAnglesXYZ
#include "android/physics/Physics.h"     // for PARAMETER_VALUE...
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/gl-common.h"             // for createShader
#include "android/skin/qt/logging-category.h"      // for emu
#include "android/skin/qt/wavefront-obj-parser.h"  // for parseWavefrontOBJ

using android::base::System;

class QMouseEvent;
class QWheelEvent;
class QWidget;

static glm::vec3 clampPosition(glm::vec3 position) {
    return glm::clamp(position,
                      glm::vec3(Device3DWidget::MinX, Device3DWidget::MinY,
                                Device3DWidget::MinZ),
                      glm::vec3(Device3DWidget::MaxX, Device3DWidget::MaxY,
                                Device3DWidget::MaxZ));
}

static constexpr int kAnimationIntervalMs = 33;

Device3DWidget::Device3DWidget(QWidget* parent)
    : GLWidget(parent), mUseAbstractDevice(android_foldable_hinge_configured() || android_foldable_rollable_configured()) {
    toggleAA();
    setFocusPolicy(Qt::ClickFocus);

    if (mUseAbstractDevice) {
        connect(&mAnimationTimer, SIGNAL(timeout()), this, SLOT(animate()));
        mAnimationTimer.setInterval(kAnimationIntervalMs);
    }
}

Device3DWidget::~Device3DWidget() {
    if (mUseAbstractDevice && mAnimationTimer.isActive()) {
        mAnimationTimer.stop();
    }

    // Free up allocated resources.
    if (!mGLES2) {
        return;
    }
    if (readyForRendering()) {
        if (makeContextCurrent()) {
            mGLES2->glDeleteProgram(mProgram);
            mGLES2->glDeleteBuffers(1, &mVertexDataBuffer);
            mGLES2->glDeleteBuffers(1, &mVertexIndexBuffer);
            mGLES2->glDeleteTextures(1, &mDiffuseMap);
            mGLES2->glDeleteTextures(1, &mSpecularMap);
            if (!mUseAbstractDevice) {
                mGLES2->glDeleteTextures(1, &mGlossMap);
                mGLES2->glDeleteTextures(1, &mEnvMap);
            }
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
            QImage(source_filename)
                    .convertToFormat(format == GL_RGBA
                                             ? QImage::Format_RGBA8888
                                             : QImage::Format_Grayscale8);
    if (image.isNull()) {
        qCWarning(emu, "Failed to load image \"%s\"", source_filename);
        return false;
    }
    gles2->glTexImage2D(target, 0, format, image.width(), image.height(), 0,
                        format, GL_UNSIGNED_BYTE, image.bits());
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
                              GLenum wrap_t = GL_CLAMP_TO_EDGE,
                              uint32_t color = 0) {
    GLuint texture = 0;
    gles2->glGenTextures(1, &texture);

    // Upload texture data and set parameters.
    gles2->glBindTexture(GL_TEXTURE_2D, texture);

    // If blank, have a blank texture (black by default)
    if (!source_filename) {
        gles2->glTexImage2D(GL_TEXTURE_2D, 0, format, 1, 1, 0, format,
                            GL_UNSIGNED_BYTE, &color);
    } else {
        if (!loadTexture(gles2, source_filename, target, format))
            return 0;
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

    // Enable depth testing and blending.
    mGLES2->glEnable(GL_DEPTH_TEST);
    mGLES2->glEnable(GL_BLEND);

    // Set the blend function.
    mGLES2->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Clear screen to gray.
    mGLES2->glClearColor(0.5, 0.5, 0.5, 1.0);

    // Set up the camera transformation.
    mCameraTransform = glm::lookAt(
            glm::vec3(0.0f, 0.0f, CameraDistance),  // Camera position
            glm::vec3(0.0f, 0.0f,
                      0.0f),  // Point at which the camera is looking
            glm::vec3(0.0f, 1.0f, 0.0f));  // "up" vector
    mCameraTransformInverse = glm::inverse(mCameraTransform);

    if (!initProgram() || !initModel() || !initTextures()) {
        return false;
    }

    resizeGL(width(), height());
    return true;
}

bool Device3DWidget::initProgram() {
    // Compile & link shaders.
    QFile vertex_shader_file(":/phone-model/vert.glsl");
    QString shader_file_name =
            mUseAbstractDevice ? ":/phone-model/frag-abstract-device.glsl"
                               : ":/phone-model/frag.glsl";
    QFile fragment_shader_file(shader_file_name);
    vertex_shader_file.open(QFile::ReadOnly | QFile::Text);
    fragment_shader_file.open(QFile::ReadOnly | QFile::Text);
    QByteArray vertex_shader_code = vertex_shader_file.readAll();
    QByteArray fragment_shader_code = fragment_shader_file.readAll();
    GLuint vertex_shader = createShader(mGLES2, GL_VERTEX_SHADER,
                                        vertex_shader_code.constData());
    GLuint fragment_shader = createShader(mGLES2, GL_FRAGMENT_SHADER,
                                          fragment_shader_code.constData());
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
        mGLES2->glGenBuffers(1, &mVertexDataBuffer);
        mGLES2->glGenBuffers(1, &mVertexIndexBuffer);
        return initAbstractDeviceModel();
    } else {
        // Load the model and set up buffers.
        std::vector<float> model_vertex_data;
        std::vector<GLuint> indices;
        QFile model_file(":/phone-model/model.obj");
        if (model_file.open(QFile::ReadOnly)) {
            QTextStream file_stream(&model_file);
            if (!parseWavefrontOBJ(file_stream, model_vertex_data, indices)) {
                qCWarning(emu, "Failed to load model");
                return false;
            }
        } else {
            qCWarning(emu, "Failed to open model file for reading");
            return false;
        }
        mElementsCount = indices.size();

        mVertexPositionAttribute =
                mGLES2->glGetAttribLocation(mProgram, "vtx_pos");
        mVertexNormalAttribute =
                mGLES2->glGetAttribLocation(mProgram, "vtx_normal");
        mVertexUVAttribute = mGLES2->glGetAttribLocation(mProgram, "vtx_uv");
        mGLES2->glGenBuffers(1, &mVertexDataBuffer);
        mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mVertexDataBuffer);
        mGLES2->glBufferData(GL_ARRAY_BUFFER,
                             model_vertex_data.size() * sizeof(float),
                             &model_vertex_data[0], GL_STATIC_DRAW);
        mGLES2->glGenBuffers(1, &mVertexIndexBuffer);
        mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexIndexBuffer);
        mGLES2->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                             mElementsCount * sizeof(GLuint), &indices[0],
                             GL_STATIC_DRAW);
        CHECK_GL_ERROR_RETURN("Failed to load model", false);
        return true;
    }
}

bool Device3DWidget::initAbstractDeviceHingeModel(FoldableDisplayType type,
                                                  int numHinges,
                                                  FoldableHingeParameters* hingeParams) {
    // Create an abstract parameterized model
    // std::vector<float> model_vertex_data;
    // std::vector<GLuint> indices;
    bool hSplit = (type == ANDROID_FOLDABLE_HORIZONTAL_SPLIT)
                          ? true
                          : false;
    int32_t displayW =
            hSplit ? android_hw->hw_lcd_width : android_hw->hw_lcd_height;
    int32_t displayH =
            hSplit ? android_hw->hw_lcd_height : android_hw->hw_lcd_width;
    int32_t centerX = displayW / 2;
    int32_t centerY = displayH / 2;
    if (!hSplit) {
        // rotate 90 degree for vertical split, the result is horizontal split
        for (uint32_t i = 0; i < numHinges; i++) {
            std::swap(hingeParams[i].x, hingeParams[i].y);
            std::swap(hingeParams[i].width,
                      hingeParams[i].height);
        }
    }
    int32_t x = 0, y = 0;
    int32_t l = x - centerX;
    int32_t r = -l;
    int32_t t, b;
    struct FoldableHingeParameters* p = hingeParams;
    mDisplaySegments.clear();
    for (int i = 0; i < numHinges; i++) {
        // NOTE: fold default display only
        if (p[i].displayId != 0) {
            continue;
        }
        // Re-position the display segments in the 2D space. Put (centerX,
        // centerY) at point (0, 0).
        t = centerY - y;
        b = centerY - p[i].y;
        // display segement above the hinge
        mDisplaySegments.push_back(
                {l, r, t, b, false, 0.0f, 0.0f, glm::mat4()});
        // hinge
        t = centerY - p[i].y;
        b = t - p[i].height;
        mDisplaySegments.push_back({p[i].x - centerX,
                                    p[i].x - centerX + p[i].width, t, b, true,
                                    p[i].defaultDegrees, 0.0f, glm::mat4()});
        y = p[i].y + p[i].height;
    }
    // last display segment
    t = centerY - y;
    b = -centerY;
    mDisplaySegments.push_back({l, r, t, b, false, 0.0f, 0.0f, glm::mat4()});

    // Find display segment in the middle of the whole display,
    // which will be always placed perpendicular to Z axis
    if (android_foldable_hinge_configured()) {
        for (uint32_t i = 0; i < mDisplaySegments.size(); i++) {
            if (mDisplaySegments[i].t >= 0 && mDisplaySegments[i].b < 0) {
                if (mDisplaySegments[i].isHinge) {
                    mCenterIndex = i + 1;
                } else {
                    mCenterIndex = i;
                }
                break;
            }
        }
    }
    mDisplaySegments[mCenterIndex].rotate = 0;

    // Translate display segments into a new 3D space, ranging [-4, 4] in x and
    // y axis. This will grarantee the phone occpuied the whole widget screen.
    int32_t length = (displayW > displayH) ? displayW : displayH;
    mFactor = 7.9f / (float)length;
    float d = mDepth;
    std::vector<float> attribs;
    std::vector<GLuint> indices;
    std::vector<GLuint> indice = {
            0,  1,  2,  1,  3,  2,  4,  5,  6,  5,  7,  6,
            8,  9,  10, 9,  11, 10, 12, 13, 14, 13, 15, 14,
            16, 17, 18, 17, 19, 18, 20, 21, 22, 21, 23, 22,
    };
    // Construct a cuboid for each segment.
    for (uint32_t idx = 0, j = 0; idx < mDisplaySegments.size(); idx++) {
        const auto i = mDisplaySegments[idx];
        if (i.t == i.b) {
            // do not draw zero-height hinge
            continue;
        }
        float l = i.l * mFactor;
        float r = i.r * mFactor;
        float t = (idx <= mCenterIndex) ? (i.t - i.b) * mFactor : 0.0f;
        float b = (idx <= mCenterIndex) ? 0.0f : (i.b - i.t) * mFactor;
        float ul = (hSplit) ? 0.0f : float(centerY - i.t) / float(displayH);
        float ur = (hSplit) ? 1.0f : float(centerY - i.b) / float(displayH);
        float ub = (hSplit) ? float(i.b + centerY) / float(displayH) : 0.0f;
        float ut = (hSplit) ? float(i.t + centerY) / float(displayH) : 1.0f;
        std::vector<float> attribFront, attribBack;
        if (hSplit) {
            attribFront = {
                    l, b, 0.0, 0.0, 0.0, +1.0, ul, ub,  // front
                    r, b, 0.0, 0.0, 0.0, +1.0, ur, ub,
                    l, t, 0.0, 0.0, 0.0, +1.0, ul, ut,
                    r, t, 0.0, 0.0, 0.0, +1.0, ur, ut,
            };
            attribBack = {
                    l, b, -d, 0.0, 0.0, -1.0, ul, ut,   // back
                    r, b, -d, 0.0, 0.0, -1.0, ur, ut,
                    l, t, -d, 0.0, 0.0, -1.0, ul, ub,
                    r, t, -d, 0.0, 0.0, -1.0, ur, ub,
            };
        } else {
            // rotate 90 degree for vertical split texture sampling
            attribFront = {
                    l, b, 0.0, 0.0, 0.0, +1.0, ur, ub,  // front
                    r, b, 0.0, 0.0, 0.0, +1.0, ur, ut,
                    l, t, 0.0, 0.0, 0.0, +1.0, ul, ub,
                    r, t, 0.0, 0.0, 0.0, +1.0, ul, ut,
            };
            attribBack = {
                    l, b, -d, 0.0, 0.0, -1.0, ul, ub,  // back
                    r, b, -d, 0.0, 0.0, -1.0, ul, ut,
                    l, t, -d, 0.0, 0.0, -1.0, ur, ub,
                    r, t, -d, 0.0, 0.0, -1.0, ur, ut,
            };
        }
        std::vector<float> attribCommon =
                {
                        //                        l, b, -d,  0.0,  0.0,  -1.0,
                        //                        0.0, 0.0,  // back
                        //                        r, b, -d,  0.0,  0.0,
                        //                        -1.0, 1.0, 0.0,
                        //                        l, t, -d,  0.0,  0.0,  -1.0,
                        //                        0.0, 1.0,
                        //                        r, t, -d,  0.0,  0.0,
                        //                        -1.0, 1.0, 1.0,
                        l, b, -d,  -1.0, 0.0,  0.0, 0.0, 0.0,  // left
                        l, b, 0.0, -1.0, 0.0,  0.0, 1.0, 0.0,
                        l, t, -d,  -1.0, 0.0,  0.0, 0.0, 1.0,
                        l, t, 0.0, -1.0, 0.0,  0.0, 1.0, 1.0,
                        r, b, -d,  +1.0, 0.0,  0.0, 0.0, 0.0,  // right
                        r, b, 0.0, +1.0, 0.0,  0.0, 1.0, 0.0,
                        r, t, -d,  +1.0, 0.0,  0.0, 0.0, 1.0,
                        r, t, 0.0, +1.0, 0.0,  0.0, 1.0, 1.0,
                        l, t, 0.0, 0.0,  +1.0, 0.0, 0.0, 0.0,  // top
                        r, t, 0.0, 0.0,  +1.0, 0.0, 1.0, 0.0,
                        l, t, -d,  0.0,  +1.0, 0.0, 0.0, 1.0,
                        r, t, -d,  0.0,  +1.0, 0.0, 1.0, 1.0,
                        l, b, 0.0, 0.0,  -1.0, 0.0, 0.0, 0.0,  // bottom
                        r, b, 0.0, 0.0,  -1.0, 0.0, 1.0, 0.0,
                        l, b, -d,  0.0,  -1.0, 0.0, 0.0, 1.0,
                        r, b, -d,  0.0,  -1.0, 0.0, 1.0, 1.0,
                };
        attribs.insert(attribs.end(), attribFront.begin(), attribFront.end());
        attribs.insert(attribs.end(), attribBack.begin(), attribBack.end());
        attribs.insert(attribs.end(), attribCommon.begin(), attribCommon.end());
        for (auto iter : indice) {
            indices.push_back(iter + j * 24);
        }
        j++;
    }

    mDisplaySegments[mCenterIndex].hingeModelTransfrom = glm::translate(
            glm::mat4(),
            glm::vec3(0, mDisplaySegments[mCenterIndex].b * mFactor, 0));

    mVertexPositionAttribute = mGLES2->glGetAttribLocation(mProgram, "vtx_pos");
    mVertexNormalAttribute =
            mGLES2->glGetAttribLocation(mProgram, "vtx_normal");
    mVertexUVAttribute = mGLES2->glGetAttribLocation(mProgram, "vtx_uv");
    mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mVertexDataBuffer);
    mGLES2->glBufferData(GL_ARRAY_BUFFER, attribs.size() * sizeof(float),
                         attribs.data(), GL_STATIC_DRAW);
    mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexIndexBuffer);
    mGLES2->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         indices.size() * sizeof(GLuint), indices.data(),
                         GL_STATIC_DRAW);
    mElementsCount = indices.size();

    CHECK_GL_ERROR_RETURN("Failed to load model", false);
    return true;
}

#define ROLLABLE_SEGMENT_DEGREE 5
#define PI 3.14125
bool Device3DWidget::initAbstractDeviceRollModel() {
    // Create an abstract parameterized model
    // std::vector<float> model_vertex_data;
    // std::vector<GLuint> indices;
    struct FoldableConfig config = mFoldableState.config;
    bool hRoll = config.type == ANDROID_FOLDABLE_HORIZONTAL_ROLL ? true : false;
    int displayW =
            hRoll ? android_hw->hw_lcd_width : android_hw->hw_lcd_height;
    int displayH =
            hRoll ? android_hw->hw_lcd_height : android_hw->hw_lcd_width;

    struct RollableParameters* p = config.rollableParams;
    // sort the rolls by rolling area low to high
    std::sort(p, p + config.numRolls, [](RollableParameters a, RollableParameters b)
        {return a.minRolledPercent < b.minRolledPercent;} );

    // convert the rollable model to a hinge mode
    int numHinges = 0;
    std::vector<FoldableHingeParameters> hingeParam;
    mCenterIndex = -1;
    for (int i = 0; i < config.numRolls; i++) {
        int left = config.rollableParams[i].minRolledPercent * displayH / 100.0f;
        int right = config.rollableParams[i].maxRolledPercent * displayH /100.0f;
        int current = mFoldableState.currentRolledPercent[i] * displayH / 100.0f;
        float radius = config.rollableParams[i].rollRadiusAsDisplayPercent * displayH / 100.0f;
        int rollSurface = PI * radius;
        int direction = config.rollableParams[i].direction;
        if (direction == 0) {
            if (current - left < rollSurface) {
                // not a 180 degree roll
                int y = left;
                int numSegs = (current - left) * 180 / (rollSurface * ROLLABLE_SEGMENT_DEGREE);
                for (int j = 0; j < numSegs; j++) {
                    y += rollSurface * ROLLABLE_SEGMENT_DEGREE / 180;
                    hingeParam.push_back({ 0, y, displayW, 0, 0, 0.0f, 360.0f, -ROLLABLE_SEGMENT_DEGREE });
                }
                if (mCenterIndex < 0) {
                    mCenterIndex = numSegs * 2;
                }
            } else {
                // 180 degree roll
                hingeParam.push_back({ 0, current - rollSurface, displayW, 0, 0, 0.0f, 360.0f, -ROLLABLE_SEGMENT_DEGREE });
                int y = current - rollSurface;
                int numSegs = 180 / ROLLABLE_SEGMENT_DEGREE;
                for (int j = 0; j < numSegs; j++) {
                    y += rollSurface * ROLLABLE_SEGMENT_DEGREE / 180;
                    hingeParam.push_back({ 0, y, displayW, 0, 0, 0.0f, 360.0f, -ROLLABLE_SEGMENT_DEGREE });
                }
                if (mCenterIndex < 0) {
                    mCenterIndex = numSegs * 2 + 1;
                }
            }
        } else if (direction == 1) {
            if (mCenterIndex < 0) {
                mCenterIndex = 0;
            }
            if (current + rollSurface > right) {
                // not a 180 degree roll
                int y = current;
                int numSegs = (right - current) * 180 / (rollSurface * ROLLABLE_SEGMENT_DEGREE);
                for (int j = 0; j < numSegs; j++) {
                    hingeParam.push_back({ 0, y, displayW, 0, 0, 0.0f, 360.0f, -ROLLABLE_SEGMENT_DEGREE });
                    y += rollSurface * ROLLABLE_SEGMENT_DEGREE / 180;
                }
            } else {
                // 180 degree roll
                int y = current;
                int numSegs = 180 / ROLLABLE_SEGMENT_DEGREE;
                for (int j = 0; j < numSegs; j++) {
                    hingeParam.push_back({ 0, y, displayW, 0, 0, 0.0f, 360.0f, -ROLLABLE_SEGMENT_DEGREE });
                    y += rollSurface * ROLLABLE_SEGMENT_DEGREE / 180;
                }
                hingeParam.push_back({ 0, y, displayW, 0, 0, 0.0f, 360.0f, 0 });
            }
        } else {
            LOG(ERROR) << "please config rollable direction to be 0 or 1";
            continue;
        }
    }
    if (!hRoll) {
      // rotate 90 degree for vertical split, the result is horizontal split
      for (int i = 0; i < hingeParam.size(); i++) {
          std::swap(hingeParam[i].x, hingeParam[i].y);
          std::swap(hingeParam[i].width, hingeParam[i].height);
      }
    }

    return initAbstractDeviceHingeModel(hRoll ? ANDROID_FOLDABLE_HORIZONTAL_SPLIT : ANDROID_FOLDABLE_VERTICAL_SPLIT,
                                        hingeParam.size(), hingeParam.data());
}

bool Device3DWidget::initAbstractDeviceModel() {
    android_foldable_get_state(&mFoldableState);
    switch (mFoldableState.config.type)
    {
    case ANDROID_FOLDABLE_HORIZONTAL_SPLIT:
    case ANDROID_FOLDABLE_VERTICAL_SPLIT:
        return initAbstractDeviceHingeModel(mFoldableState.config.type,
                                            mFoldableState.config.numHinges,
                                            mFoldableState.config.hingeParams);
    case ANDROID_FOLDABLE_HORIZONTAL_ROLL:
    case ANDROID_FOLDABLE_VERTICAL_ROLL:
        return initAbstractDeviceRollModel();
    default:
        qCWarning(emu, "invalid foldable type %d", mFoldableState.config.type);
    }
    return false;
}

static bool getVisibleArea(const QPixmap* pixMap, QRect& rect) {
    if (!pixMap)
        return false;

    const QBitmap bitMap = pixMap->mask();
    QImage bitImage = bitMap.toImage();
    int topVisible = -1;
    for (int row = 0; row < bitImage.height() && topVisible < 0; row++) {
        for (int col = 0; col < bitImage.width(); col++) {
            if (bitImage.pixelColor(col, row) != Qt::color0) {
                topVisible = row;
                break;
            }
        }
    }
    if (topVisible < 0) {
        return false;
    }
    int bottomVisible = -1;
    for (int row = bitImage.height() - 1; row >= 0 && bottomVisible < 0;
         row--) {
        for (int col = 0; col < bitImage.width(); col++) {
            if (bitImage.pixelColor(col, row) != Qt::color0) {
                bottomVisible = row;
                break;
            }
        }
    }
    if (bottomVisible < 0) {
        return false;
    }
    int leftVisible = -1;
    for (int col = 0; col < bitImage.width() && leftVisible < 0; col++) {
        for (int row = 0; row < bitImage.height(); row++) {
            if (bitImage.pixelColor(col, row) != Qt::color0) {
                leftVisible = col;
                break;
            }
        }
    }
    if (leftVisible < 0) {
        return false;
    }
    int rightVisible = -1;
    for (int col = bitImage.width() - 1; col >= 0 && rightVisible < 0; col--) {
        for (int row = 0; row < bitImage.height(); row++) {
            if (bitImage.pixelColor(col, row) != Qt::color0) {
                rightVisible = col;
                break;
            }
        }
    }
    if (rightVisible < 0) {
        return false;
    }
    rect.setRect(leftVisible, topVisible, rightVisible - leftVisible,
                 bottomVisible - topVisible);
    return true;
}

GLuint Device3DWidget::createSkinTexture() {
    const QPixmap* pixMap = EmulatorQtWindow::getInstance()->getRawSkinPixmap();
    if (pixMap != nullptr) {
        QRect rect;
        if (getVisibleArea(pixMap, rect)) {
            QImage image = pixMap->copy(rect).toImage();
            GLuint texture = 0;
            mGLES2->glGenTextures(1, &texture);
            mGLES2->glBindTexture(GL_TEXTURE_2D, texture);
            mGLES2->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(),
                                 image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                                 image.bits());
            mGLES2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                    GL_LINEAR);
            mGLES2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                    GL_LINEAR);
            mGLES2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                    GL_CLAMP_TO_EDGE);
            mGLES2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                    GL_CLAMP_TO_EDGE);
            return texture;
        }
    }
    return create2DTexture(mGLES2, nullptr, GL_TEXTURE_2D, GL_RGBA);
}

bool Device3DWidget::initTextures() {
    if (mUseAbstractDevice) {
        mSpecularMap = create2DTexture(mGLES2, nullptr, GL_TEXTURE_2D, GL_RGBA,
                                       GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE,
                                       GL_CLAMP_TO_EDGE, 0xff2f2f2f);
        mDiffuseMap = create2DTexture(mGLES2, nullptr, GL_TEXTURE_2D, GL_RGBA,
                                      GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE,
                                      GL_CLAMP_TO_EDGE, 0xff2f2f2f);
        if (!mSpecularMap || !mDiffuseMap) {
            return false;
        }
    } else {
        // Create textures.
        mDiffuseMap = create2DTexture(mGLES2, ":/phone-model/diffuse-map.png",
                                      GL_TEXTURE_2D, GL_RGBA,
                                      GL_LINEAR_MIPMAP_LINEAR);
        mSpecularMap = create2DTexture(mGLES2, ":/phone-model/specular-map.png",
                                       GL_TEXTURE_2D, GL_LUMINANCE);
        mGlossMap = create2DTexture(mGLES2, ":/phone-model/gloss-map.png",
                                    GL_TEXTURE_2D, GL_LUMINANCE);

        if (!mDiffuseMap || !mSpecularMap || !mGlossMap) {
            return false;
        }
        mGLES2->glGenTextures(1, &mEnvMap);
        mGLES2->glBindTexture(GL_TEXTURE_CUBE_MAP, mEnvMap);
        loadTexture(mGLES2, ":/phone-model/env-map-front.png",
                    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_RGBA);
        loadTexture(mGLES2, ":/phone-model/env-map-back.png",
                    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, GL_RGBA);
        loadTexture(mGLES2, ":/phone-model/env-map-right.png",
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_RGBA);
        loadTexture(mGLES2, ":/phone-model/env-map-left.png",
                    GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_RGBA);
        loadTexture(mGLES2, ":/phone-model/env-map-top.png",
                    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_RGBA);
        loadTexture(mGLES2, ":/phone-model/env-map-bottom.png",
                    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_RGBA);
        mGLES2->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR_MIPMAP_LINEAR);
        mGLES2->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR);
        mGLES2->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
                                GL_CLAMP_TO_EDGE);
        mGLES2->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
                                GL_CLAMP_TO_EDGE);
        mGLES2->glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        CHECK_GL_ERROR_RETURN("Failed to initialize cubemap", false);
    }
    return true;
}

void Device3DWidget::resizeGL(int w, int h) {
    if (!mGLES2) {
        return;
    }

    // Adjust for new viewport
    mGLES2->glViewport(0, 0, w, h);

    float aspect_ratio = static_cast<float>(w) / static_cast<float>(h);
    float min_move_aspect_ratio = (MaxX - MinX) / (MaxY - MinY);
    float plane_width = aspect_ratio > min_move_aspect_ratio
                                ? (MaxY - MinY) * aspect_ratio
                                : MaxX - MinY;
    float plane_height = aspect_ratio > min_move_aspect_ratio
                                 ? MaxY - MinY
                                 : (MaxX - MinX) / min_move_aspect_ratio;

    // Moving the phone should not clip the near/far planes.  Note that the
    // camera is looking in the -z direction so MinZ is the farthest the phone
    // model can move from the camera.
    assert(NearClip < CameraDistance - MaxZ);
    assert(FarClip > CameraDistance - MinZ);

    // Recompute the perspective projection transform.
    mPerspective =
            glm::frustum(-plane_width * NearClip / (2.0f * CameraDistance),
                         plane_width * NearClip / (2.0f * CameraDistance),
                         -plane_height * NearClip / (2.0f * CameraDistance),
                         plane_height * NearClip / (2.0f * CameraDistance),
                         NearClip, FarClip);
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

    if (mOperationMode == OperationMode::Rotate &&
        quaternionNearEqual(mLastTargetRotation, mTargetRotation)) {
        rotation = glm::mat4_cast(mLastTargetRotation);
    } else if (mOperationMode == OperationMode::Move &&
               vecNearEqual(mLastTargetPosition, mTargetPosition)) {
        position = mLastTargetPosition;
    }

    // Give the new MVP matrix to the shader.
    mGLES2->glUseProgram(mProgram);
    GLint mv_matrix_uniform =
            mGLES2->glGetUniformLocation(mProgram, "model_view");
    GLint mvit_matrix_uniform = mGLES2->glGetUniformLocation(
            mProgram, "model_view_inverse_transpose");
    GLint mvp_matrix_uniform =
            mGLES2->glGetUniformLocation(mProgram, "model_view_projection");
    GLint no_light_uniform = mGLES2->glGetUniformLocation(mProgram, "no_light");
    mGLES2->glUniform1f(no_light_uniform, 0.0f);

    if (mUseAbstractDevice) {
        glm::mat4 rotateLocal =
                (mFoldableState.config.type ==
                 ANDROID_FOLDABLE_HORIZONTAL_SPLIT ||
                 mFoldableState.config.type ==
                 ANDROID_FOLDABLE_HORIZONTAL_ROLL)
                        ? glm::mat4()
                        : glm::rotate(glm::mat4(), glm::radians(90.0f),
                                      glm::vec3(0.0, 0.0, 1.0));
        if (updateHingeAngles() || mFirstAbstractDeviceRepaint) {
            mFirstAbstractDeviceRepaint = false;
            if (android_foldable_hinge_configured()) {
                for (int32_t i = 0, j = 0; i < mDisplaySegments.size(); i++) {
                    if (mDisplaySegments[i].isHinge) {
                        mDisplaySegments[i].hingeAngle =
                                180 - mFoldableState.currentHingeDegrees[j++];
                    } else {
                        mDisplaySegments[i].hingeAngle = 0;
                    }
                }
            }
            // Calculate transform matrix for folding each segment
            for (int32_t i = mCenterIndex - 1; i >= 0; i--) {
                mDisplaySegments[i].rotate = mDisplaySegments[i + 1].rotate +
                                             mDisplaySegments[i].hingeAngle;
                if (mDisplaySegments[i].hingeAngle >= 0 &&
                    android_foldable_hinge_configured() ||
                    mDisplaySegments[i].hingeAngle < 0 &&
                    android_foldable_rollable_configured()) {
                    glm::vec4 v = mDisplaySegments[i + 1].hingeModelTransfrom *
                                  glm::vec4(0.0,
                                            (mDisplaySegments[i + 1].t -
                                             mDisplaySegments[i + 1].b) *
                                                    mFactor,
                                            0.0, 1.0);
                    glm::mat4 translate = glm::translate(
                            glm::mat4(), glm::vec3(v.x, v.y, v.z));
                    mDisplaySegments[i].hingeModelTransfrom = glm::rotate(
                            translate,
                            glm::radians(float(mDisplaySegments[i].rotate)),
                            glm::vec3(1.0, 0.0, 0.0));
                } else {
                    glm::vec4 v = mDisplaySegments[i + 1].hingeModelTransfrom *
                                  glm::vec4(0.0,
                                            (mDisplaySegments[i + 1].t -
                                             mDisplaySegments[i + 1].b) *
                                                    mFactor,
                                            -mDepth, 1.0);
                    glm::mat4 translate = glm::translate(
                            glm::mat4(), glm::vec3(v.x, v.y, v.z));
                    glm::mat4 rotate = glm::rotate(
                            translate,
                            glm::radians(float(mDisplaySegments[i].rotate)),
                            glm::vec3(1.0, 0.0, 0.0));
                    mDisplaySegments[i].hingeModelTransfrom =
                            glm::translate(rotate, glm::vec3(0.0, 0.0, mDepth));
                }
            }

            for (int32_t i = mCenterIndex + 1; i < mDisplaySegments.size();
                 i++) {
                mDisplaySegments[i].rotate = mDisplaySegments[i - 1].rotate +
                                             mDisplaySegments[i].hingeAngle;
                if (mDisplaySegments[i].hingeAngle >= 0) {
                    glm::vec4 v =
                            (i == mCenterIndex + 1)
                                    ? mDisplaySegments[i - 1]
                                                      .hingeModelTransfrom *
                                              glm::vec4(0.0, 0.0, 0.0, 1.0)
                                    : mDisplaySegments[i - 1]
                                                      .hingeModelTransfrom *
                                              glm::vec4(0.0,
                                                        (mDisplaySegments[i - 1]
                                                                 .b -
                                                         mDisplaySegments[i - 1]
                                                                 .t) *
                                                                mFactor,
                                                        0.0, 1.0);
                    glm::mat4 translate = glm::translate(
                            glm::mat4(), glm::vec3(v.x, v.y, v.z));
                    mDisplaySegments[i].hingeModelTransfrom = glm::rotate(
                            translate,
                            glm::radians(float(mDisplaySegments[i].rotate)),
                            glm::vec3(-1.0, 0.0, 0.0));
                } else {
                    glm::vec4 v =
                            (i == mCenterIndex + 1)
                                    ? mDisplaySegments[i - 1]
                                                      .hingeModelTransfrom *
                                              glm::vec4(0.0, 0.0, -mDepth, 1.0)
                                    : mDisplaySegments[i - 1]
                                                      .hingeModelTransfrom *
                                              glm::vec4(0.0,
                                                        (mDisplaySegments[i - 1]
                                                                 .b -
                                                         mDisplaySegments[i - 1]
                                                                 .t) *
                                                                mFactor,
                                                        -mDepth, 1.0);
                    glm::mat4 translate = glm::translate(
                            glm::mat4(), glm::vec3(v.x, v.y, v.z));
                    glm::mat4 rotate = glm::rotate(
                            translate,
                            glm::radians(float(mDisplaySegments[i].rotate)),
                            glm::vec3(-1.0, 0.0, 0.0));
                    mDisplaySegments[i].hingeModelTransfrom =
                            glm::translate(rotate, glm::vec3(0.0, 0.0, mDepth));
                }
            }
        }
        mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mVertexDataBuffer);
        mGLES2->glEnableVertexAttribArray(mVertexPositionAttribute);
        mGLES2->glEnableVertexAttribArray(mVertexNormalAttribute);
        mGLES2->glEnableVertexAttribArray(mVertexUVAttribute);
        mGLES2->glVertexAttribPointer(mVertexPositionAttribute, 3, GL_FLOAT,
                                      GL_FALSE, sizeof(float) * 8, 0);
        mGLES2->glVertexAttribPointer(mVertexNormalAttribute, 3, GL_FLOAT,
                                      GL_FALSE, sizeof(float) * 8,
                                      (void*)(sizeof(float) * 3));
        mGLES2->glVertexAttribPointer(mVertexUVAttribute, 2, GL_FLOAT, GL_FALSE,
                                      sizeof(float) * 8,
                                      (void*)(sizeof(float) * 6));
        mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexIndexBuffer);
        mGLES2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GLuint diffuse_map_uniform =
                mGLES2->glGetUniformLocation(mProgram, "diffuse_map");
        mGLES2->glUniform1i(diffuse_map_uniform, 0);
        FoldablePostures posture = POSTURE_UNKNOWN;
        if (mSensorsAgent) {
            glm::vec3 result;
            mSensorsAgent->getPhysicalParameter(PHYSICAL_PARAMETER_POSTURE,
                                                &result.x, &result.y, &result.z,
                                                PARAMETER_VALUE_TYPE_CURRENT);
            posture = (FoldablePostures)result.x;
        }

        uint32_t i = 0;
        // Draw 5 non-front sides of each display segment cuboid
        mGLES2->glActiveTexture(GL_TEXTURE0);
        mGLES2->glBindTexture(GL_TEXTURE_2D, mSpecularMap);
        i = 0;
        for (auto iter : mDisplaySegments) {
            if (iter.t == iter.b) {
                continue;
            }
            const glm::mat4 model_transform =
                    glm::translate(glm::mat4(), position) * rotation *
                    rotateLocal * iter.hingeModelTransfrom;
            const glm::mat4 model_view_transform =
                    mCameraTransform * model_transform;
            glm::mat4 normal_transform =
                    glm::transpose(glm::inverse(model_transform));
            glm::mat4 model_view_projection =
                    mPerspective * model_view_transform;
            mGLES2->glUniformMatrix4fv(mv_matrix_uniform, 1, GL_FALSE,
                                       &model_transform[0][0]);
            mGLES2->glUniformMatrix4fv(mvit_matrix_uniform, 1, GL_FALSE,
                                       &normal_transform[0][0]);
            mGLES2->glUniformMatrix4fv(mvp_matrix_uniform, 1, GL_FALSE,
                                       &model_view_projection[0][0]);
            if (iter.isHinge) {
                mGLES2->glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT,
                                       (void*)(i * 36 * sizeof(GLuint)));
            } else {
                if (android_foldable_folded_area_configured(0) &&
                    !android_foldable_rollable_configured()){
                    if (posture == POSTURE_CLOSED) {
                        // legacy foldable configured, display moves to
                        // secondary smaller screen on the back of device
                        mGLES2->glDrawElements(
                                GL_TRIANGLES, 6, GL_UNSIGNED_INT,
                                (void*)((i * 36) * sizeof(GLuint)));
                        mGLES2->glDrawElements(
                                GL_TRIANGLES, 24, GL_UNSIGNED_INT,
                                (void*)((i * 36 + 12) * sizeof(GLuint)));
                    } else {
                        mGLES2->glDrawElements(
                                GL_TRIANGLES, 30, GL_UNSIGNED_INT,
                                (void*)((i * 36 + 6) * sizeof(GLuint)));
                    }
                } else {
                    mGLES2->glDrawElements(
                            GL_TRIANGLES, 30, GL_UNSIGNED_INT,
                            (void*)((i * 36 + 6) * sizeof(GLuint)));
                }
            }
            i++;
        }

        // Draw front side (android display) of each display segment
        mGLES2->glActiveTexture(GL_TEXTURE0);
        mGLES2->glBindTexture(GL_TEXTURE_2D, mDiffuseMap);
        struct AndroidVirtioGpuOps* ops = android_getVirtioGpuOps();
        ops->bind_color_buffer_to_texture(ops->get_last_posted_color_buffer());
        i = 0;
        for (auto iter : mDisplaySegments) {
            if (iter.t == iter.b) {
                continue;
            }
            if (iter.isHinge) {
                i++;
                continue;
            }
            const glm::mat4 model_transform =
                    glm::translate(glm::mat4(), position) * rotation *
                    rotateLocal * iter.hingeModelTransfrom;
            const glm::mat4 model_view_transform =
                    mCameraTransform * model_transform;
            glm::mat4 normal_transform =
                    glm::transpose(glm::inverse(model_transform));
            glm::mat4 model_view_projection =
                    mPerspective * model_view_transform;
            mGLES2->glUniformMatrix4fv(mv_matrix_uniform, 1, GL_FALSE,
                                       &model_transform[0][0]);
            mGLES2->glUniformMatrix4fv(mvit_matrix_uniform, 1, GL_FALSE,
                                       &normal_transform[0][0]);
            mGLES2->glUniformMatrix4fv(mvp_matrix_uniform, 1, GL_FALSE,
                                       &model_view_projection[0][0]);
            if (android_foldable_folded_area_configured(0) &&
                !android_foldable_rollable_configured()) {
                if (posture == POSTURE_CLOSED) {
                    mGLES2->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
                                           (void*)((i * 36 + 6) * sizeof(GLuint)));
                } else {
                    mGLES2->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
                                           (void*)(i * 36 * sizeof(GLuint)));
                }
            } else {
                mGLES2->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
                                       (void*)(i * 36 * sizeof(GLuint)));
            }
            i++;
        }
    } else {
        // Recompute the model transformation matrix using the given rotations.
        const glm::mat4 model_transform =
                glm::translate(glm::mat4(), position) * rotation;
        const glm::mat4 model_view_transform =
                mCameraTransform * model_transform;

        // Normals need to be transformed using the inverse transpose of
        // modelview matrix.
        glm::mat4 normal_transform =
                glm::transpose(glm::inverse(model_view_transform));

        // Recompute the model-view-projection matrix using the new model
        // transform.
        glm::mat4 model_view_projection = mPerspective * model_view_transform;

        // Upload matrices.
        GLuint mv_matrix_uniform =
                mGLES2->glGetUniformLocation(mProgram, "model_view");
        mGLES2->glUniformMatrix4fv(mv_matrix_uniform, 1, GL_FALSE,
                                   &model_view_transform[0][0]);

        GLuint mvit_matrix_uniform = mGLES2->glGetUniformLocation(
                mProgram, "model_view_inverse_transpose");
        mGLES2->glUniformMatrix4fv(mvit_matrix_uniform, 1, GL_FALSE,
                                   &normal_transform[0][0]);

        GLuint mvp_matrix_uniform =
                mGLES2->glGetUniformLocation(mProgram, "model_view_projection");
        mGLES2->glUniformMatrix4fv(mvp_matrix_uniform, 1, GL_FALSE,
                                   &model_view_projection[0][0]);

        // Set textures.
        mGLES2->glActiveTexture(GL_TEXTURE0);
        mGLES2->glBindTexture(GL_TEXTURE_2D, mDiffuseMap);
        mGLES2->glActiveTexture(GL_TEXTURE1);
        mGLES2->glBindTexture(GL_TEXTURE_2D, mSpecularMap);
        mGLES2->glActiveTexture(GL_TEXTURE2);
        mGLES2->glBindTexture(GL_TEXTURE_2D, mGlossMap);
        mGLES2->glActiveTexture(GL_TEXTURE3);
        mGLES2->glBindTexture(GL_TEXTURE_CUBE_MAP, mEnvMap);

        GLuint diffuse_map_uniform =
                mGLES2->glGetUniformLocation(mProgram, "diffuse_map");
        mGLES2->glUniform1i(diffuse_map_uniform, 0);
        GLuint specular_map_uniform =
                mGLES2->glGetUniformLocation(mProgram, "specular_map");
        mGLES2->glUniform1i(specular_map_uniform, 1);
        GLuint gloss_map_uniform =
                mGLES2->glGetUniformLocation(mProgram, "gloss_map");
        mGLES2->glUniform1i(gloss_map_uniform, 2);
        GLuint env_map_uniform =
                mGLES2->glGetUniformLocation(mProgram, "env_map");
        mGLES2->glUniform1i(env_map_uniform, 3);
        GLuint no_light_uniform =
                mGLES2->glGetUniformLocation(mProgram, "no_light");

        // Set up attribute pointers.
        mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mVertexDataBuffer);
        mGLES2->glEnableVertexAttribArray(mVertexPositionAttribute);
        mGLES2->glEnableVertexAttribArray(mVertexNormalAttribute);
        mGLES2->glEnableVertexAttribArray(mVertexUVAttribute);
        mGLES2->glVertexAttribPointer(mVertexPositionAttribute, 3, GL_FLOAT,
                                      GL_FALSE, sizeof(float) * 8, 0);
        mGLES2->glVertexAttribPointer(mVertexNormalAttribute, 3, GL_FLOAT,
                                      GL_FALSE, sizeof(float) * 8,
                                      (void*)(sizeof(float) * 3));
        mGLES2->glVertexAttribPointer(mVertexUVAttribute, 2, GL_FLOAT, GL_FALSE,
                                      sizeof(float) * 8,
                                      (void*)(sizeof(float) * 6));
        mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexIndexBuffer);

        // Draw the model.
        mGLES2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        mGLES2->glDrawElements(GL_TRIANGLES, mElementsCount, GL_UNSIGNED_INT,
                               0);
    }
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
    const glm::vec4 screenSpace(
            (2.0f * x / static_cast<float>(width())) - 1.0f,
            1.0f - (2.0f * y / static_cast<float>(height())), -1.f, 1.f);

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

constexpr float kHingeAngleDelta = 1.0f;

bool Device3DWidget::updateHingeAngles() {
    bool ret = false;
    if (!mSensorsAgent) {
        return ret;
    }
    if (android_foldable_rollable_configured()) {
        glm::vec3 result0, result1;
        mSensorsAgent->getPhysicalParameter(
                        PHYSICAL_PARAMETER_ROLLABLE0, &result0.x, &result0.y,
                        &result0.z, PARAMETER_VALUE_TYPE_CURRENT);
        mSensorsAgent->getPhysicalParameter(
                        PHYSICAL_PARAMETER_ROLLABLE1, &result1.x, &result1.y,
                        &result1.z, PARAMETER_VALUE_TYPE_CURRENT);
        if (abs(mFoldableState.currentRolledPercent[0] - result0.x) >
                kHingeAngleDelta ||
            abs(mFoldableState.currentRolledPercent[1] - result1.x) >
                kHingeAngleDelta) {
            mFoldableState.currentRolledPercent[0] = result0.x;
            mFoldableState.currentRolledPercent[1] = result1.x;
            ret = true;
        }
        if (ret) {
            ret = initAbstractDeviceRollModel();
        }
    } else if (android_foldable_hinge_configured()) {
        glm::vec3 result;
        switch (mFoldableState.config.numHinges) {
            case 3:
                mSensorsAgent->getPhysicalParameter(
                        PHYSICAL_PARAMETER_HINGE_ANGLE2, &result.x, &result.y,
                        &result.z, PARAMETER_VALUE_TYPE_CURRENT);
                if (abs(mFoldableState.currentHingeDegrees[2] - result.x) >
                    kHingeAngleDelta) {
                    mFoldableState.currentHingeDegrees[2] = result.x;
                    ret = true;
                }
            case 2:
                mSensorsAgent->getPhysicalParameter(
                        PHYSICAL_PARAMETER_HINGE_ANGLE1, &result.x, &result.y,
                        &result.z, PARAMETER_VALUE_TYPE_CURRENT);
                if (abs(mFoldableState.currentHingeDegrees[1] - result.x) >
                    kHingeAngleDelta) {
                    mFoldableState.currentHingeDegrees[1] = result.x;
                    ret = true;
                }
            case 1:
                mSensorsAgent->getPhysicalParameter(
                        PHYSICAL_PARAMETER_HINGE_ANGLE0, &result.x, &result.y,
                        &result.z, PARAMETER_VALUE_TYPE_CURRENT);
                if (abs(mFoldableState.currentHingeDegrees[0] - result.x) >
                    kHingeAngleDelta) {
                    mFoldableState.currentHingeDegrees[0] = result.x;
                    ret = true;
                }
            default:;
        }
    }
    return ret;
}

void Device3DWidget::showEvent(QShowEvent* event) {
    GLWidget::showEvent(event);
    if (mUseAbstractDevice && !mAnimationTimer.isActive()) {
        mAnimationTimer.start();
    }
}

void Device3DWidget::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    if (mUseAbstractDevice && mAnimationTimer.isActive()) {
        mAnimationTimer.stop();
    }
}
