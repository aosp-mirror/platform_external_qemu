/*
* Copyright (C) 2017 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

#include <GLES/gl.h>
#include <GLES/glext.h>

#include <glm/mat4x4.hpp>

#include <vector>
#include <unordered_map>
#include <unordered_set>

class GLEScmContext;
class GLESpointer;

class CoreProfileEngine {
public:
    CoreProfileEngine(GLEScmContext* ctx, bool onGles = false);
    ~CoreProfileEngine();

    struct GeometryDrawState {
        GLuint vshader;
        GLuint fshader;
        GLuint program;

        GLuint vshaderFlat;
        GLuint fshaderFlat;
        GLuint programFlat;

        GLuint ibo;
        GLuint vao;

        GLint projMatrixLoc;
        GLint modelviewMatrixLoc;
        GLint textureMatrixLoc;
        GLint modelviewInvTrLoc;
        GLint textureSamplerLoc;
        GLint textureCubeSamplerLoc;

        GLint enableTextureLoc;
        GLint enableLightingLoc;
        GLint enableRescaleNormalLoc;
        GLint enableNormalizeLoc;
        GLint enableColorMaterialLoc;
        GLint enableFogLoc;
        GLint enableReflectionMapLoc;

        GLint textureEnvModeLoc;
        GLint textureFormatLoc;

        GLint materialAmbientLoc;
        GLint materialDiffuseLoc;
        GLint materialSpecularLoc;
        GLint materialEmissiveLoc;
        GLint materialSpecularExponentLoc;

        GLint lightModelSceneAmbientLoc;
        GLint lightModelTwoSidedLoc;

        GLint lightEnablesLoc;
        GLint lightAmbientsLoc;
        GLint lightDiffusesLoc;
        GLint lightSpecularsLoc;
        GLint lightPositionsLoc;
        GLint lightDirectionsLoc;
        GLint lightSpotlightExponentsLoc;
        GLint lightSpotlightCutoffAnglesLoc;
        GLint lightAttenuationConstsLoc;
        GLint lightAttenuationLinearsLoc;
        GLint lightAttenuationQuadraticsLoc;

        GLint fogModeLoc;
        GLint fogDensityLoc;
        GLint fogStartLoc;
        GLint fogEndLoc;
        GLint fogColorLoc;

        GLuint posVbo;
        GLuint normalVbo;
        GLuint colorVbo;
        GLuint pointsizeVbo;
        GLuint texcoordVbo;
    };

    struct DrawTexOESCoreState {
        GLuint vshader;
        GLuint fshader;
        GLuint program;
        GLuint vbo;
        GLuint ibo;
        GLuint vao;
    };

    const DrawTexOESCoreState& getDrawTexOESCoreState();
    void teardown();
    const GeometryDrawState& getGeometryDrawState();

    GLint getAndClearLastError() {
        GLint err = mCurrError;
        mCurrError = 0;
        return err;
    }

    // Utility functions
    void setupArrayForDraw(GLenum arrayType, GLESpointer* p, GLint first, GLsizei count, bool isIndexed, GLenum indicesType, const GLvoid* indices);

    void preDrawTextureUnitEmulation();
    void postDrawTextureUnitEmulation();

    void preDrawVertexSetup();
    void postDrawVertexSetup();

    void setupLighting();
    void setupFog();

    // GLES 1 API (deprecated + incompatible with core only)

    void enable(GLenum cap);
    void disable(GLenum cap);

    void shadeModel(GLenum mode);

    void matrixMode(GLenum mode);
    void loadIdentity();
    void loadMatrixf(const GLfloat* m);
    void pushMatrix();
    void popMatrix();
    void multMatrixf(const GLfloat* m);

    void orthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
    void frustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);

    void texEnvf(GLenum target, GLenum pname, GLfloat param);
    void texEnvfv(GLenum target, GLenum pname, const GLfloat* params);
    void texEnvi(GLenum target, GLenum pname, GLint param);
    void texEnviv(GLenum target, GLenum pname, const GLint* params);
    void getTexEnvfv(GLenum env, GLenum pname, GLfloat* params);
    void getTexEnviv(GLenum env, GLenum pname, GLint* params);

    void texGenf(GLenum coord, GLenum pname, GLfloat param);
    void texGenfv(GLenum coord, GLenum pname, const GLfloat* params);
    void texGeni(GLenum coord, GLenum pname, GLint param);
    void texGeniv(GLenum coord, GLenum pname, const GLint* params);
    void getTexGeniv(GLenum coord, GLenum pname, GLint* params);
    void getTexGenfv(GLenum coord, GLenum pname, GLfloat* params);

    void enableClientState(GLenum clientState);
    void disableClientState(GLenum clientState);

    void drawTexOES(float x, float y, float z, float width, float height);

    void rotatef(float angle, float x, float y, float z);
    void scalef(float x, float y, float z);
    void translatef(float x, float y, float z);

    void color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    void color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);

    void activeTexture(GLenum unit);
    void clientActiveTexture(GLenum unit);
    void drawArrays(GLenum type, GLint first, GLsizei count);
    void drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);

private:
    GLEScmContext* mCtx = nullptr;

    GLint mCurrError = 0;
    void setError(GLint err) { mCurrError = err; }

    size_t sizeOfType(GLenum dataType);
    GLuint getVboFor(GLenum arrayType);

    DrawTexOESCoreState m_drawTexOESCoreState = {};
    GeometryDrawState   m_geometryDrawState = {};

    // If we are on a gles impl.
    bool mOnGles = false;

    static constexpr int kMaxLights = 8;
    struct LightingBuffer {
        GLint lightEnables[kMaxLights];
        GLfloat lightAmbients[4 * kMaxLights];
        GLfloat lightDiffuses[4 * kMaxLights];
        GLfloat lightSpeculars[4 * kMaxLights];
        GLfloat lightPositions[4 * kMaxLights];
        GLfloat lightDirections[3 * kMaxLights];
        GLfloat spotlightExponents[kMaxLights];
        GLfloat spotlightCutoffAngles[kMaxLights];
        GLfloat attenuationConsts[kMaxLights];
        GLfloat attenuationLinears[kMaxLights];
        GLfloat attenuationQuadratics[kMaxLights];
    };

    LightingBuffer m_lightingBuffer;
};
