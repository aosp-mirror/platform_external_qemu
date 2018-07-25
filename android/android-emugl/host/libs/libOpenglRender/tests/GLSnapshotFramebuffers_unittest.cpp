// Copyright (C) 2018 The Android Open Source Project
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

#include "GLSnapshotTesting.h"
#include "OpenglCodecCommon/glUtils.h"

#include <gtest/gtest.h>

#include <map>

namespace emugl {

struct GlFramebufferAttachment {
    GLenum type;
    GLuint name;
    GLenum textureLevel;
    GLenum textureCubeMapFace;
};

struct GlFramebufferObjectState {
    std::map<GLenum, GlFramebufferAttachment> attachments;
};

class SnapshotGlFramebufferObjectTest : public SnapshotPreserveTest {
public:
    void defaultStateCheck() override {
        EXPECT_EQ(GL_FALSE, gl->glIsFramebuffer(m_framebuffer_name));
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_FRAMEBUFFER_BINDING, 0));
    }

    void changedStateCheck() override {
        EXPECT_EQ(GL_TRUE, gl->glIsFramebuffer(m_framebuffer_name));
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_FRAMEBUFFER_BINDING,
                                       m_framebuffer_name));

        // don't lose current framebuffer binding
        GLint currentBind;
        gl->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentBind);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        for (auto& pair : m_state.attachments) {
            const GLenum& attachment = pair.first;
            GlFramebufferAttachment& expected = pair.second;

            GlFramebufferAttachment current = {};
            gl->glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer_name);
            gl->glGetFramebufferAttachmentParameteriv(
                    GL_FRAMEBUFFER, attachment,
                    GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                    (GLint*)&current.type);
            EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

            if (current.type != GL_NONE) {
                gl->glGetFramebufferAttachmentParameteriv(
                        GL_FRAMEBUFFER, attachment,
                        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                        (GLint*)&current.name);
                if (current.type == GL_TEXTURE) {
                    gl->glGetFramebufferAttachmentParameteriv(
                            GL_FRAMEBUFFER, attachment,
                            GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL,
                            (GLint*)&current.textureLevel);
                    gl->glGetFramebufferAttachmentParameteriv(
                            GL_FRAMEBUFFER, attachment,
                            GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE,
                            (GLint*)&current.textureCubeMapFace);
                }
            }

            EXPECT_EQ(expected.type, current.type);
            EXPECT_EQ(expected.name, current.name);
            EXPECT_EQ(expected.textureLevel, current.textureLevel);
            EXPECT_EQ(expected.textureCubeMapFace, current.textureCubeMapFace);
        }

        // restore framebuffer binding
        gl->glBindFramebuffer(GL_FRAMEBUFFER, currentBind);
    }

    void stateChange() override {
        gl->glGenFramebuffers(1, &m_framebuffer_name);
        gl->glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer_name);

        m_state_changer();
    }

    void setStateChanger(std::function<void()> changer) {
        m_state_changer = changer;
    }

protected:
    GLuint m_framebuffer_name = 0;
    GlFramebufferObjectState m_state = {};
    std::function<void()> m_state_changer = [] {};
};

TEST_F(SnapshotGlFramebufferObjectTest, CreateAndBind) {
    doCheckedSnapshot();
}

TEST_F(SnapshotGlFramebufferObjectTest, BindDepthRenderbuffer) {
    setStateChanger([this] {
        GLuint renderbuffer;
        gl->glGenRenderbuffers(1, &renderbuffer);
        gl->glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        gl->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                      GL_RENDERBUFFER, renderbuffer);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        m_state.attachments[GL_DEPTH_ATTACHMENT] = {GL_RENDERBUFFER,
                                                    renderbuffer, 0, 0};
    });
    doCheckedSnapshot();
}

TEST_F(SnapshotGlFramebufferObjectTest, BindStencilTextureCubeFace) {
    setStateChanger([this] {
        GLuint texture;
        gl->glGenTextures(1, &texture);
        gl->glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
        gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                   GL_TEXTURE_CUBE_MAP_NEGATIVE_X, texture, 0);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        m_state.attachments[GL_STENCIL_ATTACHMENT] = {
                GL_TEXTURE, texture, 0, GL_TEXTURE_CUBE_MAP_NEGATIVE_X};
    });
    doCheckedSnapshot();
}

TEST_F(SnapshotGlFramebufferObjectTest, BindColor0Texture2D) {
    setStateChanger([this] {
        GLuint texture;
        gl->glGenTextures(1, &texture);
        gl->glBindTexture(GL_TEXTURE_2D, texture);
        // In GLES2, mipmap level must be 0
        gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, texture, 0);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        m_state.attachments[GL_COLOR_ATTACHMENT0] = {GL_TEXTURE, texture, 0, 0};
    });
    doCheckedSnapshot();
}

}  // namespace emugl
