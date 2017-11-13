// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/system/System.h"
#include "android/skin/qt/accelerometer-3d-widget.h"
#include "android/skin/qt/gl-common.h"
#include "android/skin/qt/wavefront-obj-parser.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "android/utils/debug.h"

Accelerometer3DWidget::Accelerometer3DWidget(QWidget* parent) :
    GLWidget(parent),
    mTracking(false),
    mOperationMode(OperationMode::Rotate) {
    toggleAA();
}

Accelerometer3DWidget::~Accelerometer3DWidget() {
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
        qWarning("Failed to load image \"%s\"", source_filename);
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
    if(!loadTexture(gles2,
                    source_filename,
                    target,
                    format)) return 0;
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

bool Accelerometer3DWidget::initGL() {
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
    mGLES2->glClearColor(0.5, 0.5, 0.5  , 1.0);

    // Set up the camera transformation.
    mCameraTransform = glm::lookAt(
        glm::vec3(0.0f, 0.0f, CameraDistance), // Camera position
        glm::vec3(0.0f, 0.0f, 0.0f), // Point at which the camera is looking
        glm::vec3(0.0f, 1.0f, 0.0f)); // "up" vector

    if (!initProgram() ||
        !initModel() ||
        !initTextures()) {
        return false;
    }

    resizeGL(width(), height());
    return true;
}

bool Accelerometer3DWidget::initProgram() {
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
        qWarning("Failed to link program");
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

bool Accelerometer3DWidget::initModel() {
    // Load the model and set up buffers.
    std::vector<float> model_vertex_data;
    std::vector<GLuint> indices;
    QFile model_file(":/phone-model/model.obj");
    if (model_file.open(QFile::ReadOnly)) {
        QTextStream file_stream(&model_file);
        if(!parseWavefrontOBJ(file_stream, model_vertex_data, indices)) {
            qWarning("Failed to load model");
            return false;
        }
    } else {
        qWarning("Failed to open model file for reading");
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

bool Accelerometer3DWidget::initTextures() {
    // Create textures.
    mDiffuseMap = create2DTexture(mGLES2,
                                  ":/phone-model/diffuse-map.png",
                                  GL_TEXTURE_2D,
                                  GL_RGBA,
                                  GL_LINEAR_MIPMAP_LINEAR);
    mSpecularMap = create2DTexture(mGLES2,
                                   ":/phone-model/specular-map.png",
                                   GL_TEXTURE_2D,
                                   GL_LUMINANCE);
    mGlossMap = create2DTexture(mGLES2,
                                ":/phone-model/gloss-map.png",
                                GL_TEXTURE_2D,
                                GL_LUMINANCE);
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

void Accelerometer3DWidget::resizeGL(int w, int h) {
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

    // Recompute the perspective projection transform.
    mPerspective = glm::frustum(
            -plane_width * NearClip / (2.0f * CameraDistance),
            plane_width * NearClip / (2.0f * CameraDistance),
            -plane_height * NearClip / (2.0f * CameraDistance),
            plane_height * NearClip / (2.0f * CameraDistance),
            NearClip,
            FarClip);
}

void Accelerometer3DWidget::repaintGL() {
    if (!mGLES2) {
        return;
    }

    // Handle repaint event.
    // Recompute the model transformation matrix using the given rotations.
    const glm::mat4 model_transform =
            glm::translate(glm::mat4(), glm::vec3(mTranslation, 0.0f)) * mRotation;
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

void Accelerometer3DWidget::mouseMoveEvent(QMouseEvent *event) {
    if (mTracking && mOperationMode == OperationMode::Rotate) {
        float diff_x = event->x() - mPrevMouseX,
              diff_y = event->y() - mPrevMouseY;
        const glm::quat q = glm::angleAxis(glm::radians(diff_y),
                                     glm::vec3(1.0f, 0.0f, 0.0f)) *
                      glm::angleAxis(glm::radians(diff_x),
                                     glm::vec3(0.0f, 1.0f, 0.0f));
        mRotation = glm::mat4_cast(q) * mRotation;
        renderFrame();
        emit(rotationChanged());
        mPrevMouseX = event->x();
        mPrevMouseY = event->y();
    } else if (mTracking && mOperationMode == OperationMode::Move) {
        glm::vec2 vec = screenToXYPlane(event->x(), event->y());
        mTranslation += vec - mPrevDragOrigin;
        mTranslation.x = clamp(MinX, MaxX, mTranslation.x);
        mTranslation.y = clamp(MinY, MaxY, mTranslation.y);
        renderFrame();
        emit(positionChanged());
        mPrevDragOrigin = vec;
    }
}

glm::vec2 Accelerometer3DWidget::screenToXYPlane(int x, int y) const {
    glm::vec4 vec((2.0f * x / static_cast<float>(width())) - 1.0f,
                  1.0f - (2.0f * y / static_cast<float>(height())),
                  0.0f,
                  1.0f);

    // Now, by applying the inverse perspective and camera transform,
    // turn vec into world coordinates.
    vec = glm::inverse(mPerspective * mCameraTransform) * vec;
    return glm::vec2(vec * (CameraDistance / NearClip));
}

void Accelerometer3DWidget::mousePressEvent(QMouseEvent *event) {
    mPrevMouseX = event->x();
    mPrevMouseY = event->y();
    mPrevDragOrigin = screenToXYPlane(event->x(), event->y());
    mTracking = true;
    if (mOperationMode == OperationMode::Move) {
        mDragging = true;
        emit(dragStarted());
    }
}

void Accelerometer3DWidget::mouseReleaseEvent(QMouseEvent* event) {
    mTracking = false;
    if (mDragging) {
        mDragging = false;
        emit(dragStopped());
    }
    emit(rotationChanged());
}
