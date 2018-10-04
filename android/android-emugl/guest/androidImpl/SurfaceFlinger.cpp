// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "SurfaceFlinger.h"


namespace aemu {

SurfaceFlinger::SurfaceFlinger(int width, int height) {

}


} // namespace aemu

// class SurfaceFlinger {
// public:
//     SurfaceFlinger() = default;
// 
//     void setComposeWindow(ANativeWindow* w) {
//         mWindow = w;
//     }
// 
//     void setAppProducer(BQ* app2sf, BQ* sf2App) {
//         AutoLock lock(mLock);
//         mApp2SfQueue = app2sf;
//         mSf2AppQueue = sf2App;
// 
//         if (!mThread) {
//             composeThread();
//         }
//     }
// 
//     void composeThread() {
//         mThread = new FunctorThread([this]() {
//             auto& egl = emugl::loadGoldfishOpenGL()->egl;
//             auto& gl = emugl::loadGoldfishOpenGL()->gles2;
// 
//             EGLDisplay d = egl.eglGetDisplay(EGL_DEFAULT_DISPLAY);
//             EGLint maj, min;
//             egl.eglInitialize(d, &maj, &min);
// 
//             egl.eglBindAPI(EGL_OPENGL_ES_API);
//             EGLint total_num_configs = 0;
//             egl.eglGetConfigs(d, 0, 0, &total_num_configs);
// 
//             EGLint wantedRedSize = 8;
//             EGLint wantedGreenSize = 8;
//             EGLint wantedBlueSize = 8;
// 
//             const GLint configAttribs[] = {
//                     EGL_RED_SIZE,       wantedRedSize,
//                     EGL_GREEN_SIZE, wantedGreenSize,    EGL_BLUE_SIZE, wantedBlueSize,
//                     EGL_SURFACE_TYPE,   EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
//                     EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE};
//             EGLint total_compatible = 0;
// 
//             std::vector<EGLConfig> configs(total_num_configs);
//             egl.eglChooseConfig(d, configAttribs, configs.data(), total_num_configs, &total_compatible);
// 
//             EGLint exact_match_index = -1;
//             for (EGLint i = 0; i < total_compatible; i++) {
//                 EGLint r, g, b;
//                 EGLConfig c = configs[i];
//                 egl.eglGetConfigAttrib(d, c, EGL_RED_SIZE, &r);
//                 egl.eglGetConfigAttrib(d, c, EGL_GREEN_SIZE, &g);
//                 egl.eglGetConfigAttrib(d, c, EGL_BLUE_SIZE, &b);
// 
//                 if (r == wantedRedSize && g == wantedGreenSize && b == wantedBlueSize) {
//                     exact_match_index = i;
//                     break;
//                 }
//             }
// 
//             EGLConfig match = configs[exact_match_index];
// 
//             static const GLint glesAtt[] =
//             { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
// 
//             EGLContext c = egl.eglCreateContext(d, match, EGL_NO_CONTEXT, glesAtt);
// 
//             assert(mWindow);
// 
//             EGLSurface s = egl.eglCreateWindowSurface(d, match, mWindow, 0);
//             
//             egl.eglMakeCurrent(d, s, s, c);
// 
//             fprintf(stderr, "%s: extensionStr %s\n", __func__, gl.glGetString(GL_EXTENSIONS));
// 
//             static constexpr char blitVshaderSrc[] = R"(#version 300 es
//             precision highp float;
//             layout (location = 0) in vec2 pos;
//             layout (location = 1) in vec2 texcoord;
//             out vec2 texcoord_varying;
//             void main() {
//                 gl_Position = vec4(pos, 0.0, 1.0);
//                 texcoord_varying = texcoord;
//             })";
// 
//             static constexpr char blitFshaderSrc[] = R"(#version 300 es
//             precision highp float;
//             uniform sampler2D tex;
//             in vec2 texcoord_varying;
//             out vec4 fragColor;
//             void main() {
//                 fragColor = texture(tex, texcoord_varying);
//             })";
// 
//             GLint blitProgram =
//                 compileAndLinkShaderProgram(
//                     blitVshaderSrc, blitFshaderSrc);
// 
//             GLint samplerLoc = gl.glGetUniformLocation(blitProgram, "tex");
// 
//             GLuint blitVbo;
//             gl.glGenBuffers(1, &blitVbo);
//             gl.glBindBuffer(GL_ARRAY_BUFFER, blitVbo);
//             const float attrs[] = {
//                 -1.0f, -1.0f, 0.0f, 1.0f,
//                 1.0f, -1.0f, 1.0f, 1.0f,
//                 1.0f, 1.0f, 1.0f, 0.0f,
//                 -1.0f, -1.0f, 0.0f, 1.0f,
//                 1.0f, 1.0f, 1.0f, 0.0f,
//                 -1.0f, 1.0f, 0.0f, 0.0f,
//             };
//             gl.glBufferData(GL_ARRAY_BUFFER, sizeof(attrs), attrs, GL_STATIC_DRAW);
//             gl.glEnableVertexAttribArray(0);
//             gl.glEnableVertexAttribArray(1);
// 
//             gl.glVertexAttribPointer(
//                 0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
//             gl.glVertexAttribPointer(
//                 1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
//                 (GLvoid*)(uintptr_t)(2 * sizeof(GLfloat)));
// 
//             GLuint blitTexture;
//             gl.glActiveTexture(GL_TEXTURE0);
//             gl.glGenTextures(1, &blitTexture);
//             gl.glBindTexture(GL_TEXTURE_2D, blitTexture);
// 
//             gl.glUseProgram(blitProgram);
//             gl.glUniform1i(samplerLoc, 0);
// 
//             BQ::Item appItem = {};
// 
//             while (true) {
//                 AutoLock lock(mLock);
// 
//                 mApp2SfQueue->dequeueBuffer(&appItem);
// 
//                 EGLImageKHR image = getEGLImage(appItem.buffer);
// 
//                 gl.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
// 
//                 gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
// 
//                 gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//                 gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
// 
//                 gl.glDrawArrays(GL_TRIANGLES, 0, 6);
//                 mSf2AppQueue->queueBuffer(BQ::Item(appItem.buffer, -1));
// 
//                 egl.eglSwapBuffers(d, s);
//             }
//         });
// 
//         mThread->start();
//     }
// 
// private:
// 
//     EGLImageKHR getEGLImage(ANativeWindowBuffer* buffer) {
//         auto it = mEGLImages.find(buffer);
//         if (it != mEGLImages.end()) return it->second;
// 
//         auto& egl = emugl::loadGoldfishOpenGL()->egl;
//         EGLDisplay d = egl.eglGetDisplay(EGL_DEFAULT_DISPLAY);
//         EGLImageKHR image = egl.eglCreateImageKHR(d, 0, EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)buffer, 0);
// 
//         mEGLImages[buffer] = image;
// 
//         return image;
//     }
// 
//     GLuint compileShader(GLenum shaderType, const char* src) {
//         auto& gl = emugl::loadGoldfishOpenGL()->gles2;
//    
//         GLuint shader = gl.glCreateShader(shaderType);
//         gl.glShaderSource(shader, 1, (const GLchar* const*)&src, nullptr);
//         gl.glCompileShader(shader);
//     
//         GLint compileStatus;
//         gl.glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
//     
//         if (compileStatus != GL_TRUE) {
//             GLsizei infoLogLength = 0;
//             gl.glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
//             std::vector<char> infoLog(infoLogLength + 1, 0);
//             gl.glGetShaderInfoLog(shader, infoLogLength, nullptr, &infoLog[0]);
//             fprintf(stderr, "%s: fail to compile. infolog %s\n", __func__, &infoLog[0]);
//                 fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
//         }
//     
//         return shader;
//     }
//     
//     GLint compileAndLinkShaderProgram(const char* vshaderSrc, const char* fshaderSrc) {
//         auto& gl = emugl::loadGoldfishOpenGL()->gles2;
//     
//         GLuint vshader = compileShader(GL_VERTEX_SHADER, vshaderSrc);
//         GLuint fshader = compileShader(GL_FRAGMENT_SHADER, fshaderSrc);
//     
//         GLuint program = gl.glCreateProgram();
//         gl.glAttachShader(program, vshader);
//         gl.glAttachShader(program, fshader);
//         gl.glLinkProgram(program);
//     
//         GLint linkStatus;
//         gl.glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
//     
//         gl.glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
//     
//         if (linkStatus != GL_TRUE) {
//             GLsizei infoLogLength = 0;
//             gl.glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
//             std::vector<char> infoLog(infoLogLength + 1, 0);
//             gl.glGetProgramInfoLog(program, infoLogLength, nullptr, &infoLog[0]);
//     
//             fprintf(stderr, "%s: fail to link program. infolog: %s\n", __func__,
//                     &infoLog[0]);
//         }
//     
//         return program;
//     }
// 
//     BQ* mApp2SfQueue = nullptr;
//     BQ* mSf2AppQueue = nullptr;
// 
//     ANativeWindow* mWindow = nullptr;
// 
//     std::unordered_map<ANativeWindowBuffer*, EGLImageKHR> mEGLImages;
//     FunctorThread* mThread = nullptr;
// 
//     Lock mLock;
// };