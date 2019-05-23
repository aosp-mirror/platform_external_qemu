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

#include "GLESv2Context.h"

#include "ProgramData.h"
#include "SamplerData.h"
#include "ShaderParser.h"
#include "TransformFeedbackData.h"
#include "android/base/files/StreamSerializing.h"

#include "emugl/common/crash_reporter.h"

#include <string.h>

static const char kGLES20StringPart[] = "OpenGL ES 2.0";
static const char kGLES30StringPart[] = "OpenGL ES 3.0";
static const char kGLES31StringPart[] = "OpenGL ES 3.1";
static const char kGLES32StringPart[] = "OpenGL ES 3.2";

static GLESVersion s_maxGlesVersion = GLES_2_0;

static const char* sPickVersionStringPart(int maj, int min) {
    switch (maj) {
        case 2:
            return kGLES20StringPart;
        case 3:
            switch (min) {
                case 0:
                    return kGLES30StringPart;
                case 1:
                    return kGLES31StringPart;
                case 2:
                    return kGLES32StringPart;
                default:
                    return nullptr;
            }
        default:
            return nullptr;
    }
    return nullptr;
}

void GLESv2Context::setMaxGlesVersion(GLESVersion version) {
    s_maxGlesVersion = version;
}

void GLESv2Context::initGlobal(EGLiface* iface) {
    s_glDispatch.dispatchFuncs(s_maxGlesVersion, iface->eglGetGlLibrary());
    GLEScontext::initGlobal(iface);
}

void GLESv2Context::init() {
    emugl::Mutex::AutoLock mutex(s_lock);
    if(!m_initialized) {
        GLEScontext::init();
        addVertexArrayObject(0);
        setVertexArrayObject(0);
        setAttribute0value(0.0, 0.0, 0.0, 1.0);

        buildStrings((const char*)dispatcher().glGetString(GL_VENDOR),
                     (const char*)dispatcher().glGetString(GL_RENDERER),
                     (const char*)dispatcher().glGetString(GL_VERSION),
                     sPickVersionStringPart(m_glesMajorVersion, m_glesMinorVersion));
        if (m_glesMajorVersion > 2 && !isGles2Gles()) {
            // OpenGL ES assumes that colors computed by / given to shaders will be converted to / from SRGB automatically
            // by the underlying implementation.
            // Desktop OpenGL makes no such assumption, and requires glEnable(GL_FRAMEBUFFER_SRGB) for the automatic conversion
            // to work.
            // This should work in most cases: just glEnable(GL_FRAMEBUFFER_SRGB) for every context.
            // But, that's not the whole story:
            // TODO: For dEQP tests standalone, we can just glEnable GL_FRAMEBUFFER_SRGB from the beginning and
            // pass all the framebuffer blit tests. However with CTS dEQP, EGL gets failures in color clear
            // and some dEQP-GLES3 framebuffer blit tests fail.
            // So we need to start out each context with GL_FRAMEBUFFER_SRGB disabled, and then enable it depending on
            // whether or not the current draw or read framebuffer has a SRGB texture color attachment.
            dispatcher().glDisable(GL_FRAMEBUFFER_SRGB);
            // Desktop OpenGL allows one to make cube maps seamless _or not_, but
            // OpenGL ES assumes seamless cubemaps are activated 100% of the time.
            // Many dEQP cube map tests fail without this enable.
            dispatcher().glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        }

        initEmulatedVAO();
        initEmulatedBuffers();
        // init emulated transform feedback
        if (m_glesMajorVersion >= 3) {
            m_transformFeedbackNameSpace->genName(
                    GenNameInfo(NamedObjectType::TRANSFORM_FEEDBACK), 0, false);
            TransformFeedbackData* tf = new TransformFeedbackData();
            tf->setMaxSize(getCaps()->maxTransformFeedbackSeparateAttribs);
            m_transformFeedbackNameSpace->setObjectData(0, ObjectDataPtr(tf));
        }
    }
    m_initialized = true;
}

void GLESv2Context::initDefaultFBO(
        GLint width, GLint height, GLint colorFormat, GLint depthstencilFormat, GLint multisamples,
        GLuint* eglSurfaceRBColorId, GLuint* eglSurfaceRBDepthId,
        GLuint readWidth, GLint readHeight, GLint readColorFormat, GLint readDepthstencilFormat, GLint readMultisamples,
        GLuint* eglReadSurfaceRBColorId, GLuint* eglReadSurfaceRBDepthId) {
    GLEScontext::initDefaultFBO(
            width, height, colorFormat, depthstencilFormat, multisamples,
            eglSurfaceRBColorId, eglSurfaceRBDepthId,
            readWidth, readHeight, readColorFormat, readDepthstencilFormat, readMultisamples,
            eglReadSurfaceRBColorId, eglReadSurfaceRBDepthId
            );
}


void GLESv2Context::initEmulatedVAO() {
    if (!isCoreProfile()) return;

    // Create emulated default VAO
    genVAOName(0, false);
    dispatcher().glBindVertexArray(getVAOGlobalName(0));
}

void GLESv2Context::initEmulatedBuffers() {
    if (m_emulatedClientVBOs.empty()) {
        // Create emulated client VBOs
        GLint neededClientVBOs = 0;
        dispatcher().glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &neededClientVBOs);

        // Spec minimum: 16 attribs. Some drivers won't report the right values.
        neededClientVBOs = std::max(neededClientVBOs, 16);

        m_emulatedClientVBOs.resize(neededClientVBOs, 0);
        dispatcher().glGenBuffers(neededClientVBOs, &m_emulatedClientVBOs[0]);
    }

    if (!m_emulatedClientIBO) {
        // Create emulated IBO
        dispatcher().glGenBuffers(1, &m_emulatedClientIBO);
    }
}

GLESv2Context::GLESv2Context(int maj, int min, GlobalNameSpace* globalNameSpace,
        android::base::Stream* stream, GlLibrary* glLib)
        : GLEScontext(globalNameSpace, stream, glLib) {
    if (stream) {
        assert(maj == m_glesMajorVersion);
        assert(min == m_glesMinorVersion);
        stream->read(m_attribute0value, sizeof(m_attribute0value));
        m_attribute0valueChanged = stream->getByte();
        m_att0ArrayLength = stream->getBe32();
        if (m_att0ArrayLength != 0) {
            m_att0Array.reset(new GLfloat[4 * m_att0ArrayLength]);
            stream->read(m_att0Array.get(), sizeof(GLfloat) * 4 * m_att0ArrayLength);
        }
        m_att0NeedsDisable = stream->getByte();
        m_useProgram = stream->getBe32();
        android::base::loadCollection(stream, &m_bindSampler,
                [](android::base::Stream* stream) {
                    GLuint idx = stream->getBe32();
                    GLuint val = stream->getBe32();
                    return std::make_pair(idx, val);
                });
    } else {
        m_glesMajorVersion = maj;
        m_glesMinorVersion = min;
    }
    ObjectData::loadObject_t loader = [this](NamedObjectType type,
                                             long long unsigned int localName,
                                             android::base::Stream* stream) {
        return loadObject(type, localName, stream);
    };
    m_transformFeedbackNameSpace =
            new NameSpace(NamedObjectType::TRANSFORM_FEEDBACK, globalNameSpace,
                          stream, loader);
}

GLESv2Context::~GLESv2Context() {
    if (m_emulatedClientIBO) {
        s_glDispatch.glDeleteBuffers(1, &m_emulatedClientIBO);
    }

    if (!m_emulatedClientVBOs.empty()) {
        s_glDispatch.glDeleteBuffers(
            m_emulatedClientVBOs.size(),
            &m_emulatedClientVBOs[0]);
    }

    deleteVAO(0);
    delete m_transformFeedbackNameSpace;
}

void GLESv2Context::onSave(android::base::Stream* stream) const {
    GLEScontext::onSave(stream);
    stream->write(m_attribute0value, sizeof(m_attribute0value));
    stream->putByte(m_attribute0valueChanged);
    stream->putBe32(m_att0ArrayLength);
    stream->write(m_att0Array.get(), sizeof(GLfloat) * 4 * m_att0ArrayLength);
    stream->putByte(m_att0NeedsDisable);
    stream->putBe32(m_useProgram);
    android::base::saveCollection(stream, m_bindSampler,
            [](android::base::Stream* stream,
                const std::pair<const GLenum, GLuint>& item) {
                stream->putBe32(item.first);
                stream->putBe32(item.second);
            });
    m_transformFeedbackNameSpace->onSave(stream);
}

void GLESv2Context::addVertexArrayObject(GLuint array) {
    m_vaoStateMap[array] = VAOState(0, nullptr, kMaxVertexAttributes);
}

void GLESv2Context::enableArr(GLenum arrType, bool enable) {
    uint32_t index = (uint32_t)arrType;
    if (index > kMaxVertexAttributes) return;
    m_currVaoState.attribInfo()[index].enable(enable);
}

const GLESpointer* GLESv2Context::getPointer(GLenum arrType) {
    uint32_t index = (uint32_t)arrType;
    if (index > kMaxVertexAttributes) return nullptr;
    return m_currVaoState.attribInfo().data() + index;
}

void GLESv2Context::postLoadRestoreCtx() {
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    m_useProgramData = shareGroup()->getObjectDataPtr(
            NamedObjectType::SHADER_OR_PROGRAM, m_useProgram);
    const GLuint globalProgramName = shareGroup()->getGlobalName(
            NamedObjectType::SHADER_OR_PROGRAM, m_useProgram);
    dispatcher.glUseProgram(globalProgramName);

    initEmulatedBuffers();
    initEmulatedVAO();

    // vertex attribute pointers
    for (const auto& vaoIte : m_vaoStateMap) {
        if (vaoIte.first != 0) {
            genVAOName(vaoIte.first, false);
        }
        if (m_glesMajorVersion >= 3) {
            dispatcher.glBindVertexArray(getVAOGlobalName(vaoIte.first));
        }
        for (uint32_t i = 0; i < kMaxVertexAttributes; ++i) {
            GLESpointer* glesPointer =
                (GLESpointer*)(vaoIte.second.vertexAttribInfo.data() + i);

            // don't skip enabling if the guest assumes it was enabled.
            if (glesPointer->isEnable()) {
                dispatcher.glEnableVertexAttribArray(i);
            }

            // attribute 0 are bound right before draw, no need to bind it here
            if (glesPointer->getAttribType() == GLESpointer::VALUE
                    && i == 0) {
                break;
            }
            switch (glesPointer->getAttribType()) {
                case GLESpointer::BUFFER: {
                    const GLuint globalBufferName = shareGroup()
                            ->getGlobalName(NamedObjectType::VERTEXBUFFER,
                                            glesPointer->getBufferName());
                    if (!globalBufferName) {
                        continue;
                    }
                    glesPointer->restoreBufferObj(getBufferObj);
                    dispatcher.glBindBuffer(GL_ARRAY_BUFFER,
                            globalBufferName);
                    if (glesPointer->isIntPointer()) {
                        dispatcher.glVertexAttribIPointer(i,
                                glesPointer->getSize(),
                                glesPointer->getType(),
                                glesPointer->getStride(),
                                (GLvoid*)(size_t)glesPointer->getBufferOffset());
                    } else {
                        dispatcher.glVertexAttribPointer(i,
                                glesPointer->getSize(),
                                glesPointer->getType(), glesPointer->isNormalize(),
                                glesPointer->getStride(),
                                (GLvoid*)(size_t)glesPointer->getBufferOffset());
                    }
                    break;
                }
                case GLESpointer::VALUE:
                    switch (glesPointer->getValueCount()) {
                        case 1:
                            dispatcher.glVertexAttrib1fv(i,
                                    glesPointer->getValues());
                            break;
                        case 2:
                            dispatcher.glVertexAttrib2fv(i,
                                    glesPointer->getValues());
                            break;
                        case 3:
                            dispatcher.glVertexAttrib3fv(i,
                                    glesPointer->getValues());
                            break;
                        case 4:
                            dispatcher.glVertexAttrib4fv(i,
                                    glesPointer->getValues());
                            break;
                    }
                    break;
                case GLESpointer::ARRAY:
                    // client arrays are set up right before draw calls
                    // so we do nothing here
                    break;
            }
        }
        for (size_t i = 0; i < vaoIte.second.bindingState.size(); i++) {
            const BufferBinding& bufferBinding = vaoIte.second.bindingState[i];
            if (bufferBinding.divisor) {
                dispatcher.glVertexAttribDivisor(i, bufferBinding.divisor);
            }
        }
    }
    if (m_glesMajorVersion >= 3) {
        dispatcher.glBindVertexArray(getVAOGlobalName(m_currVaoState.vaoId()));
        auto bindBufferRangeFunc =
                [this](GLenum target,
                    const std::vector<BufferBinding>& bufferBindings) {
                    for (unsigned int i = 0; i < bufferBindings.size(); i++) {
                        const BufferBinding& bd = bufferBindings[i];
                        GLuint globalName = this->shareGroup()->getGlobalName(
                                NamedObjectType::VERTEXBUFFER,
                                bd.buffer);
                        assert(bd.buffer == 0 || globalName != 0);
                        if (bd.isBindBase || bd.buffer == 0) {
                            this->dispatcher().glBindBufferBase(target,
                                    i, globalName);
                        } else {
                            this->dispatcher().glBindBufferRange(target,
                                    i, globalName, bd.offset, bd.size);
                        }
                    }
                };
        bindBufferRangeFunc(GL_TRANSFORM_FEEDBACK_BUFFER,
                m_indexedTransformFeedbackBuffers);
        bindBufferRangeFunc(GL_UNIFORM_BUFFER,
                m_indexedUniformBuffers);

        if (m_glesMinorVersion >= 1) {
            bindBufferRangeFunc(GL_ATOMIC_COUNTER_BUFFER,
                    m_indexedAtomicCounterBuffers);
            bindBufferRangeFunc(GL_SHADER_STORAGE_BUFFER,
                    m_indexedShaderStorageBuffers);
        }

        // buffer bindings
        auto bindBuffer = [this](GLenum target, GLuint buffer) {
            this->dispatcher().glBindBuffer(target,
                    m_shareGroup->getGlobalName(NamedObjectType::VERTEXBUFFER, buffer));
        };
        bindBuffer(GL_COPY_READ_BUFFER, m_copyReadBuffer);
        bindBuffer(GL_COPY_WRITE_BUFFER, m_copyWriteBuffer);
        bindBuffer(GL_PIXEL_PACK_BUFFER, m_pixelPackBuffer);
        bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pixelUnpackBuffer);
        bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_transformFeedbackBuffer);
        bindBuffer(GL_UNIFORM_BUFFER, m_uniformBuffer);

        if (m_glesMinorVersion >= 1) {
            bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
            bindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatchIndirectBuffer);
            bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawIndirectBuffer);
            bindBuffer(GL_SHADER_STORAGE_BUFFER, m_shaderStorageBuffer);
        }

        for (const auto& bindSampler : m_bindSampler) {
            dispatcher.glBindSampler(bindSampler.first,
                    shareGroup()->getGlobalName(NamedObjectType::SAMPLER,
                        bindSampler.second));
        }
        m_transformFeedbackNameSpace->postLoadRestore(
                [this](NamedObjectType p_type, ObjectLocalName p_localName) {
                    switch (p_type) {
                        case NamedObjectType::FRAMEBUFFER:
                            return getFBOGlobalName(p_localName);
                        case NamedObjectType::TRANSFORM_FEEDBACK:
                            return getTransformFeedbackGlobalName(p_localName);
                        default:
                            return m_shareGroup->getGlobalName(p_type,
                                                               p_localName);
                    }
                });
        dispatcher.glBindTransformFeedback(
                GL_TRANSFORM_FEEDBACK,
                getTransformFeedbackGlobalName(m_transformFeedbackBuffer));
    }

    GLEScontext::postLoadRestoreCtx();
}

ObjectDataPtr GLESv2Context::loadObject(NamedObjectType type,
            ObjectLocalName localName, android::base::Stream* stream) const {
    switch (type) {
        case NamedObjectType::VERTEXBUFFER:
        case NamedObjectType::TEXTURE:
        case NamedObjectType::FRAMEBUFFER:
        case NamedObjectType::RENDERBUFFER:
            return GLEScontext::loadObject(type, localName, stream);
        case NamedObjectType::SAMPLER:
            return ObjectDataPtr(new SamplerData(stream));
        case NamedObjectType::SHADER_OR_PROGRAM:
            // load the first bit to see if it is a program or shader
            switch (stream->getByte()) {
                case LOAD_PROGRAM:
                    return ObjectDataPtr(new ProgramData(stream));
                case LOAD_SHADER:
                    return ObjectDataPtr(new ShaderParser(stream));
                default:
                    fprintf(stderr, "corrupted snapshot\n");
                    assert(false);
                    return nullptr;
            }
        case NamedObjectType::TRANSFORM_FEEDBACK:
            return ObjectDataPtr(new TransformFeedbackData(stream));
        default:
            return nullptr;
    }
}

void GLESv2Context::setAttribValue(int idx, unsigned int count,
        const GLfloat* val) {
    m_currVaoState.attribInfo()[idx].setValue(count, val);
}

void GLESv2Context::setAttribute0value(float x, float y, float z, float w)
{
    m_attribute0valueChanged |=
            x != m_attribute0value[0] || y != m_attribute0value[1] ||
            z != m_attribute0value[2] || w != m_attribute0value[3];
    m_attribute0value[0] = x;
    m_attribute0value[1] = y;
    m_attribute0value[2] = z;
    m_attribute0value[3] = w;
}

bool GLESv2Context::needAtt0PreDrawValidation()
{
    m_att0NeedsDisable = false;

    // We could go into the driver here and call
    //      s_glDispatch.glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled)
    // ... but it's too much for a simple check that runs on almost every draw
    // call.
    return !isArrEnabled(0);
}

void GLESv2Context::validateAtt0PreDraw(unsigned int count)
{
    if (count == 0) {
        return;
    }

    if (count > m_att0ArrayLength) {
        const unsigned newLen = std::max(count, 2 * m_att0ArrayLength);
        m_att0Array.reset(new GLfloat[4 * newLen]);
        m_att0ArrayLength = newLen;
        m_attribute0valueChanged = true;
    }
    if (m_attribute0valueChanged) {
        for(unsigned int i = 0; i<m_att0ArrayLength; i++) {
            memcpy(m_att0Array.get()+i*4, m_attribute0value,
                    sizeof(m_attribute0value));
        }
        m_attribute0valueChanged = false;
    }

    s_glDispatch.glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0,
            m_att0Array.get());
    s_glDispatch.glEnableVertexAttribArray(0);

    m_att0NeedsDisable = true;
}

void GLESv2Context::validateAtt0PostDraw(void)
{
    if (m_att0NeedsDisable) {
        s_glDispatch.glDisableVertexAttribArray(0);
        m_att0NeedsDisable = false;
    }
}

void GLESv2Context::drawWithEmulations(
    DrawCallCmd cmd,
    GLenum mode,
    GLint first,
    GLsizei count,
    GLenum type,
    const GLvoid* indices,
    GLsizei primcount,
    GLuint start,
    GLuint end) {

    if (getMajorVersion() < 3) {
        drawValidate();
    }

    bool needClientVBOSetup = !vertexAttributesBufferBacked();

    bool needClientIBOSetup =
        (cmd != DrawCallCmd::Arrays &&
         cmd != DrawCallCmd::ArraysInstanced) &&
        !isBindedBuffer(GL_ELEMENT_ARRAY_BUFFER);
    bool needPointEmulation = mode == GL_POINTS && !isGles2Gles();

#ifdef __APPLE__
    if (primitiveRestartEnabled() && type) {
        updatePrimitiveRestartIndex(type);
    }
#endif

    if (needPointEmulation) {
        s_glDispatch.glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        if (!isCoreProfile()) {
            // Enable texture generation for GL_POINTS and gl_PointSize shader variable
            // GLES2 assumes this is enabled by default, we need to set this state for GL
            s_glDispatch.glEnable(GL_POINT_SPRITE);
        }
    }

    if (needClientVBOSetup) {
        GLESConversionArrays tmpArrs;
        setupArraysPointers(tmpArrs, 0, count, type, indices, false);
        if (needAtt0PreDrawValidation()) {
            if (indices) {
                validateAtt0PreDraw(findMaxIndex(count, type, indices));
            } else {
                validateAtt0PreDraw(count);
            }
        }
    }

    GLuint prevIBO;
    if (needClientIBOSetup) {
        int bpv = 2;
        switch (type) {
            case GL_UNSIGNED_BYTE:
                bpv = 1;
                break;
            case GL_UNSIGNED_SHORT:
                bpv = 2;
                break;
            case GL_UNSIGNED_INT:
                bpv = 4;
                break;
        }

        size_t dataSize = bpv * count;

        s_glDispatch.glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, (GLint*)&prevIBO);
        s_glDispatch.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_emulatedClientIBO);
        s_glDispatch.glBufferData(GL_ELEMENT_ARRAY_BUFFER, dataSize, indices, GL_STREAM_DRAW);
    }

    const GLvoid* indicesOrOffset =
        needClientIBOSetup ? nullptr : indices;

    switch (cmd) {
        case DrawCallCmd::Elements:
            s_glDispatch.glDrawElements(mode, count, type, indicesOrOffset);
            break;
        case DrawCallCmd::ElementsInstanced:
            s_glDispatch.glDrawElementsInstanced(mode, count, type,
                                                 indicesOrOffset,
                                                 primcount);
            break;
        case DrawCallCmd::RangeElements:
            s_glDispatch.glDrawRangeElements(mode, start, end, count, type,
                                             indicesOrOffset);
            break;
        case DrawCallCmd::Arrays:
            s_glDispatch.glDrawArrays(mode, first, count);
            break;
        case DrawCallCmd::ArraysInstanced:
            s_glDispatch.glDrawArraysInstanced(mode, first, count, primcount);
            break;
        default:
            emugl::emugl_crash_reporter(
                    "drawWithEmulations has corrupt call parameters!");
    }

    if (needClientIBOSetup) {
        s_glDispatch.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prevIBO);
    }

    if (needClientVBOSetup) {
        validateAtt0PostDraw();
    }

    if (needPointEmulation) {
        s_glDispatch.glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
        if (!isCoreProfile()) {
            s_glDispatch.glDisable(GL_POINT_SPRITE);
        }
    }
}

void GLESv2Context::setupArraysPointers(GLESConversionArrays& cArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct) {
    //going over all clients arrays Pointers
    for (uint32_t i = 0; i < kMaxVertexAttributes; ++i) {
        GLESpointer* p = m_currVaoState.attribInfo().data() + i;
        if (!p->isEnable() || p->getAttribType() == GLESpointer::VALUE) {
            continue;
        }

        setupArrWithDataSize(
            p->getDataSize(),
            p->getArrayData(),
            i,
            p->getType(),
            p->getSize(),
            p->getStride(),
            p->getNormalized(),
            -1,
            p->isIntPointer());
    }
}

//setting client side arr
void GLESv2Context::setupArrWithDataSize(GLsizei datasize, const GLvoid* arr,
                                         GLenum arrayType, GLenum dataType,
                                         GLint size, GLsizei stride, GLboolean normalized, int index, bool isInt){
    // is not really a client side arr.
    if (arr == NULL) return;

    GLuint prevArrayBuffer;
    s_glDispatch.glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&prevArrayBuffer);

    if (arrayType < m_emulatedClientVBOs.size()) {
        s_glDispatch.glBindBuffer(GL_ARRAY_BUFFER, m_emulatedClientVBOs[arrayType]);
    } else {
        fprintf(stderr, "%s: invalid attribute index: %d\n", __func__, (int)arrayType);
    }

    s_glDispatch.glBufferData(GL_ARRAY_BUFFER, datasize, arr, GL_STREAM_DRAW);

    if (isInt) {
        s_glDispatch.glVertexAttribIPointer(arrayType, size, dataType, stride, 0);
    } else {
        s_glDispatch.glVertexAttribPointer(arrayType, size, dataType, normalized, stride, 0);
    }

    s_glDispatch.glBindBuffer(GL_ARRAY_BUFFER, prevArrayBuffer);
}

void GLESv2Context::setVertexAttribDivisor(GLuint bindingindex, GLuint divisor) {
    if (bindingindex >= m_currVaoState.bufferBindings().size()) {
        return;
    }
    m_currVaoState.bufferBindings()[bindingindex].divisor = divisor;
}

void GLESv2Context::setVertexAttribBindingIndex(GLuint attribindex, GLuint bindingindex) {
    if (attribindex > kMaxVertexAttributes) return;

    m_currVaoState.attribInfo()[attribindex].setBindingIndex(bindingindex);
}

void GLESv2Context::setVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint reloffset, bool isInt) {
    if (attribindex > kMaxVertexAttributes) return;
    m_currVaoState.attribInfo()[attribindex].setFormat(size, type, normalized == GL_TRUE, reloffset, isInt);
}

void GLESv2Context::setBindSampler(GLuint unit, GLuint sampler) {
    m_bindSampler[unit] = sampler;
}

bool GLESv2Context::needConvert(GLESConversionArrays& cArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct,GLESpointer* p,GLenum array_id) {

    bool usingVBO = p->getAttribType() == GLESpointer::BUFFER;
    GLenum arrType = p->getType();

    /*
     conversion is not necessary in the following cases:
      (*) array type is not fixed
    */
    if(arrType != GL_FIXED) return false;

    if(!usingVBO) {
        if (direct) {
            convertDirect(cArrs,first,count,array_id,p);
        } else {
            convertIndirect(cArrs,count,type,indices,array_id,p);
        }
    } else {
        if (direct) {
            convertDirectVBO(cArrs,first,count,array_id,p);
        } else {
            convertIndirectVBO(cArrs,count,type,indices,array_id,p);
        }
    }
    return true;
}

void GLESv2Context::setUseProgram(GLuint program,
        const ObjectDataPtr& programData) {
    m_useProgram = program;
    assert(!programData ||
            programData->getDataType() == ObjectDataType::PROGRAM_DATA);
    m_useProgramData = programData;
}

GLuint GLESv2Context::getCurrentProgram() const {
    return m_useProgram;
}

ProgramData* GLESv2Context::getUseProgram() {
    return (ProgramData*)m_useProgramData.get();
}

void GLESv2Context::initExtensionString() {
    if (s_glExtensionsInitialized) return;

    *s_glExtensions = "GL_OES_EGL_image GL_OES_EGL_image_external GL_OES_depth24 GL_OES_depth32 GL_OES_element_index_uint "
                      "GL_OES_texture_float GL_OES_texture_float_linear "
                      "GL_OES_compressed_paletted_texture GL_OES_compressed_ETC1_RGB8_texture GL_OES_depth_texture ";
    if (s_glSupport.GL_ARB_HALF_FLOAT_PIXEL || s_glSupport.GL_NV_HALF_FLOAT)
        *s_glExtensions+="GL_OES_texture_half_float GL_OES_texture_half_float_linear ";
    if (s_glSupport.GL_EXT_PACKED_DEPTH_STENCIL)
        *s_glExtensions+="GL_OES_packed_depth_stencil ";
    if (s_glSupport.GL_ARB_HALF_FLOAT_VERTEX)
        *s_glExtensions+="GL_OES_vertex_half_float ";
    if (s_glSupport.GL_OES_STANDARD_DERIVATIVES)
        *s_glExtensions+="GL_OES_standard_derivatives ";
    if (s_glSupport.GL_OES_TEXTURE_NPOT)
        *s_glExtensions+="GL_OES_texture_npot ";
    if (s_glSupport.GL_OES_RGB8_RGBA8)
        *s_glExtensions+="GL_OES_rgb8_rgba8 ";
    if (s_glSupport.ext_GL_EXT_color_buffer_float)
        *s_glExtensions+="GL_EXT_color_buffer_float ";
    if (s_glSupport.ext_GL_EXT_color_buffer_half_float)
        *s_glExtensions+="GL_EXT_color_buffer_half_float ";
    if (s_glSupport.ext_GL_EXT_shader_framebuffer_fetch)
        *s_glExtensions+="GL_EXT_shader_framebuffer_fetch ";
    if (s_glSupport.GL_EXT_TEXTURE_FORMAT_BGRA8888) {
        *s_glExtensions+="GL_EXT_texture_format_BGRA8888 GL_APPLE_texture_format_BGRA8888 ";
    }

    s_glExtensionsInitialized = true;
}

int GLESv2Context::getMaxTexUnits() {
    return getCaps()->maxTexImageUnits;
}

int GLESv2Context::getMaxCombinedTexUnits() {
    return getCaps()->maxCombinedTexImageUnits;
}

unsigned int GLESv2Context::getTransformFeedbackGlobalName(
        ObjectLocalName p_localName) {
    return m_transformFeedbackNameSpace->getGlobalName(p_localName);
}

bool GLESv2Context::hasBoundTransformFeedback(
        ObjectLocalName transformFeedback) {
    return transformFeedback &&
           m_transformFeedbackNameSpace->getObjectDataPtr(transformFeedback)
                   .get();
}

ObjectLocalName GLESv2Context::genTransformFeedbackName(
        ObjectLocalName p_localName,
        bool genLocal) {
    return m_transformFeedbackNameSpace->genName(
            GenNameInfo(NamedObjectType::TRANSFORM_FEEDBACK), p_localName,
            genLocal);
}

void GLESv2Context::bindTransformFeedback(ObjectLocalName p_localName) {
    if (m_transformFeedbackDeletePending &&
        m_bindTransformFeedback != p_localName) {
        m_transformFeedbackNameSpace->deleteName(m_bindTransformFeedback);
        m_transformFeedbackDeletePending = false;
    }
    m_bindTransformFeedback = p_localName;
    if (p_localName &&
        !m_transformFeedbackNameSpace->getGlobalName(p_localName)) {
        genTransformFeedbackName(p_localName, false);
    }
    if (p_localName &&
        !m_transformFeedbackNameSpace->getObjectDataPtr(p_localName).get()) {
        TransformFeedbackData* tf = new TransformFeedbackData();
        tf->setMaxSize(getCaps()->maxTransformFeedbackSeparateAttribs);
        m_transformFeedbackNameSpace->setObjectData(p_localName,
                                                    ObjectDataPtr(tf));
    }
}

ObjectLocalName GLESv2Context::getTransformFeedbackBinding() {
    return m_bindTransformFeedback;
}

void GLESv2Context::deleteTransformFeedback(ObjectLocalName p_localName) {
    // Note: GLES3.0 says it should be pending for delete if it is active
    // GLES3.2 says report error in this situation
    if (m_bindTransformFeedback == p_localName) {
        m_transformFeedbackDeletePending = true;
        return;
    }
    m_transformFeedbackNameSpace->deleteName(p_localName);
}

TransformFeedbackData* GLESv2Context::boundTransformFeedback() {
    return (TransformFeedbackData*)m_transformFeedbackNameSpace
            ->getObjectDataPtr(m_bindTransformFeedback)
            .get();
}

GLuint GLESv2Context::getIndexedBuffer(GLenum target, GLuint index) {
    switch (target) {
        case GL_TRANSFORM_FEEDBACK_BUFFER:
            return boundTransformFeedback()->getIndexedBuffer(index);
        default:
            return GLEScontext::getIndexedBuffer(target, index);
    }
}

void GLESv2Context::bindIndexedBuffer(GLenum target,
                                      GLuint index,
                                      GLuint buffer,
                                      GLintptr offset,
                                      GLsizeiptr size,
                                      GLintptr stride,
                                      bool isBindBase) {
    switch (target) {
        case GL_TRANSFORM_FEEDBACK_BUFFER: {
            TransformFeedbackData* tf = boundTransformFeedback();
            tf->bindIndexedBuffer(index, buffer, offset, size, stride,
                                  isBindBase);
            break;
        }
        default:
            GLEScontext::bindIndexedBuffer(target, index, buffer, offset, size,
                                           stride, isBindBase);
    }
}

void GLESv2Context::bindIndexedBuffer(GLenum target,
                                      GLuint index,
                                      GLuint buffer) {
    GLEScontext::bindIndexedBuffer(target, index, buffer);
}

void GLESv2Context::unbindBuffer(GLuint buffer) {
    if (m_glesMajorVersion >= 3) {
        boundTransformFeedback()->unbindBuffer(buffer);
    }
    GLEScontext::unbindBuffer(buffer);
}
