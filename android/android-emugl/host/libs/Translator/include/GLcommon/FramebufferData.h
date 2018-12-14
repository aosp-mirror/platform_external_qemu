/*
* Copyright (C) 2011 The Android Open Source Project
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
#ifndef _FRAMEBUFFER_DATA_H
#define _FRAMEBUFFER_DATA_H

#include "ObjectData.h"
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES3/gl3.h>
#include <vector>

class SaveableTexture;
typedef std::shared_ptr<SaveableTexture> SaveableTexturePtr;

class RenderbufferData : public ObjectData
{
public:
    RenderbufferData() : ObjectData(RENDERBUFFER_DATA) {  };
    RenderbufferData(android::base::Stream* stream);
    void onSave(android::base::Stream* stream,
                unsigned int globalName) const override;
    void restore(ObjectLocalName localName,
                 const getGlobalName_t& getGlobalName) override;
    // Mark the texture handles dirty
    void makeTextureDirty();
    GLuint attachedFB = 0;
    GLenum attachedPoint = 0;
    NamedObjectPtr eglImageGlobalTexObject = 0;
    SaveableTexturePtr saveableTexture = {};

    // We call on the dispatcher rather than returning these values when
    // glGetRenderbufferParameter is called, so the initial format values here
    // are unimportant; hostInternalFormat still being GL_NONE indicates
    // we can skip the glRenderbufferStorage call when restoring this state.
    GLenum internalformat = GL_RGBA4;
    GLenum hostInternalFormat = GL_NONE;

    GLsizei width = 0;
    GLsizei height = 0;
    GLint samples = 0;

    bool everBound = false;
};

const int MAX_ATTACH_POINTS = 19;

class FramebufferData : public ObjectData
{
public:
    explicit FramebufferData(GLuint name, GLuint globalName);
    FramebufferData(android::base::Stream* stream);
    ~FramebufferData();
    void onSave(android::base::Stream* stream,
                unsigned int globalName) const override;
    void postLoad(const getObjDataPtr_t& getObjDataPtr) override;
    void restore(ObjectLocalName localName,
                 const getGlobalName_t& getGlobalName) override;

    void setAttachment(class GLEScontext* ctx,
                       GLenum attachment,
                       GLenum target,
                       GLuint name,
                       ObjectDataPtr obj,
                       bool takeOwnership = false);

    GLuint getAttachment(GLenum attachment,
                         GLenum *outTarget,
                         ObjectDataPtr *outObj);

    GLint getAttachmentSamples(class GLEScontext* ctx, GLenum attachment);
    void getAttachmentDimensions(class GLEScontext* ctx, GLenum attachment, GLint* width, GLint* height);
    GLint getAttachmentInternalFormat(class GLEScontext* ctx, GLenum attachment);

    void validate(class GLEScontext* ctx);

    void setBoundAtLeastOnce() {
        m_hasBeenBound = true;
    }

    bool hasBeenBoundAtLeastOnce() const {
        return m_hasBeenBound;
    }

    void setDrawBuffers(GLsizei n, const GLenum * bufs);
    void setReadBuffers(GLenum src);

    GLenum getReadBuffer() const {
        return m_readBuffer;
    }

    // Mark the texture handles dirty
    void makeTextureDirty(const getObjDataPtr_t& getObjDataPtr);

    void separateDepthStencilWorkaround(class GLEScontext* ctx);

private:
    inline int attachmentPointIndex(GLenum attachment);
    void detachObject(int idx);
    void refreshSeparateDepthStencilAttachmentState();

private:
    GLuint m_fbName = 0;
    GLuint m_fbGlobalName = 0;
    struct attachPoint {
        GLenum target; // OGL if owned, GLES otherwise
        GLuint name; // OGL if owned, GLES otherwise
        GLuint globalName; // derived from |name| on attachment setting
        // objType is only used in snapshot postLoad, for retrieving obj.
        // objType's data is only valid between loading and postLoad snapshot
        NamedObjectType objType;
        ObjectDataPtr obj;
        bool owned;
    } m_attachPoints[MAX_ATTACH_POINTS+1] = {};
    bool m_dirty = false;
    bool m_hasBeenBound = false;
    bool m_hasDrawBuffers = false;
    bool m_hasSeparateDepthStencil = false;
    // Invariant: m_separateDSEmulationRbo != 0 iff we are actively
    // emulating separate depth/stencil buffers.
    GLuint m_separateDSEmulationRbo = 0;
    std::vector<GLenum> m_drawBuffers = {};
    GLenum m_readBuffer = GL_COLOR_ATTACHMENT0;
};

#endif
