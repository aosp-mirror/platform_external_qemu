/*
* Copyright 2011 The Android Open Source Project
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

#include <GLcommon/GLEScontext.h>

#include "android/base/containers/Lookup.h"
#include "android/base/files/StreamSerializing.h"

#include <GLcommon/GLconversion_macros.h>
#include <GLcommon/GLSnapshotSerializers.h>
#include <GLcommon/GLESmacros.h>
#include <GLcommon/TextureData.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <OpenglCodecCommon/ErrorLog.h>
#include <GLcommon/GLESvalidate.h>
#include <GLcommon/TextureUtils.h>
#include <GLcommon/FramebufferData.h>
#include <GLcommon/ScopedGLState.h>
#ifndef _MSC_VER
#include <strings.h>
#endif
#include <string.h>

#include <numeric>

//decleration
static void convertFixedDirectLoop(const char* dataIn,unsigned int strideIn,void* dataOut,unsigned int nBytes,unsigned int strideOut,int attribSize);
static void convertFixedIndirectLoop(const char* dataIn,unsigned int strideIn,void* dataOut,GLsizei count,GLenum indices_type,const GLvoid* indices,unsigned int strideOut,int attribSize);
static void convertByteDirectLoop(const char* dataIn,unsigned int strideIn,void* dataOut,unsigned int nBytes,unsigned int strideOut,int attribSize);
static void convertByteIndirectLoop(const char* dataIn,unsigned int strideIn,void* dataOut,GLsizei count,GLenum indices_type,const GLvoid* indices,unsigned int strideOut,int attribSize);

void BufferBinding::onLoad(android::base::Stream* stream) {
    buffer = stream->getBe32();
    offset = stream->getBe32();
    size = stream->getBe32();
    stride = stream->getBe32();
    divisor = stream->getBe32();
    isBindBase = stream->getByte();
}

void BufferBinding::onSave(android::base::Stream* stream) const {
    stream->putBe32(buffer);
    stream->putBe32(offset);
    stream->putBe32(size);
    stream->putBe32(stride);
    stream->putBe32(divisor);
    stream->putByte(isBindBase);
}

VAOState::VAOState(android::base::Stream* stream) {
    element_array_buffer_binding = stream->getBe32();

    vertexAttribInfo.clear();
    for (uint32_t i = 0; i < kMaxVertexAttributes; ++i) {
        vertexAttribInfo.emplace_back(stream);
    }

    uint64_t arraysMapPtr = stream->getBe64();

    if (arraysMapPtr) {
        arraysMap.reset(new ArraysMap());
        size_t mapSize = stream->getBe32();
        for (size_t i = 0; i < mapSize; i++) {
            GLuint id = stream->getBe32();
            arraysMap->emplace(id, new GLESpointer(stream));
        }
        legacy = true;
    } else {
        arraysMap.reset();
    }

    loadContainer(stream, bindingState);
    bufferBacked = stream->getByte();
    everBound = stream->getByte();
}

void VAOState::onSave(android::base::Stream* stream) const {
    stream->putBe32(element_array_buffer_binding);
    for (uint32_t i = 0; i < kMaxVertexAttributes; ++i) {
        vertexAttribInfo[i].onSave(stream);
    }

    if (arraysMap) {
        stream->putBe64((uint64_t)(uintptr_t)arraysMap.get());
    } else {
        stream->putBe64(0);
    }

    if (arraysMap) {
        stream->putBe32(arraysMap->size());
        for (const auto& ite : *arraysMap) {
            stream->putBe32(ite.first);
            assert(ite.second);
            ite.second->onSave(stream);
        }
    }

    saveContainer(stream, bindingState);
    stream->putByte(bufferBacked);
    stream->putByte(everBound);
}

GLESConversionArrays::~GLESConversionArrays() {
    for(auto it = m_arrays.begin(); it != m_arrays.end(); ++it) {
        if((*it).second.allocated){
            if((*it).second.type == GL_FLOAT){
                GLfloat* p = (GLfloat *)((*it).second.data);
                if(p) delete[] p;
            } else if((*it).second.type == GL_SHORT){
                GLshort* p = (GLshort *)((*it).second.data);
                if(p) delete[] p;
            }
        }
    }
}

void GLESConversionArrays::allocArr(unsigned int size,GLenum type){
    if(type == GL_FIXED){
        m_arrays[m_current].data = new GLfloat[size];
        m_arrays[m_current].type = GL_FLOAT;
    } else if(type == GL_BYTE){
        m_arrays[m_current].data = new GLshort[size];
        m_arrays[m_current].type = GL_SHORT;
    }
    m_arrays[m_current].stride = 0;
    m_arrays[m_current].allocated = true;
}

void GLESConversionArrays::setArr(void* data,unsigned int stride,GLenum type){
   m_arrays[m_current].type = type;
   m_arrays[m_current].data = data;
   m_arrays[m_current].stride = stride;
   m_arrays[m_current].allocated = false;
}

void* GLESConversionArrays::getCurrentData(){
    return m_arrays[m_current].data;
}

ArrayData& GLESConversionArrays::getCurrentArray(){
    return m_arrays[m_current];
}

unsigned int GLESConversionArrays::getCurrentIndex(){
    return m_current;
}

ArrayData& GLESConversionArrays::operator[](int i){
    return m_arrays[i];
}

void GLESConversionArrays::operator++(){
    m_current++;
}

GLDispatch     GLEScontext::s_glDispatch;
emugl::Mutex   GLEScontext::s_lock;
std::string*   GLEScontext::s_glExtensions = NULL;
bool           GLEScontext::s_glExtensionsInitialized = false;
std::string    GLEScontext::s_glVendor;
std::string    GLEScontext::s_glRenderer;
std::string    GLEScontext::s_glVersion;
GLSupport      GLEScontext::s_glSupport;

Version::Version(int major,int minor,int release):m_major(major),
                                                  m_minor(minor),
                                                  m_release(release){};

Version::Version(const Version& ver):m_major(ver.m_major),
                                     m_minor(ver.m_minor),
                                     m_release(ver.m_release){}

Version::Version(const char* versionString){
    m_release = 0;
    if((!versionString) ||
      ((!(sscanf(versionString,"%d.%d"   ,&m_major,&m_minor) == 2)) &&
       (!(sscanf(versionString,"%d.%d.%d",&m_major,&m_minor,&m_release) == 3)))){
        m_major = m_minor = 0; // the version is not in the right format
    }
}

Version& Version::operator=(const Version& ver){
    m_major   = ver.m_major;
    m_minor   = ver.m_minor;
    m_release = ver.m_release;
    return *this;
}

bool Version::operator<(const Version& ver) const{
    if(m_major < ver.m_major) return true;
    if(m_major == ver.m_major){
        if(m_minor < ver.m_minor) return true;
        if(m_minor == ver.m_minor){
           return m_release < ver.m_release;
        }
    }
    return false;
}

static std::string getHostExtensionsString(GLDispatch* dispatch) {
    // glGetString(GL_EXTENSIONS) is deprecated in GL 3.0, one has to use
    // glGetStringi(GL_EXTENSIONS, index) instead to get individual extension
    // names. Recent desktop drivers implement glGetStringi() but have a
    // version of glGetString() that returns NULL, so deal with this by
    // doing the following:
    //
    //  - If glGetStringi() is available, and glGetIntegerv(GL_NUM_EXTENSIONS &num_exts)
    //    gives a nonzero value, use it to build the extensions
    //    string, using simple spaces to separate the names.
    //
    //  - Otherwise, fallback to getGetString(). If it returns NULL, return
    //    an empty string.
    //

    std::string result;
    int num_exts = 0;

    if (dispatch->glGetStringi) {
        dispatch->glGetIntegerv(GL_NUM_EXTENSIONS, &num_exts);
        GLenum err = dispatch->glGetError();
        if (err == GL_NO_ERROR) {
            for (int n = 0; n < num_exts; n++) {
                const char* ext = reinterpret_cast<const char*>(
                        dispatch->glGetStringi(GL_EXTENSIONS, n));
                if (ext != NULL) {
                    if (!result.empty()) {
                        result += " ";
                    }
                    result += ext;
                }
            }
        }
    }

    // If glGetIntegerv does not affect the value,
    // our system does not actually support
    // GL 3.0 style extension getting.
    if (!dispatch->glGetStringi || num_exts == 0) {
        const char* extensions = reinterpret_cast<const char*>(
                dispatch->glGetString(GL_EXTENSIONS));
        if (extensions) {
            result = extensions;
        }
    }

    // For the sake of initCapsLocked() add a starting and trailing space.
    if (!result.empty()) {
        if (result[0] != ' ') {
            result.insert(0, 1, ' ');
        }
        if (result[result.size() - 1U] != ' ') {
            result += ' ';
        }
    }
    return result;
}

static GLuint getIndex(GLenum indices_type, const GLvoid* indices, unsigned int i) {
    switch (indices_type) {
        case GL_UNSIGNED_BYTE:
            return static_cast<const GLubyte*>(indices)[i];
        case GL_UNSIGNED_SHORT:
            return static_cast<const GLushort*>(indices)[i];
        case GL_UNSIGNED_INT:
            return static_cast<const GLuint*>(indices)[i];
        default:
            ERR("**** ERROR unknown type 0x%x (%s,%d)\n", indices_type, __FUNCTION__,__LINE__);
            return 0;
    }
}

void GLEScontext::addVertexArrayObjects(GLsizei n, GLuint* arrays) {
    for (int i = 0; i < n; i++) {
        addVertexArrayObject(arrays[i]);
    }
}

void GLEScontext::removeVertexArrayObjects(GLsizei n, const GLuint* arrays) {
    for (int i = 0; i < n; i++) {
        removeVertexArrayObject(arrays[i]);
    }
}

void GLEScontext::addVertexArrayObject(GLuint array) {
    ArraysMap* map = new ArraysMap();
    for (int i = 0; i < s_glSupport.maxVertexAttribs; i++) {
        map->insert(
                ArraysMap::value_type(
                    i,
                    new GLESpointer()));
    }
    assert(m_vaoStateMap.count(array) == 0);  // Overwriting existing entry, leaking memory
    m_vaoStateMap[array] = VAOState(0, map, std::max(s_glSupport.maxVertexAttribs, s_glSupport.maxVertexAttribBindings));
}

void GLEScontext::removeVertexArrayObject(GLuint array) {
    if (array == 0) return;
    if (m_vaoStateMap.find(array) == m_vaoStateMap.end())
        return;
    if (array == m_currVaoState.vaoId()) {
        setVertexArrayObject(0);
    }

    auto& state = m_vaoStateMap[array];

    if (state.arraysMap) {
        for (auto elem : *(state.arraysMap)) {
            delete elem.second;
        }
    }

    m_vaoStateMap.erase(array);
}

bool GLEScontext::setVertexArrayObject(GLuint array) {
    VAOStateMap::iterator it = m_vaoStateMap.find(array);
    if (it != m_vaoStateMap.end()) {
        m_currVaoState = VAOStateRef(it);
        return true;
    }
    return false;
}

void GLEScontext::setVAOEverBound() {
    m_currVaoState.setEverBound();
}

GLuint GLEScontext::getVertexArrayObject() const {
    return m_currVaoState.vaoId();
}

bool GLEScontext::vertexAttributesBufferBacked() {
    const auto& info = m_currVaoState.attribInfo_const();
    for (uint32_t i = 0; i  < kMaxVertexAttributes; ++i) {
        const auto& pointerInfo = info[i];
        if (pointerInfo.isEnable() &&
            !m_currVaoState.bufferBindings()[pointerInfo.getBindingIndex()].buffer) {
            return false;
        }
    }

    return true;
}

static EGLiface*      s_eglIface = nullptr;

// static
EGLiface* GLEScontext::eglIface() {
    return s_eglIface;
}

// static
void GLEScontext::initEglIface(EGLiface* iface) {
    if (!s_eglIface) s_eglIface = iface;
}

void GLEScontext::initGlobal(EGLiface* iface) {
    initEglIface(iface);
    s_lock.lock();
    if (!s_glExtensions) {
        initCapsLocked(reinterpret_cast<const GLubyte*>(
                getHostExtensionsString(&s_glDispatch).c_str()));
        // NOTE: the string below corresponds to the extensions reported
        // by this context, which is initialized in each GLESv1 or GLESv2
        // context implementation, based on the parsing of the host
        // extensions string performed by initCapsLocked(). I.e. it will
        // be populated after calling this ::init() method.
        s_glExtensions = new std::string();
    }
    s_lock.unlock();
}

void GLEScontext::init() {
    if (!m_initialized) {
        initExtensionString();

        m_maxTexUnits = getMaxCombinedTexUnits();
        m_texState = new textureUnitState[m_maxTexUnits];
        for (int i=0;i<m_maxTexUnits;++i) {
            for (int j=0;j<NUM_TEXTURE_TARGETS;++j)
            {
                m_texState[i][j].texture = 0;
                m_texState[i][j].enabled = GL_FALSE;
            }
        }

        m_indexedTransformFeedbackBuffers.resize(getCaps()->maxTransformFeedbackSeparateAttribs);
        m_indexedUniformBuffers.resize(getCaps()->maxUniformBufferBindings);
        m_indexedAtomicCounterBuffers.resize(getCaps()->maxAtomicCounterBufferBindings);
        m_indexedShaderStorageBuffers.resize(getCaps()->maxShaderStorageBufferBindings);
    }
}

void GLEScontext::restore() {
    postLoadRestoreShareGroup();
    if (m_needRestoreFromSnapshot) {
        postLoadRestoreCtx();
        m_needRestoreFromSnapshot = false;
    }
}

bool GLEScontext::needRestore() {
    bool ret = m_needRestoreFromSnapshot;
    if (m_shareGroup) {
        ret |= m_shareGroup->needRestore();
    }
    return ret;
}

GLenum GLEScontext::getGLerror() {
    return m_glError;
}

void GLEScontext::setGLerror(GLenum err) {
    m_glError = err;
}

void GLEScontext::setActiveTexture(GLenum tex) {
   m_activeTexture = tex - GL_TEXTURE0;
   m_maxUsedTexUnit = std::max(m_activeTexture, m_maxUsedTexUnit);
}

GLEScontext::GLEScontext() {}

GLEScontext::GLEScontext(GlobalNameSpace* globalNameSpace,
        android::base::Stream* stream, GlLibrary* glLib) {
    if (stream) {
        m_initialized = stream->getByte();
        m_glesMajorVersion = stream->getBe32();
        m_glesMinorVersion = stream->getBe32();
        if (m_initialized) {
            m_activeTexture = (GLuint)stream->getBe32();

            loadNameMap<VAOStateMap>(stream, m_vaoStateMap);
            uint32_t vaoId = stream->getBe32();
            setVertexArrayObject(vaoId);

            m_copyReadBuffer = static_cast<GLuint>(stream->getBe32());
            m_copyWriteBuffer = static_cast<GLuint>(stream->getBe32());
            m_pixelPackBuffer = static_cast<GLuint>(stream->getBe32());
            m_pixelUnpackBuffer = static_cast<GLuint>(stream->getBe32());
            m_transformFeedbackBuffer = static_cast<GLuint>(stream->getBe32());
            m_uniformBuffer = static_cast<GLuint>(stream->getBe32());
            m_atomicCounterBuffer = static_cast<GLuint>(stream->getBe32());
            m_dispatchIndirectBuffer = static_cast<GLuint>(stream->getBe32());
            m_drawIndirectBuffer = static_cast<GLuint>(stream->getBe32());
            m_shaderStorageBuffer = static_cast<GLuint>(stream->getBe32());

            loadContainer(stream, m_indexedTransformFeedbackBuffers);
            loadContainer(stream, m_indexedUniformBuffers);
            loadContainer(stream, m_indexedAtomicCounterBuffers);
            loadContainer(stream, m_indexedShaderStorageBuffers);

            // TODO: handle the case where the loaded size and the supported
            // side does not match

            m_isViewport = stream->getByte();
            m_viewportX = static_cast<GLint>(stream->getBe32());
            m_viewportY = static_cast<GLint>(stream->getBe32());
            m_viewportWidth = static_cast<GLsizei>(stream->getBe32());
            m_viewportHeight = static_cast<GLsizei>(stream->getBe32());

            m_polygonOffsetFactor = static_cast<GLfloat>(stream->getFloat());
            m_polygonOffsetUnits = static_cast<GLfloat>(stream->getFloat());

            m_isScissor = stream->getByte();
            m_scissorX = static_cast<GLint>(stream->getBe32());
            m_scissorY = static_cast<GLint>(stream->getBe32());
            m_scissorWidth = static_cast<GLsizei>(stream->getBe32());
            m_scissorHeight = static_cast<GLsizei>(stream->getBe32());

            loadCollection(stream, &m_glEnableList,
                    [](android::base::Stream* stream) {
                        GLenum item = stream->getBe32();
                        bool enabled = stream->getByte();
                        return std::make_pair(item, enabled);
            });

            m_blendEquationRgb = static_cast<GLenum>(stream->getBe32());
            m_blendEquationAlpha = static_cast<GLenum>(stream->getBe32());
            m_blendSrcRgb = static_cast<GLenum>(stream->getBe32());
            m_blendDstRgb = static_cast<GLenum>(stream->getBe32());
            m_blendSrcAlpha = static_cast<GLenum>(stream->getBe32());
            m_blendDstAlpha = static_cast<GLenum>(stream->getBe32());

            loadCollection(stream, &m_glPixelStoreiList,
                    [](android::base::Stream* stream) {
                        GLenum item = stream->getBe32();
                        GLint val = stream->getBe32();
                        return std::make_pair(item, val);
            });

            m_cullFace = static_cast<GLenum>(stream->getBe32());
            m_frontFace = static_cast<GLenum>(stream->getBe32());
            m_depthFunc = static_cast<GLenum>(stream->getBe32());
            m_depthMask = static_cast<GLboolean>(stream->getByte());
            m_zNear = static_cast<GLclampf>(stream->getFloat());
            m_zFar = static_cast<GLclampf>(stream->getFloat());

            m_lineWidth = static_cast<GLclampf>(stream->getFloat());

            m_sampleCoverageVal = static_cast<GLclampf>(stream->getFloat());
            m_sampleCoverageInvert = static_cast<GLboolean>(stream->getByte());

            stream->read(m_stencilStates, sizeof(m_stencilStates));

            m_colorMaskR = static_cast<GLboolean>(stream->getByte());
            m_colorMaskG = static_cast<GLboolean>(stream->getByte());
            m_colorMaskB = static_cast<GLboolean>(stream->getByte());
            m_colorMaskA = static_cast<GLboolean>(stream->getByte());

            m_clearColorR = static_cast<GLclampf>(stream->getFloat());
            m_clearColorG = static_cast<GLclampf>(stream->getFloat());
            m_clearColorB = static_cast<GLclampf>(stream->getFloat());
            m_clearColorA = static_cast<GLclampf>(stream->getFloat());

            m_clearDepth = static_cast<GLclampf>(stream->getFloat());
            m_clearStencil = static_cast<GLint>(stream->getBe32());

            // share group is supposed to be loaded by EglContext and reset
            // when loading EglContext
            //int sharegroupId = stream->getBe32();
            m_glError = static_cast<GLenum>(stream->getBe32());
            m_maxTexUnits = static_cast<int>(stream->getBe32());
            m_maxUsedTexUnit = static_cast<int>(stream->getBe32());
            m_texState = new textureUnitState[m_maxTexUnits];
            stream->read(m_texState, sizeof(textureUnitState) * m_maxTexUnits);
            m_arrayBuffer = static_cast<unsigned int>(stream->getBe32());
            m_elementBuffer = static_cast<unsigned int>(stream->getBe32());
            m_renderbuffer = static_cast<GLuint>(stream->getBe32());
            m_drawFramebuffer = static_cast<GLuint>(stream->getBe32());
            m_readFramebuffer = static_cast<GLuint>(stream->getBe32());
            m_defaultFBODrawBuffer = static_cast<GLenum>(stream->getBe32());
            m_defaultFBOReadBuffer = static_cast<GLenum>(stream->getBe32());

            m_needRestoreFromSnapshot = true;
        }
    }
    ObjectData::loadObject_t loader = [this](NamedObjectType type,
                                             long long unsigned int localName,
                                             android::base::Stream* stream) {
        return loadObject(type, localName, stream);
    };
    m_fboNameSpace = new NameSpace(NamedObjectType::FRAMEBUFFER,
                                   globalNameSpace, stream, loader);
    // do not load m_vaoNameSpace
    m_vaoNameSpace = new NameSpace(NamedObjectType::VERTEX_ARRAY_OBJECT,
                                   globalNameSpace, nullptr, loader);
}

GLEScontext::~GLEScontext() {
    auto& gl = dispatcher();

    if (m_blitState.program) {
        gl.glDeleteProgram(m_blitState.program);
        gl.glDeleteTextures(1, &m_blitState.tex);
        gl.glDeleteVertexArrays(1, &m_blitState.vao);
        gl.glDeleteBuffers(1, &m_blitState.vbo);
        gl.glDeleteFramebuffers(1, &m_blitState.fbo);
    }

    if (m_textureEmulationProg) {
        gl.glDeleteProgram(m_textureEmulationProg);
        gl.glDeleteTextures(2, m_textureEmulationTextures);
        gl.glDeleteFramebuffers(1, &m_textureEmulationFBO);
        gl.glDeleteVertexArrays(1, &m_textureEmulationVAO);
    }

    if (m_defaultFBO) {
        gl.glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
        gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, 0);
        gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
        gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
        gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.glDeleteFramebuffers(1, &m_defaultFBO);
    }

    if (m_defaultReadFBO && (m_defaultReadFBO != m_defaultFBO)) {
        gl.glBindFramebuffer(GL_READ_FRAMEBUFFER, m_defaultReadFBO);
        gl.glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, 0);
        gl.glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
        gl.glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
        gl.glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        gl.glDeleteFramebuffers(1, &m_defaultReadFBO);
    }

    m_defaultFBO = 0;
    m_defaultReadFBO = 0;

    for (auto&& vao : m_vaoStateMap) {
        if (vao.second.arraysMap) {
            for (auto elem : *(vao.second.arraysMap)) {
                delete elem.second;
            }
            vao.second.arraysMap.reset();
        }
    }

    delete[] m_texState;
    m_texState = nullptr;
    delete m_fboNameSpace;
    m_fboNameSpace = nullptr;
    delete m_vaoNameSpace;
    m_vaoNameSpace = nullptr;
}

void GLEScontext::postLoad() {
    m_fboNameSpace->postLoad(
            [this](NamedObjectType p_type, ObjectLocalName p_localName) {
                if (p_type == NamedObjectType::FRAMEBUFFER) {
                    return this->getFBODataPtr(p_localName);
                } else {
                    return m_shareGroup->getObjectDataPtr(p_type, p_localName);
                }
            });
}

void GLEScontext::onSave(android::base::Stream* stream) const {
    stream->putByte(m_initialized);
    stream->putBe32(m_glesMajorVersion);
    stream->putBe32(m_glesMinorVersion);
    if (m_initialized) {
        stream->putBe32(m_activeTexture);

        saveNameMap(stream, m_vaoStateMap);
        stream->putBe32(getVertexArrayObject());

        stream->putBe32(m_copyReadBuffer);
        stream->putBe32(m_copyWriteBuffer);
        stream->putBe32(m_pixelPackBuffer);
        stream->putBe32(m_pixelUnpackBuffer);
        stream->putBe32(m_transformFeedbackBuffer);
        stream->putBe32(m_uniformBuffer);
        stream->putBe32(m_atomicCounterBuffer);
        stream->putBe32(m_dispatchIndirectBuffer);
        stream->putBe32(m_drawIndirectBuffer);
        stream->putBe32(m_shaderStorageBuffer);

        saveContainer(stream, m_indexedTransformFeedbackBuffers);
        saveContainer(stream, m_indexedUniformBuffers);
        saveContainer(stream, m_indexedAtomicCounterBuffers);
        saveContainer(stream, m_indexedShaderStorageBuffers);

        stream->putByte(m_isViewport);
        stream->putBe32(m_viewportX);
        stream->putBe32(m_viewportY);
        stream->putBe32(m_viewportWidth);
        stream->putBe32(m_viewportHeight);

        stream->putFloat(m_polygonOffsetFactor);
        stream->putFloat(m_polygonOffsetUnits);

        stream->putByte(m_isScissor);
        stream->putBe32(m_scissorX);
        stream->putBe32(m_scissorY);
        stream->putBe32(m_scissorWidth);
        stream->putBe32(m_scissorHeight);

        saveCollection(stream, m_glEnableList, [](android::base::Stream* stream,
                const std::pair<const GLenum, bool>& enableItem) {
                    stream->putBe32(enableItem.first);
                    stream->putByte(enableItem.second);
        });

        stream->putBe32(m_blendEquationRgb);
        stream->putBe32(m_blendEquationAlpha);
        stream->putBe32(m_blendSrcRgb);
        stream->putBe32(m_blendDstRgb);
        stream->putBe32(m_blendSrcAlpha);
        stream->putBe32(m_blendDstAlpha);

        saveCollection(stream, m_glPixelStoreiList, [](android::base::Stream* stream,
                const std::pair<const GLenum, GLint>& pixelStore) {
                    stream->putBe32(pixelStore.first);
                    stream->putBe32(pixelStore.second);
        });

        stream->putBe32(m_cullFace);
        stream->putBe32(m_frontFace);
        stream->putBe32(m_depthFunc);
        stream->putByte(m_depthMask);
        stream->putFloat(m_zNear);
        stream->putFloat(m_zFar);

        stream->putFloat(m_lineWidth);

        stream->putFloat(m_sampleCoverageVal);
        stream->putByte(m_sampleCoverageInvert);

        stream->write(m_stencilStates, sizeof(m_stencilStates));

        stream->putByte(m_colorMaskR);
        stream->putByte(m_colorMaskG);
        stream->putByte(m_colorMaskB);
        stream->putByte(m_colorMaskA);

        stream->putFloat(m_clearColorR);
        stream->putFloat(m_clearColorG);
        stream->putFloat(m_clearColorB);
        stream->putFloat(m_clearColorA);

        stream->putFloat(m_clearDepth);
        stream->putBe32(m_clearStencil);

        // share group is supposed to be saved / loaded by EglContext
        stream->putBe32(m_glError);
        stream->putBe32(m_maxTexUnits);
        stream->putBe32(m_maxUsedTexUnit);
        stream->write(m_texState, sizeof(textureUnitState) * m_maxTexUnits);
        stream->putBe32(m_arrayBuffer);
        stream->putBe32(m_elementBuffer);
        stream->putBe32(m_renderbuffer);
        stream->putBe32(m_drawFramebuffer);
        stream->putBe32(m_readFramebuffer);
        stream->putBe32(m_defaultFBODrawBuffer);
        stream->putBe32(m_defaultFBOReadBuffer);
    }
    m_fboNameSpace->onSave(stream);
    // do not save m_vaoNameSpace
}

void GLEScontext::postSave(android::base::Stream* stream) const {
    (void)stream;
    // We need to mark the textures dirty, for those that has been bound to
    // a potential render target.
    for (ObjectDataMap::const_iterator it = m_fboNameSpace->objDataMapBegin();
        it != m_fboNameSpace->objDataMapEnd();
        it ++) {
        FramebufferData* fbData = (FramebufferData*)it->second.get();
        fbData->makeTextureDirty([this](NamedObjectType p_type,
            ObjectLocalName p_localName) {
                if (p_type == NamedObjectType::FRAMEBUFFER) {
                    return this->getFBODataPtr(p_localName);
                } else {
                    return m_shareGroup->getObjectDataPtr(p_type, p_localName);
                }
            });
    }
}

void GLEScontext::postLoadRestoreShareGroup() {
    m_shareGroup->postLoadRestore();
}

void GLEScontext::postLoadRestoreCtx() {
    GLDispatch& dispatcher = GLEScontext::dispatcher();

    assert(!m_shareGroup->needRestore());
    m_fboNameSpace->postLoadRestore(
            [this](NamedObjectType p_type, ObjectLocalName p_localName) {
                if (p_type == NamedObjectType::FRAMEBUFFER) {
                    return getFBOGlobalName(p_localName);
                } else {
                    return m_shareGroup->getGlobalName(p_type, p_localName);
                }
            }
        );

    // buffer bindings
    auto bindBuffer = [this](GLenum target, GLuint buffer) {
        this->dispatcher().glBindBuffer(target,
                m_shareGroup->getGlobalName(NamedObjectType::VERTEXBUFFER, buffer));
    };
    bindBuffer(GL_ARRAY_BUFFER, m_arrayBuffer);
    bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_currVaoState.iboId());

    // framebuffer binding
    auto bindFrameBuffer = [this](GLenum target, GLuint buffer) {
        this->dispatcher().glBindFramebuffer(target,
                getFBOGlobalName(buffer));
    };
    bindFrameBuffer(GL_READ_FRAMEBUFFER, m_readFramebuffer);
    bindFrameBuffer(GL_DRAW_FRAMEBUFFER, m_drawFramebuffer);

    for (unsigned int i = 0; i <= m_maxUsedTexUnit; i++) {
        for (unsigned int j = 0; j < NUM_TEXTURE_TARGETS; j++) {
            textureTargetState& texState = m_texState[i][j];
            if (texState.texture || texState.enabled) {
                this->dispatcher().glActiveTexture(i + GL_TEXTURE0);
                GLenum texTarget = GL_TEXTURE_2D;
                switch (j) {
                    case TEXTURE_2D:
                        texTarget = GL_TEXTURE_2D;
                        break;
                    case TEXTURE_CUBE_MAP:
                        texTarget = GL_TEXTURE_CUBE_MAP;
                        break;
                    case TEXTURE_2D_ARRAY:
                        texTarget = GL_TEXTURE_2D_ARRAY;
                        break;
                    case TEXTURE_3D:
                        texTarget = GL_TEXTURE_3D;
                        break;
                    case TEXTURE_2D_MULTISAMPLE:
                        texTarget = GL_TEXTURE_2D_MULTISAMPLE;
                        break;
                    default:
                        fprintf(stderr,
                                "Warning: unsupported texture target 0x%x.\n",
                                j);
                        break;
                }
                // TODO: refactor the following line since it is duplicated in
                // GLESv2Imp and GLEScmImp as well
                ObjectLocalName texName = texState.texture != 0 ?
                        texState.texture : getDefaultTextureName(texTarget);
                this->dispatcher().glBindTexture(
                        texTarget,
                        m_shareGroup->getGlobalName(
                            NamedObjectType::TEXTURE, texName));
                if (!isCoreProfile() && texState.enabled) {
                    dispatcher.glEnable(texTarget);
                }
            }
        }
    }
    dispatcher.glActiveTexture(m_activeTexture + GL_TEXTURE0);

    // viewport & scissor
    if (m_isViewport) {
        dispatcher.glViewport(m_viewportX, m_viewportY,
                m_viewportWidth, m_viewportHeight);
    }
    if (m_isScissor) {
        dispatcher.glScissor(m_scissorX, m_scissorY,
                m_scissorWidth, m_scissorHeight);
    }
    dispatcher.glPolygonOffset(m_polygonOffsetFactor,
            m_polygonOffsetUnits);

    for (auto item : m_glEnableList) {
        if (item.first == GL_TEXTURE_2D
                || item.first == GL_TEXTURE_CUBE_MAP_OES) {
            continue;
        }
        std::function<void(GLenum)> enableFunc = item.second ? dispatcher.glEnable :
                                                   dispatcher.glDisable;
        if (item.first==GL_TEXTURE_GEN_STR_OES) {
            enableFunc(GL_TEXTURE_GEN_S);
            enableFunc(GL_TEXTURE_GEN_T);
            enableFunc(GL_TEXTURE_GEN_R);
        } else {
            enableFunc(item.first);
        }
    }
    dispatcher.glBlendEquationSeparate(m_blendEquationRgb,
            m_blendEquationAlpha);
    dispatcher.glBlendFuncSeparate(m_blendSrcRgb, m_blendDstRgb,
            m_blendSrcAlpha, m_blendDstAlpha);
    for (const auto& pixelStore : m_glPixelStoreiList) {
        dispatcher.glPixelStorei(pixelStore.first, pixelStore.second);
    }

    dispatcher.glCullFace(m_cullFace);
    dispatcher.glFrontFace(m_frontFace);
    dispatcher.glDepthFunc(m_depthFunc);
    dispatcher.glDepthMask(m_depthMask);

    dispatcher.glLineWidth(m_lineWidth);

    dispatcher.glSampleCoverage(m_sampleCoverageVal, m_sampleCoverageInvert);

    for (int i = 0; i < 2; i++) {
        GLenum face = i == StencilFront ? GL_FRONT
                                       : GL_BACK;
        dispatcher.glStencilFuncSeparate(face, m_stencilStates[i].m_func,
                m_stencilStates[i].m_ref, m_stencilStates[i].m_funcMask);
        dispatcher.glStencilMaskSeparate(face, m_stencilStates[i].m_writeMask);
        dispatcher.glStencilOpSeparate(face, m_stencilStates[i].m_sfail,
                m_stencilStates[i].m_dpfail, m_stencilStates[i].m_dppass);
    }

    dispatcher.glClearColor(m_clearColorR, m_clearColorG, m_clearColorB,
            m_clearColorA);
    if (isGles2Gles()) {
        dispatcher.glClearDepthf(m_clearDepth);
        dispatcher.glDepthRangef(m_zNear, m_zFar);
    } else {
        dispatcher.glClearDepth(m_clearDepth);
        dispatcher.glDepthRange(m_zNear, m_zFar);
    }
    dispatcher.glClearStencil(m_clearStencil);
    dispatcher.glColorMask(m_colorMaskR, m_colorMaskG, m_colorMaskB,
            m_colorMaskA);

    // report any GL errors when loading from a snapshot
    GLenum err = 0;
    do {
        err = dispatcher.glGetError();
#ifdef _DEBUG
        if (err) {
            fprintf(stderr,
                    "warning: get GL error %d while restoring a snapshot\n",
                    err);
        }
#endif
    } while (err != 0);
}

ObjectDataPtr GLEScontext::loadObject(NamedObjectType type,
            ObjectLocalName localName, android::base::Stream* stream) const {
    switch (type) {
        case NamedObjectType::VERTEXBUFFER:
            return ObjectDataPtr(new GLESbuffer(stream));
        case NamedObjectType::TEXTURE:
            return ObjectDataPtr(new TextureData(stream));
        case NamedObjectType::FRAMEBUFFER:
            return ObjectDataPtr(new FramebufferData(stream));
        case NamedObjectType::RENDERBUFFER:
            return ObjectDataPtr(new RenderbufferData(stream));
        default:
            return {};
    }
}

const GLvoid* GLEScontext::setPointer(GLenum arrType,GLint size,GLenum type,GLsizei stride,const GLvoid* data, GLsizei dataSize, bool normalize, bool isInt) {
    GLuint bufferName = m_arrayBuffer;
    GLESpointer* glesPointer = nullptr;

    if (m_currVaoState.it->second.legacy) {
        auto vertexAttrib = m_currVaoState.find(arrType);
        if (vertexAttrib == m_currVaoState.end()) {
            return nullptr;
        }
        glesPointer = m_currVaoState[arrType];
    } else {
        uint32_t attribIndex = (uint32_t)arrType;
        if (attribIndex > kMaxVertexAttributes) return nullptr;
        glesPointer = m_currVaoState.attribInfo().data() + (uint32_t)arrType;
    }

    if(bufferName) {
        unsigned int offset = SafeUIntFromPointer(data);
        GLESbuffer* vbo = static_cast<GLESbuffer*>(
                m_shareGroup
                        ->getObjectData(NamedObjectType::VERTEXBUFFER,
                                        bufferName));
        if(offset >= vbo->getSize() || vbo->getSize() - offset < size) {
#ifdef _DEBUG
            fprintf(stderr, "Warning: Invalid pointer offset %u, arrType %d, type %d\n", offset, arrType, type);
#endif
            return nullptr;
        }

        glesPointer->setBuffer(size,type,stride,vbo,bufferName,offset,normalize, isInt);

        return  static_cast<const unsigned char*>(vbo->getData()) +  offset;
    }
    glesPointer->setArray(size,type,stride,data,dataSize,normalize,isInt);
    return data;
}

GLint GLEScontext::getUnpackAlignment() {
    return android::base::findOrDefault(m_glPixelStoreiList,
            GL_UNPACK_ALIGNMENT, 4);
}

void GLEScontext::enableArr(GLenum arr,bool enable) {
    auto vertexAttrib = m_currVaoState.find(arr);
    if (vertexAttrib != m_currVaoState.end()) {
        vertexAttrib->second->enable(enable);
    }
}

bool GLEScontext::isArrEnabled(GLenum arr) {
    if (m_currVaoState.it->second.legacy) {
        return m_currVaoState[arr]->isEnable();
    } else {
        if ((uint32_t)arr > kMaxVertexAttributes) return false;
        return m_currVaoState.attribInfo()[(uint32_t)arr].isEnable();
    }
}

const GLESpointer* GLEScontext::getPointer(GLenum arrType) {
    const auto it = m_currVaoState.find(arrType);
    return it != m_currVaoState.end() ? it->second : nullptr;
}

static void convertFixedDirectLoop(const char* dataIn,unsigned int strideIn,void* dataOut,unsigned int nBytes,unsigned int strideOut,int attribSize) {

    for(unsigned int i = 0; i < nBytes;i+=strideOut) {
        const GLfixed* fixed_data = (const GLfixed *)dataIn;
        //filling attrib
        for(int j=0;j<attribSize;j++) {
            reinterpret_cast<GLfloat*>(&static_cast<unsigned char*>(dataOut)[i])[j] = X2F(fixed_data[j]);
        }
        dataIn += strideIn;
    }
}

static void convertFixedIndirectLoop(const char* dataIn,unsigned int strideIn,void* dataOut,GLsizei count,GLenum indices_type,const GLvoid* indices,unsigned int strideOut,int attribSize) {
    for(int i = 0 ;i < count ;i++) {
        GLuint index = getIndex(indices_type, indices, i);

        const GLfixed* fixed_data = (GLfixed *)(dataIn  + index*strideIn);
        GLfloat* float_data = reinterpret_cast<GLfloat*>(static_cast<unsigned char*>(dataOut) + index*strideOut);

        for(int j=0;j<attribSize;j++) {
            float_data[j] = X2F(fixed_data[j]);
         }
    }
}

static void convertByteDirectLoop(const char* dataIn,unsigned int strideIn,void* dataOut,unsigned int nBytes,unsigned int strideOut,int attribSize) {

    for(unsigned int i = 0; i < nBytes;i+=strideOut) {
        const GLbyte* byte_data = (const GLbyte *)dataIn;
        //filling attrib
        for(int j=0;j<attribSize;j++) {
            reinterpret_cast<GLshort*>(&static_cast<unsigned char*>(dataOut)[i])[j] = B2S(byte_data[j]);
        }
        dataIn += strideIn;
    }
}

static void convertByteIndirectLoop(const char* dataIn,unsigned int strideIn,void* dataOut,GLsizei count,GLenum indices_type,const GLvoid* indices,unsigned int strideOut,int attribSize) {
    for(int i = 0 ;i < count ;i++) {
        GLuint index = getIndex(indices_type, indices, i);
        const GLbyte* bytes_data = (GLbyte *)(dataIn  + index*strideIn);
        GLshort* short_data = reinterpret_cast<GLshort*>(static_cast<unsigned char*>(dataOut) + index*strideOut);

        for(int j=0;j<attribSize;j++) {
            short_data[j] = B2S(bytes_data[j]);
         }
    }
}
static void directToBytesRanges(GLint first,GLsizei count,GLESpointer* p,RangeList& list) {

    int attribSize = p->getSize()*4; //4 is the sizeof GLfixed or GLfloat in bytes
    int stride = p->getStride()?p->getStride():attribSize;
    int start  = p->getBufferOffset()+first*stride;
    if(!p->getStride()) {
        list.addRange(Range(start,count*attribSize));
    } else {
        for(int i = 0 ;i < count; i++,start+=stride) {
            list.addRange(Range(start,attribSize));
        }
    }
}

static void indirectToBytesRanges(const GLvoid* indices,GLenum indices_type,GLsizei count,GLESpointer* p,RangeList& list) {

    int attribSize = p->getSize() * 4; //4 is the sizeof GLfixed or GLfloat in bytes
    int stride = p->getStride()?p->getStride():attribSize;
    int start  = p->getBufferOffset();
    for(int i=0 ; i < count; i++) {
        GLuint index = getIndex(indices_type, indices, i);
        list.addRange(Range(start+index*stride,attribSize));

    }
}

int bytesRangesToIndices(RangeList& ranges,GLESpointer* p,GLuint* indices) {

    int attribSize = p->getSize() * 4; //4 is the sizeof GLfixed or GLfloat in bytes
    int stride = p->getStride()?p->getStride():attribSize;
    int offset = p->getBufferOffset();

    int n = 0;
    for(int i=0;i<ranges.size();i++) {
        int startIndex = (ranges[i].getStart() - offset) / stride;
        int nElements = ranges[i].getSize()/attribSize;
        for(int j=0;j<nElements;j++) {
            indices[n++] = startIndex+j;
        }
    }
    return n;
}

void GLEScontext::convertDirect(GLESConversionArrays& cArrs,GLint first,GLsizei count,GLenum array_id,GLESpointer* p) {

    GLenum type    = p->getType();
    int attribSize = p->getSize();
    unsigned int size = attribSize*count + first;
    unsigned int bytes = type == GL_FIXED ? sizeof(GLfixed):sizeof(GLbyte);
    cArrs.allocArr(size,type);
    int stride = p->getStride()?p->getStride():bytes*attribSize;
    const char* data = (const char*)p->getArrayData() + (first*stride);

    if(type == GL_FIXED) {
        convertFixedDirectLoop(data,stride,cArrs.getCurrentData(),size*sizeof(GLfloat),attribSize*sizeof(GLfloat),attribSize);
    } else if(type == GL_BYTE) {
        convertByteDirectLoop(data,stride,cArrs.getCurrentData(),size*sizeof(GLshort),attribSize*sizeof(GLshort),attribSize);
    }
}

void GLEScontext::convertDirectVBO(GLESConversionArrays& cArrs,GLint first,GLsizei count,GLenum array_id,GLESpointer* p) {

    RangeList ranges;
    RangeList conversions;
    GLuint* indices = NULL;
    int attribSize = p->getSize();
    int stride = p->getStride()?p->getStride():sizeof(GLfixed)*attribSize;
    char* data = (char*)p->getBufferData();

    if(p->bufferNeedConversion()) {
        directToBytesRanges(first,count,p,ranges); //converting indices range to buffer bytes ranges by offset
        p->getBufferConversions(ranges,conversions); // getting from the buffer the relevant ranges that still needs to be converted

        if(conversions.size()) { // there are some elements to convert
           indices = new GLuint[count];
           int nIndices = bytesRangesToIndices(conversions,p,indices); //converting bytes ranges by offset to indices in this array
           convertFixedIndirectLoop(data,stride,data,nIndices,GL_UNSIGNED_INT,indices,stride,attribSize);
        }
    }
    if(indices) delete[] indices;
    cArrs.setArr(data,p->getStride(),GL_FLOAT);
}

unsigned int GLEScontext::findMaxIndex(GLsizei count,GLenum type,const GLvoid* indices) {
    //finding max index
    unsigned int max = 0;
    if(type == GL_UNSIGNED_BYTE) {
        GLubyte*  b_indices  =(GLubyte *)indices;
        for(int i=0;i<count;i++) {
            if(b_indices[i] > max) max = b_indices[i];
        }
    } else if (type == GL_UNSIGNED_SHORT) {
        GLushort* us_indices =(GLushort *)indices;
        for(int i=0;i<count;i++) {
            if(us_indices[i] > max) max = us_indices[i];
        }
    } else { // type == GL_UNSIGNED_INT
        GLuint* ui_indices =(GLuint *)indices;
        for(int i=0;i<count;i++) {
            if(ui_indices[i] > max) max = ui_indices[i];
        }
    }
    return max;
}

void GLEScontext::convertIndirect(GLESConversionArrays& cArrs,GLsizei count,GLenum indices_type,const GLvoid* indices,GLenum array_id,GLESpointer* p) {
    GLenum type    = p->getType();
    int maxElements = findMaxIndex(count,indices_type,indices) + 1;

    int attribSize = p->getSize();
    int size = attribSize * maxElements;
    unsigned int bytes = type == GL_FIXED ? sizeof(GLfixed):sizeof(GLbyte);
    cArrs.allocArr(size,type);
    int stride = p->getStride()?p->getStride():bytes*attribSize;

    const char* data = (const char*)p->getArrayData();
    if(type == GL_FIXED) {
        convertFixedIndirectLoop(data,stride,cArrs.getCurrentData(),count,indices_type,indices,attribSize*sizeof(GLfloat),attribSize);
    } else if(type == GL_BYTE){
        convertByteIndirectLoop(data,stride,cArrs.getCurrentData(),count,indices_type,indices,attribSize*sizeof(GLshort),attribSize);
    }
}

void GLEScontext::convertIndirectVBO(GLESConversionArrays& cArrs,GLsizei count,GLenum indices_type,const GLvoid* indices,GLenum array_id,GLESpointer* p) {
    RangeList ranges;
    RangeList conversions;
    GLuint* conversionIndices = NULL;
    int attribSize = p->getSize();
    int stride = p->getStride()?p->getStride():sizeof(GLfixed)*attribSize;
    char* data = static_cast<char*>(p->getBufferData());
    if(p->bufferNeedConversion()) {
        indirectToBytesRanges(indices,indices_type,count,p,ranges); //converting indices range to buffer bytes ranges by offset
        p->getBufferConversions(ranges,conversions); // getting from the buffer the relevant ranges that still needs to be converted
        if(conversions.size()) { // there are some elements to convert
            conversionIndices = new GLuint[count];
            int nIndices = bytesRangesToIndices(conversions,p,conversionIndices); //converting bytes ranges by offset to indices in this array
            convertFixedIndirectLoop(data,stride,data,nIndices,GL_UNSIGNED_INT,conversionIndices,stride,attribSize);
        }
    }
    if(conversionIndices) delete[] conversionIndices;
    cArrs.setArr(data,p->getStride(),GL_FLOAT);
}

void GLEScontext::bindBuffer(GLenum target,GLuint buffer) {
    switch(target) {
    case GL_ARRAY_BUFFER:
        m_arrayBuffer = buffer;
        break;
    case GL_ELEMENT_ARRAY_BUFFER:
        m_currVaoState.iboId() = buffer;
        break;
    case GL_COPY_READ_BUFFER:
        m_copyReadBuffer = buffer;
        break;
    case GL_COPY_WRITE_BUFFER:
        m_copyWriteBuffer = buffer;
        break;
    case GL_PIXEL_PACK_BUFFER:
        m_pixelPackBuffer = buffer;
        break;
    case GL_PIXEL_UNPACK_BUFFER:
        m_pixelUnpackBuffer = buffer;
        break;
    case GL_TRANSFORM_FEEDBACK_BUFFER:
        m_transformFeedbackBuffer = buffer;
        break;
    case GL_UNIFORM_BUFFER:
        m_uniformBuffer = buffer;
        break;
    case GL_ATOMIC_COUNTER_BUFFER:
        m_atomicCounterBuffer = buffer;
        break;
    case GL_DISPATCH_INDIRECT_BUFFER:
        m_dispatchIndirectBuffer = buffer;
        break;
    case GL_DRAW_INDIRECT_BUFFER:
        m_drawIndirectBuffer = buffer;
        break;
    case GL_SHADER_STORAGE_BUFFER:
        m_shaderStorageBuffer = buffer;
        break;
    default:
        m_arrayBuffer = buffer;
        break;
    }
}

void GLEScontext::bindIndexedBuffer(GLenum target, GLuint index, GLuint buffer,
        GLintptr offset, GLsizeiptr size, GLintptr stride, bool isBindBase) {
    VertexAttribBindingVector* bindings = nullptr;
    switch (target) {
    case GL_UNIFORM_BUFFER:
        bindings = &m_indexedUniformBuffers;
        break;
    case GL_ATOMIC_COUNTER_BUFFER:
        bindings = &m_indexedAtomicCounterBuffers;
        break;
    case GL_SHADER_STORAGE_BUFFER:
        bindings = &m_indexedShaderStorageBuffers;
        break;
    default:
        bindings = &m_currVaoState.bufferBindings();
        break;
    }
    if (index >= bindings->size()) {
            return;
    }
    auto& bufferBinding = (*bindings)[index];
    bufferBinding.buffer = buffer;
    bufferBinding.offset = offset;
    bufferBinding.size = size;
    bufferBinding.stride = stride;
    bufferBinding.isBindBase = isBindBase;
}

void GLEScontext::bindIndexedBuffer(GLenum target, GLuint index, GLuint buffer) {
    GLint sz;
    getBufferSizeById(buffer, &sz);
    bindIndexedBuffer(target, index, buffer, 0, sz, 0, true);
}

static void sClearIndexedBufferBinding(GLuint id, std::vector<BufferBinding>& bindings) {
    for (size_t i = 0; i < bindings.size(); i++) {
        if (bindings[i].buffer == id) {
            bindings[i].offset = 0;
            bindings[i].size = 0;
            bindings[i].stride = 0;
            bindings[i].buffer = 0;
            bindings[i].isBindBase = false;
        }
    }
}

void GLEScontext::unbindBuffer(GLuint buffer) {
    if (m_arrayBuffer == buffer)
        m_arrayBuffer = 0;
    if (m_currVaoState.iboId() == buffer)
        m_currVaoState.iboId() = 0;
    if (m_copyReadBuffer == buffer)
        m_copyReadBuffer = 0;
    if (m_copyWriteBuffer == buffer)
        m_copyWriteBuffer = 0;
    if (m_pixelPackBuffer == buffer)
        m_pixelPackBuffer = 0;
    if (m_pixelUnpackBuffer == buffer)
        m_pixelUnpackBuffer = 0;
    if (m_transformFeedbackBuffer == buffer)
        m_transformFeedbackBuffer = 0;
    if (m_uniformBuffer == buffer)
        m_uniformBuffer = 0;
    if (m_atomicCounterBuffer == buffer)
        m_atomicCounterBuffer = 0;
    if (m_dispatchIndirectBuffer == buffer)
        m_dispatchIndirectBuffer = 0;
    if (m_drawIndirectBuffer == buffer)
        m_drawIndirectBuffer = 0;
    if (m_shaderStorageBuffer == buffer)
        m_shaderStorageBuffer = 0;

    // One might think that indexed buffer bindings for transform feedbacks
    // must be cleared as well, but transform feedbacks are
    // considered GL objects with attachments, so even if the buffer is
    // deleted (unbindBuffer is called), the state query with
    // glGetIntegeri_v must still return the deleted name [1].
    // sClearIndexedBufferBinding(buffer, m_indexedTransformFeedbackBuffers);
    // [1] OpenGL ES 3.0.5 spec Appendix D.1.3
    sClearIndexedBufferBinding(buffer, m_indexedUniformBuffers);
    sClearIndexedBufferBinding(buffer, m_indexedAtomicCounterBuffers);
    sClearIndexedBufferBinding(buffer, m_indexedShaderStorageBuffers);
    sClearIndexedBufferBinding(buffer, m_currVaoState.bufferBindings());
}

//checks if any buffer is binded to target
bool GLEScontext::isBindedBuffer(GLenum target) {
    switch(target) {
    case GL_ARRAY_BUFFER:
        return m_arrayBuffer != 0;
    case GL_ELEMENT_ARRAY_BUFFER:
        return m_currVaoState.iboId() != 0;
    case GL_COPY_READ_BUFFER:
        return m_copyReadBuffer != 0;
    case GL_COPY_WRITE_BUFFER:
        return m_copyWriteBuffer != 0;
    case GL_PIXEL_PACK_BUFFER:
        return m_pixelPackBuffer != 0;
    case GL_PIXEL_UNPACK_BUFFER:
        return m_pixelUnpackBuffer != 0;
    case GL_TRANSFORM_FEEDBACK_BUFFER:
        return m_transformFeedbackBuffer != 0;
    case GL_UNIFORM_BUFFER:
        return m_uniformBuffer != 0;
    case GL_ATOMIC_COUNTER_BUFFER:
        return m_atomicCounterBuffer != 0;
    case GL_DISPATCH_INDIRECT_BUFFER:
        return m_dispatchIndirectBuffer != 0;
    case GL_DRAW_INDIRECT_BUFFER:
        return m_drawIndirectBuffer != 0;
    case GL_SHADER_STORAGE_BUFFER:
        return m_shaderStorageBuffer != 0;
    default:
        return m_arrayBuffer != 0;
    }
}

GLuint GLEScontext::getBuffer(GLenum target) {
    switch(target) {
    case GL_ARRAY_BUFFER:
        return m_arrayBuffer;
    case GL_ELEMENT_ARRAY_BUFFER:
        return m_currVaoState.iboId();
    case GL_COPY_READ_BUFFER:
        return m_copyReadBuffer;
    case GL_COPY_WRITE_BUFFER:
        return m_copyWriteBuffer;
    case GL_PIXEL_PACK_BUFFER:
        return m_pixelPackBuffer;
    case GL_PIXEL_UNPACK_BUFFER:
        return m_pixelUnpackBuffer;
    case GL_TRANSFORM_FEEDBACK_BUFFER:
        return m_transformFeedbackBuffer;
    case GL_UNIFORM_BUFFER:
        return m_uniformBuffer;
    case GL_ATOMIC_COUNTER_BUFFER:
        return m_atomicCounterBuffer;
    case GL_DISPATCH_INDIRECT_BUFFER:
        return m_dispatchIndirectBuffer;
    case GL_DRAW_INDIRECT_BUFFER:
        return m_drawIndirectBuffer;
    case GL_SHADER_STORAGE_BUFFER:
        return m_shaderStorageBuffer;
    default:
        return m_arrayBuffer;
    }
}

GLuint GLEScontext::getIndexedBuffer(GLenum target, GLuint index) {
    switch (target) {
    case GL_UNIFORM_BUFFER:
        return m_indexedUniformBuffers[index].buffer;
    case GL_ATOMIC_COUNTER_BUFFER:
        return m_indexedAtomicCounterBuffers[index].buffer;
    case GL_SHADER_STORAGE_BUFFER:
        return m_indexedShaderStorageBuffers[index].buffer;
    default:
        return m_currVaoState.bufferBindings()[index].buffer;
    }
}


GLvoid* GLEScontext::getBindedBuffer(GLenum target) {
    GLuint bufferName = getBuffer(target);
    if(!bufferName) return NULL;

    GLESbuffer* vbo = static_cast<GLESbuffer*>(
            m_shareGroup
                    ->getObjectData(NamedObjectType::VERTEXBUFFER, bufferName));
    return vbo->getData();
}

void GLEScontext::getBufferSize(GLenum target,GLint* param) {
    GLuint bufferName = getBuffer(target);
    getBufferSizeById(bufferName, param);
}

void GLEScontext::getBufferSizeById(GLuint bufferName, GLint* param) {
    if (!bufferName) { *param = 0; return; }
    GLESbuffer* vbo = static_cast<GLESbuffer*>(
            m_shareGroup
                    ->getObjectData(NamedObjectType::VERTEXBUFFER, bufferName));
    *param = vbo->getSize();
}

void GLEScontext::getBufferUsage(GLenum target,GLint* param) {
    GLuint bufferName = getBuffer(target);
    GLESbuffer* vbo = static_cast<GLESbuffer*>(
            m_shareGroup
                    ->getObjectData(NamedObjectType::VERTEXBUFFER, bufferName));
    *param = vbo->getUsage();
}

bool GLEScontext::setBufferData(GLenum target,GLsizeiptr size,const GLvoid* data,GLenum usage) {
    GLuint bufferName = getBuffer(target);
    if(!bufferName) return false;
    GLESbuffer* vbo = static_cast<GLESbuffer*>(
            m_shareGroup
                    ->getObjectData(NamedObjectType::VERTEXBUFFER, bufferName));
    return vbo->setBuffer(size,usage,data);
}

bool GLEScontext::setBufferSubData(GLenum target,GLintptr offset,GLsizeiptr size,const GLvoid* data) {

    GLuint bufferName = getBuffer(target);
    if(!bufferName) return false;
    GLESbuffer* vbo = static_cast<GLESbuffer*>(
            m_shareGroup
                    ->getObjectData(NamedObjectType::VERTEXBUFFER, bufferName));
    return vbo->setSubBuffer(offset,size,data);
}

void GLEScontext::setViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    m_isViewport = true;
    m_viewportX = x;
    m_viewportY = y;
    m_viewportWidth = width;
    m_viewportHeight = height;
}

void GLEScontext::getViewport(GLint* params) {
    if (!m_isViewport) {
        dispatcher().glGetIntegerv(GL_VIEWPORT, params);
    } else {
        params[0] = m_viewportX;
        params[1] = m_viewportY;
        params[2] = m_viewportWidth;
        params[3] = m_viewportHeight;
    }
}

void GLEScontext::setScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    m_isScissor = true;
    m_scissorX = x;
    m_scissorY = y;
    m_scissorWidth = width;
    m_scissorHeight = height;
}

void GLEScontext::setPolygonOffset(GLfloat factor, GLfloat units) {
    m_polygonOffsetFactor = factor;
    m_polygonOffsetUnits = units;
}

void GLEScontext::setEnable(GLenum item, bool isEnable) {
    switch (item) {
        case GL_TEXTURE_2D:
        case GL_TEXTURE_CUBE_MAP_OES:
        case GL_TEXTURE_3D:
        case GL_TEXTURE_2D_ARRAY:
        case GL_TEXTURE_2D_MULTISAMPLE:
            setTextureEnabled(item,true);
            break;
        default:
            m_glEnableList[item] = isEnable;
            break;
    }
}

bool GLEScontext::isEnabled(GLenum item) const {
    switch (item) {
        case GL_TEXTURE_2D:
        case GL_TEXTURE_CUBE_MAP_OES:
        case GL_TEXTURE_3D:
        case GL_TEXTURE_2D_ARRAY:
        case GL_TEXTURE_2D_MULTISAMPLE:
            return m_texState[m_activeTexture][GLTextureTargetToLocal(item)].enabled;
        default:
            return android::base::findOrDefault(m_glEnableList, item, false);
    }
}

void GLEScontext::setBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    m_blendEquationRgb = modeRGB;
    m_blendEquationAlpha = modeAlpha;
}

void GLEScontext::setBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB,
            GLenum srcAlpha, GLenum dstAlpha) {
    m_blendSrcRgb = srcRGB;
    m_blendDstRgb = dstRGB;
    m_blendSrcAlpha = srcAlpha;
    m_blendDstAlpha = dstAlpha;
}

void GLEScontext::setPixelStorei(GLenum pname, GLint param) {
    m_glPixelStoreiList[pname] = param;
}

void GLEScontext::setCullFace(GLenum mode) {
    m_cullFace = mode;
}

void GLEScontext::setFrontFace(GLenum mode) {
    m_frontFace = mode;
}

void GLEScontext::setDepthFunc(GLenum func) {
    m_depthFunc = func;
}

void GLEScontext::setDepthMask(GLboolean flag) {
    m_depthMask = flag;
}

void GLEScontext::setDepthRangef(GLclampf zNear, GLclampf zFar) {
    m_zNear = zNear;
    m_zFar = zFar;
}

void GLEScontext::setLineWidth(GLfloat lineWidth) {
    m_lineWidth = lineWidth;
}

void GLEScontext::setSampleCoverage(GLclampf value, GLboolean invert) {
    m_sampleCoverageVal = value;
    m_sampleCoverageInvert = invert;
}
void GLEScontext::setStencilFuncSeparate(GLenum face, GLenum func, GLint ref,
        GLuint mask) {
    if (face == GL_FRONT_AND_BACK) {
        setStencilFuncSeparate(GL_FRONT, func, ref, mask);
        setStencilFuncSeparate(GL_BACK, func, ref, mask);
        return;
    }
    int idx = 0;
    switch (face) {
        case GL_FRONT:
            idx = StencilFront;
            break;
        case GL_BACK:
            idx = StencilBack;
            break;
        default:
            return;
    }
    m_stencilStates[idx].m_func = func;
    m_stencilStates[idx].m_ref = ref;
    m_stencilStates[idx].m_funcMask = mask;
}

void GLEScontext::setStencilMaskSeparate(GLenum face, GLuint mask) {
    if (face == GL_FRONT_AND_BACK) {
        setStencilMaskSeparate(GL_FRONT, mask);
        setStencilMaskSeparate(GL_BACK, mask);
        return;
    }
    int idx = 0;
    switch (face) {
        case GL_FRONT:
            idx = StencilFront;
            break;
        case GL_BACK:
            idx = StencilBack;
            break;
        default:
            return;
    }
    m_stencilStates[idx].m_writeMask = mask;
}

void GLEScontext::setStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail,
        GLenum zpass) {
    if (face == GL_FRONT_AND_BACK) {
        setStencilOpSeparate(GL_FRONT, fail, zfail, zpass);
        setStencilOpSeparate(GL_BACK, fail, zfail, zpass);
        return;
    }
    int idx = 0;
    switch (face) {
        case GL_FRONT:
            idx = StencilFront;
            break;
        case GL_BACK:
            idx = StencilBack;
            break;
        default:
            return;
    }
    m_stencilStates[idx].m_sfail = fail;
    m_stencilStates[idx].m_dpfail = zfail;
    m_stencilStates[idx].m_dppass = zpass;
}

void GLEScontext::setColorMask(GLboolean red, GLboolean green, GLboolean blue,
        GLboolean alpha) {
    m_colorMaskR = red;
    m_colorMaskG = green;
    m_colorMaskB = blue;
    m_colorMaskA = alpha;
}

void GLEScontext::setClearColor(GLclampf red, GLclampf green, GLclampf blue,
        GLclampf alpha) {
    m_clearColorR = red;
    m_clearColorG = green;
    m_clearColorB = blue;
    m_clearColorA = alpha;
}

void GLEScontext::setClearDepth(GLclampf depth) {
    m_clearDepth = depth;
}

void GLEScontext::setClearStencil(GLint s) {
    m_clearStencil = s;
}

const char * GLEScontext::getExtensionString() {
    const char * ret;
    s_lock.lock();
    if (s_glExtensions)
        ret = s_glExtensions->c_str();
    else
        ret="";
    s_lock.unlock();
    return ret;
}

const char * GLEScontext::getVendorString() const {
    return s_glVendor.c_str();
}

const char * GLEScontext::getRendererString() const {
    return s_glRenderer.c_str();
}

const char * GLEScontext::getVersionString() const {
    return s_glVersion.c_str();
}

void GLEScontext::getGlobalLock() {
    s_lock.lock();
}

void GLEScontext::releaseGlobalLock() {
    s_lock.unlock();
}

void GLEScontext::initCapsLocked(const GLubyte * extensionString)
{
    const char* cstring = (const char*)extensionString;

    s_glDispatch.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS,&s_glSupport.maxVertexAttribs);

    if (s_glSupport.maxVertexAttribs > kMaxVertexAttributes) {
        s_glSupport.maxVertexAttribs = kMaxVertexAttributes;
    }

    s_glDispatch.glGetIntegerv(GL_MAX_CLIP_PLANES,&s_glSupport.maxClipPlane);
    s_glDispatch.glGetIntegerv(GL_MAX_LIGHTS,&s_glSupport.maxLights);
    s_glDispatch.glGetIntegerv(GL_MAX_TEXTURE_SIZE,&s_glSupport.maxTexSize);
    s_glDispatch.glGetIntegerv(GL_MAX_TEXTURE_UNITS,&s_glSupport.maxTexUnits);
    // Core profile lacks a fixed-function pipeline with texture units,
    // but we still want glDrawTexOES to work in core profile.
    // So, set it to 8.
    if ((::isCoreProfile() || isGles2Gles()) &&
        !s_glSupport.maxTexUnits) {
        s_glSupport.maxTexUnits = 8;
    }
    s_glDispatch.glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS,&s_glSupport.maxTexImageUnits);
    s_glDispatch.glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &s_glSupport.maxCombinedTexImageUnits);

    s_glDispatch.glGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, &s_glSupport.maxTransformFeedbackSeparateAttribs);
    s_glDispatch.glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &s_glSupport.maxUniformBufferBindings);
    s_glDispatch.glGetIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, &s_glSupport.maxAtomicCounterBufferBindings);
    s_glDispatch.glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &s_glSupport.maxShaderStorageBufferBindings);
    s_glDispatch.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &s_glSupport.maxDrawBuffers);
    s_glDispatch.glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &s_glSupport.maxVertexAttribBindings);

    // Clear GL error in case these enums not supported.
    s_glDispatch.glGetError();

    const GLubyte* glslVersion = s_glDispatch.glGetString(GL_SHADING_LANGUAGE_VERSION);
    s_glSupport.glslVersion = Version((const  char*)(glslVersion));
    const GLubyte* glVersion = s_glDispatch.glGetString(GL_VERSION);

    if (strstr(cstring,"GL_EXT_bgra ")!=NULL ||
        (isGles2Gles() && strstr(cstring, "GL_EXT_texture_format_BGRA8888")) ||
        (!isGles2Gles() && !(Version((const char*)glVersion) < Version("1.2"))))
        s_glSupport.GL_EXT_TEXTURE_FORMAT_BGRA8888 = true;

    if (::isCoreProfile() ||
        strstr(cstring,"GL_EXT_framebuffer_object ")!=NULL)
        s_glSupport.GL_EXT_FRAMEBUFFER_OBJECT = true;

    if (strstr(cstring,"GL_ARB_vertex_blend ")!=NULL)
        s_glSupport.GL_ARB_VERTEX_BLEND = true;

    if (strstr(cstring,"GL_ARB_matrix_palette ")!=NULL)
        s_glSupport.GL_ARB_MATRIX_PALETTE = true;

    if (strstr(cstring,"GL_EXT_packed_depth_stencil ")!=NULL ||
        strstr(cstring,"GL_OES_packed_depth_stencil ")!=NULL)
        s_glSupport.GL_EXT_PACKED_DEPTH_STENCIL = true;

    if (strstr(cstring,"GL_OES_read_format ")!=NULL)
        s_glSupport.GL_OES_READ_FORMAT = true;

    if (strstr(cstring,"GL_ARB_half_float_pixel ")!=NULL ||
        strstr(cstring,"GL_OES_texture_half_float ")!=NULL)
        s_glSupport.GL_ARB_HALF_FLOAT_PIXEL = true;

    if (strstr(cstring,"GL_NV_half_float ")!=NULL)
        s_glSupport.GL_NV_HALF_FLOAT = true;

    if (strstr(cstring,"GL_ARB_half_float_vertex ")!=NULL ||
        strstr(cstring,"GL_OES_vertex_half_float ")!=NULL)
        s_glSupport.GL_ARB_HALF_FLOAT_VERTEX = true;

    if (strstr(cstring,"GL_SGIS_generate_mipmap ")!=NULL)
        s_glSupport.GL_SGIS_GENERATE_MIPMAP = true;

    if (strstr(cstring,"GL_ARB_ES2_compatibility ")!=NULL
            || isGles2Gles())
        s_glSupport.GL_ARB_ES2_COMPATIBILITY = true;

    if (strstr(cstring,"GL_OES_standard_derivatives ")!=NULL)
        s_glSupport.GL_OES_STANDARD_DERIVATIVES = true;

    if (::isCoreProfile() ||
        strstr(cstring,"GL_ARB_texture_non_power_of_two")!=NULL ||
        strstr(cstring,"GL_OES_texture_npot")!=NULL)
        s_glSupport.GL_OES_TEXTURE_NPOT = true;

    if (::isCoreProfile() ||
        strstr(cstring,"GL_ARB_color_buffer_float")!=NULL ||
        strstr(cstring,"GL_EXT_color_buffer_float")!=NULL)
        s_glSupport.ext_GL_EXT_color_buffer_float = true;

    if (::isCoreProfile() ||
        strstr(cstring,"GL_EXT_color_buffer_half_float")!=NULL)
        s_glSupport.ext_GL_EXT_color_buffer_half_float = true;

    if (::isCoreProfile() ||
        strstr(cstring,"GL_EXT_shader_framebuffer_fetch")!=NULL)
        s_glSupport.ext_GL_EXT_shader_framebuffer_fetch = true;

    if (!(Version((const char*)glVersion) < Version("3.0")) || strstr(cstring,"GL_OES_rgb8_rgba8")!=NULL)
        s_glSupport.GL_OES_RGB8_RGBA8 = true;

    if (strstr(cstring, "GL_EXT_memory_object") != NULL) {
        s_glSupport.ext_GL_EXT_memory_object = true;
    }

    if (strstr(cstring, "GL_EXT_semaphore") != NULL) {
        s_glSupport.ext_GL_EXT_semaphore = true;
    }
}

void GLEScontext::buildStrings(const char* baseVendor,
        const char* baseRenderer, const char* baseVersion, const char* version)
{
    static const char VENDOR[]   = {"Google ("};
    static const char RENDERER[] = {"Android Emulator OpenGL ES Translator ("};
    const size_t VENDOR_LEN   = sizeof(VENDOR) - 1;
    const size_t RENDERER_LEN = sizeof(RENDERER) - 1;

    // Sanitize the strings as some OpenGL implementations return NULL
    // when asked the basic questions (this happened at least once on a client
    // machine)
    if (!baseVendor) {
        baseVendor = "N/A";
    }
    if (!baseRenderer) {
        baseRenderer = "N/A";
    }
    if (!baseVersion) {
        baseVersion = "N/A";
    }
    if (!version) {
        version = "N/A";
    }

    size_t baseVendorLen = strlen(baseVendor);
    s_glVendor.clear();
    s_glVendor.reserve(baseVendorLen + VENDOR_LEN + 1);
    s_glVendor.append(VENDOR, VENDOR_LEN);
    s_glVendor.append(baseVendor, baseVendorLen);
    s_glVendor.append(")", 1);

    size_t baseRendererLen = strlen(baseRenderer);
    s_glRenderer.clear();
    s_glRenderer.reserve(baseRendererLen + RENDERER_LEN + 1);
    s_glRenderer.append(RENDERER, RENDERER_LEN);
    s_glRenderer.append(baseRenderer, baseRendererLen);
    s_glRenderer.append(")", 1);

    size_t baseVersionLen = strlen(baseVersion);
    size_t versionLen = strlen(version);
    s_glVersion.clear();
    s_glVersion.reserve(baseVersionLen + versionLen + 3);
    s_glVersion.append(version, versionLen);
    s_glVersion.append(" (", 2);
    s_glVersion.append(baseVersion, baseVersionLen);
    s_glVersion.append(")", 1);
}

bool GLEScontext::isTextureUnitEnabled(GLenum unit) {
    for (int i=0;i<NUM_TEXTURE_TARGETS;++i) {
        if (m_texState[unit-GL_TEXTURE0][i].enabled)
            return true;
    }
    return false;
}

bool GLEScontext::glGetBooleanv(GLenum pname, GLboolean *params)
{
    GLint iParam;

    if(glGetIntegerv(pname, &iParam))
    {
        *params = (iParam != 0);
        return true;
    }

    return false;
}

bool GLEScontext::glGetFixedv(GLenum pname, GLfixed *params)
{
    bool result = false;
    GLint numParams = 1;

    GLint* iParams = new GLint[numParams];
    if (numParams>0 && glGetIntegerv(pname,iParams)) {
        while(numParams >= 0)
        {
            params[numParams] = I2X(iParams[numParams]);
            numParams--;
        }
        result = true;
    }
    delete [] iParams;

    return result;
}

bool GLEScontext::glGetFloatv(GLenum pname, GLfloat *params)
{
    bool result = false;
    GLint numParams = 1;

    GLint* iParams = new GLint[numParams];
    if (numParams>0 && glGetIntegerv(pname,iParams)) {
        while(numParams >= 0)
        {
            params[numParams] = (GLfloat)iParams[numParams];
            numParams--;
        }
        result = true;
    }
    delete [] iParams;

    return result;
}

bool GLEScontext::glGetIntegerv(GLenum pname, GLint *params)
{
    switch(pname)
    {
        case GL_ARRAY_BUFFER_BINDING:
            *params = m_arrayBuffer;
            break;

        case GL_ELEMENT_ARRAY_BUFFER_BINDING:
            *params = m_currVaoState.iboId();
            break;

        case GL_TEXTURE_BINDING_CUBE_MAP:
            *params = m_texState[m_activeTexture][TEXTURE_CUBE_MAP].texture;
            break;

        case GL_TEXTURE_BINDING_2D:
            *params = m_texState[m_activeTexture][TEXTURE_2D].texture;
            break;

        case GL_ACTIVE_TEXTURE:
            *params = m_activeTexture+GL_TEXTURE0;
            break;

        case GL_MAX_TEXTURE_SIZE:
            *params = getMaxTexSize();
            break;
        default:
            return false;
    }

    return true;
}

TextureTarget GLEScontext::GLTextureTargetToLocal(GLenum target) {
    TextureTarget value=TEXTURE_2D;
    switch (target) {
    case GL_TEXTURE_CUBE_MAP:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        value = TEXTURE_CUBE_MAP;
        break;
    case GL_TEXTURE_2D:
        value = TEXTURE_2D;
        break;
    case GL_TEXTURE_2D_ARRAY:
        value = TEXTURE_2D_ARRAY;
        break;
    case GL_TEXTURE_3D:
        value = TEXTURE_3D;
        break;
    case GL_TEXTURE_2D_MULTISAMPLE:
        value = TEXTURE_2D_MULTISAMPLE;
        break;
    }
    return value;
}

unsigned int GLEScontext::getBindedTexture(GLenum target) {
    TextureTarget pos = GLTextureTargetToLocal(target);
    return m_texState[m_activeTexture][pos].texture;
}

unsigned int GLEScontext::getBindedTexture(GLenum unit, GLenum target) {
    TextureTarget pos = GLTextureTargetToLocal(target);
    return m_texState[unit-GL_TEXTURE0][pos].texture;
}

void GLEScontext::setBindedTexture(GLenum target, unsigned int tex) {
    TextureTarget pos = GLTextureTargetToLocal(target);
    m_texState[m_activeTexture][pos].texture = tex;
}

void GLEScontext::setTextureEnabled(GLenum target, GLenum enable) {
    TextureTarget pos = GLTextureTargetToLocal(target);
    m_texState[m_activeTexture][pos].enabled = enable;
}

#define INTERNAL_NAME(x) (x +0x100000000ll);

ObjectLocalName GLEScontext::getDefaultTextureName(GLenum target) {
    ObjectLocalName name = 0;
    switch (GLTextureTargetToLocal(target)) {
    case TEXTURE_2D:
        name = INTERNAL_NAME(0);
        break;
    case TEXTURE_CUBE_MAP:
        name = INTERNAL_NAME(1);
        break;
    case TEXTURE_2D_ARRAY:
        name = INTERNAL_NAME(2);
        break;
    case TEXTURE_3D:
        name = INTERNAL_NAME(3);
        break;
    case TEXTURE_2D_MULTISAMPLE:
        name = INTERNAL_NAME(4);
        break;
    default:
        name = 0;
        break;
    }
    return name;
}

ObjectLocalName GLEScontext::getTextureLocalName(GLenum target,
        unsigned int tex) {
    return (tex!=0? tex : getDefaultTextureName(target));
}

void GLEScontext::drawValidate(void)
{
    if(m_drawFramebuffer == 0)
        return;

    auto fbObj = getFBOData(m_drawFramebuffer);
    if (!fbObj)
        return;

    fbObj->validate(this);
}

void GLEScontext::initEmulatedEGLSurface(GLint width, GLint height,
                             GLint colorFormat, GLint depthstencilFormat, GLint multisamples,
                             GLuint rboColor, GLuint rboDepth) {
    dispatcher().glBindRenderbuffer(GL_RENDERBUFFER, rboColor);
    if (multisamples) {
        dispatcher().glRenderbufferStorageMultisample(GL_RENDERBUFFER, multisamples, colorFormat, width, height);
        GLint err = dispatcher().glGetError();
        if (err != GL_NO_ERROR) {
            fprintf(stderr, "%s: error setting up multisampled RBO! 0x%x\n", __func__, err);
        }
    } else {
        dispatcher().glRenderbufferStorage(GL_RENDERBUFFER, colorFormat, width, height);
    }

    dispatcher().glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    if (multisamples) {
        dispatcher().glRenderbufferStorageMultisample(GL_RENDERBUFFER, multisamples, depthstencilFormat, width, height);
        GLint err = dispatcher().glGetError();
        if (err != GL_NO_ERROR) {
            fprintf(stderr, "%s: error setting up multisampled RBO! 0x%x\n", __func__, err);
        }
    } else {
        dispatcher().glRenderbufferStorage(GL_RENDERBUFFER, depthstencilFormat, width, height);
    }
}

void GLEScontext::initDefaultFBO(
        GLint width, GLint height, GLint colorFormat, GLint depthstencilFormat, GLint multisamples,
        GLuint* eglSurfaceRBColorId, GLuint* eglSurfaceRBDepthId,
        GLuint readWidth, GLint readHeight, GLint readColorFormat, GLint readDepthStencilFormat, GLint readMultisamples,
        GLuint* eglReadSurfaceRBColorId, GLuint* eglReadSurfaceRBDepthId) {

    if (!m_defaultFBO) {
        dispatcher().glGenFramebuffers(1, &m_defaultFBO);
        m_defaultReadFBO = m_defaultFBO;
    }

    bool needReallocateRbo = false;
    bool separateReadRbo = false;
    bool needReallocateReadRbo = false;

    separateReadRbo =
        eglReadSurfaceRBColorId !=
        eglSurfaceRBColorId;

    if (separateReadRbo && (m_defaultReadFBO == m_defaultFBO)) {
        dispatcher().glGenFramebuffers(1, &m_defaultReadFBO);
    }

    if (!(*eglSurfaceRBColorId)) {
        dispatcher().glGenRenderbuffers(1, eglSurfaceRBColorId);
        dispatcher().glGenRenderbuffers(1, eglSurfaceRBDepthId);
        needReallocateRbo = true;
    }

    if (!(*eglReadSurfaceRBColorId) && separateReadRbo) {
        dispatcher().glGenRenderbuffers(1, eglReadSurfaceRBColorId);
        dispatcher().glGenRenderbuffers(1, eglReadSurfaceRBDepthId);
        needReallocateReadRbo = true;
    }

    m_defaultFBOColorFormat = colorFormat;
    m_defaultFBOWidth = width;
    m_defaultFBOHeight = height;
    m_defaultFBOSamples = multisamples;

    GLint prevRbo;
    dispatcher().glGetIntegerv(GL_RENDERBUFFER_BINDING, &prevRbo);

    // OS X in legacy opengl mode does not actually support GL_RGB565 as a renderbuffer.
    // Just replace it with GL_RGB8 for now.
    // TODO: Re-enable GL_RGB565 for OS X when we move to core profile.
#ifdef __APPLE__
    if (colorFormat == GL_RGB565)
        colorFormat = GL_RGB8;
    if (readColorFormat == GL_RGB565)
        readColorFormat = GL_RGB8;
#endif

    if (needReallocateRbo) {
        initEmulatedEGLSurface(width, height, colorFormat, depthstencilFormat, multisamples,
                                *eglSurfaceRBColorId, *eglSurfaceRBDepthId);
    }

    if (needReallocateReadRbo) {
        initEmulatedEGLSurface(readWidth, readHeight, readColorFormat, readDepthStencilFormat, readMultisamples,
                                *eglReadSurfaceRBColorId, *eglReadSurfaceRBDepthId);
    }

    dispatcher().glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);

    dispatcher().glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, *eglSurfaceRBColorId);
    dispatcher().glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, *eglSurfaceRBDepthId);
    dispatcher().glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *eglSurfaceRBDepthId);

    if (m_defaultFBODrawBuffer != GL_COLOR_ATTACHMENT0) {
        dispatcher().glDrawBuffers(1, &m_defaultFBODrawBuffer);
    }
    if (m_defaultFBOReadBuffer != GL_COLOR_ATTACHMENT0) {
        dispatcher().glReadBuffer(m_defaultFBOReadBuffer);
    }

    if (separateReadRbo) {
        dispatcher().glBindFramebuffer(GL_READ_FRAMEBUFFER, m_defaultReadFBO);
        dispatcher().glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, *eglReadSurfaceRBColorId);
        dispatcher().glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, *eglReadSurfaceRBDepthId);
        dispatcher().glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *eglReadSurfaceRBDepthId);
    }

    dispatcher().glBindRenderbuffer(GL_RENDERBUFFER, prevRbo);
    GLuint prevDrawFBOBinding = getFramebufferBinding(GL_FRAMEBUFFER);
    GLuint prevReadFBOBinding = getFramebufferBinding(GL_READ_FRAMEBUFFER);

    if (prevDrawFBOBinding)
        dispatcher().glBindFramebuffer(GL_FRAMEBUFFER, getFBOGlobalName(prevDrawFBOBinding));
    if (prevReadFBOBinding)
        dispatcher().glBindFramebuffer(GL_READ_FRAMEBUFFER, getFBOGlobalName(prevReadFBOBinding));

    // We might be initializing a surfaceless context underneath
    // where the viewport is initialized to 0x0 width and height.
    // Set to our wanted pbuffer dimensions if this is the first time
    // the viewport has been set.
    if (!m_isViewport) {
        setViewport(0, 0, width, height);
        dispatcher().glViewport(0, 0, width, height);
    }
    // same for the scissor
    if (!m_isScissor) {
        setScissor(0, 0, width, height);
        dispatcher().glScissor(0, 0, width, height);
    }
}


void GLEScontext::prepareCoreProfileEmulatedTexture(TextureData* texData, bool is3d, GLenum target,
                                                    GLenum format, GLenum type,
                                                    GLint* internalformat_out, GLenum* format_out) {
    if (format != GL_ALPHA &&
        format != GL_LUMINANCE &&
        format != GL_LUMINANCE_ALPHA) {
        return;
    }

    if (isCubeMapFaceTarget(target)) {
        target = is3d ? GL_TEXTURE_CUBE_MAP_ARRAY_EXT : GL_TEXTURE_CUBE_MAP;
    }

    // Set up the swizzle from the underlying supported
    // host format to the emulated format.
    // Make sure to re-apply any user-specified custom swizlz
    TextureSwizzle userSwz; // initialized to identity map

    if (texData) {
        userSwz.toRed = texData->getSwizzle(GL_TEXTURE_SWIZZLE_R);
        userSwz.toGreen = texData->getSwizzle(GL_TEXTURE_SWIZZLE_G);
        userSwz.toBlue = texData->getSwizzle(GL_TEXTURE_SWIZZLE_B);
        userSwz.toAlpha = texData->getSwizzle(GL_TEXTURE_SWIZZLE_A);
    }

    TextureSwizzle swz =
        concatSwizzles(getSwizzleForEmulatedFormat(format),
                       userSwz);

    dispatcher().glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, swz.toRed);
    dispatcher().glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, swz.toGreen);
    dispatcher().glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, swz.toBlue);
    dispatcher().glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, swz.toAlpha);

    // Change the format/internalformat communicated to GL.
    GLenum emulatedFormat =
        getCoreProfileEmulatedFormat(format);
    GLint emulatedInternalFormat =
        getCoreProfileEmulatedInternalFormat(format, type);

    if (format_out) *format_out = emulatedFormat;
    if (internalformat_out) *internalformat_out = emulatedInternalFormat;
}

bool GLEScontext::isFBO(ObjectLocalName p_localName) {
    return m_fboNameSpace->isObject(p_localName);
}

ObjectLocalName GLEScontext::genFBOName(ObjectLocalName p_localName,
        bool genLocal) {
    return m_fboNameSpace->genName(GenNameInfo(NamedObjectType::FRAMEBUFFER),
            p_localName, genLocal);
}

void GLEScontext::setFBOData(ObjectLocalName p_localName, ObjectDataPtr data) {
    m_fboNameSpace->setObjectData(p_localName, data);
}

void GLEScontext::deleteFBO(ObjectLocalName p_localName) {
    m_fboNameSpace->deleteName(p_localName);
}

FramebufferData* GLEScontext::getFBOData(ObjectLocalName p_localName) const {
    return (FramebufferData*)getFBODataPtr(p_localName).get();
}

ObjectDataPtr GLEScontext::getFBODataPtr(ObjectLocalName p_localName) const {
    return m_fboNameSpace->getObjectDataPtr(p_localName);
}

unsigned int GLEScontext::getFBOGlobalName(ObjectLocalName p_localName) const {
    return m_fboNameSpace->getGlobalName(p_localName);
}

ObjectLocalName GLEScontext::getFBOLocalName(unsigned int p_globalName) const {
    return m_fboNameSpace->getLocalName(p_globalName);
}

int GLEScontext::queryCurrFboBits(ObjectLocalName localFboName, GLenum pname) {
    GLint colorInternalFormat = 0;
    GLint depthInternalFormat = 0;
    GLint stencilInternalFormat = 0;
    bool combinedDepthStencil = false;

    if (!localFboName) {
        colorInternalFormat = m_defaultFBOColorFormat;
        // FBO 0 defaulting to d24s8
        depthInternalFormat =
            m_defaultFBODepthFormat ? m_defaultFBODepthFormat : GL_DEPTH24_STENCIL8;
        stencilInternalFormat =
            m_defaultFBOStencilFormat ? m_defaultFBOStencilFormat : GL_DEPTH24_STENCIL8;
    } else {
        FramebufferData* fbData = getFBOData(localFboName);

        std::vector<GLenum> colorAttachments(getCaps()->maxDrawBuffers);
        std::iota(colorAttachments.begin(), colorAttachments.end(), GL_COLOR_ATTACHMENT0);

        bool hasColorAttachment = false;
        for (auto attachment : colorAttachments) {
            GLint internalFormat =
                fbData->getAttachmentInternalFormat(this, attachment);

            // Only defined if all used color attachments are the same
            // internal format.
            if (internalFormat) {
                if (hasColorAttachment &&
                    colorInternalFormat != internalFormat) {
                    colorInternalFormat = 0;
                    break;
                }
                colorInternalFormat = internalFormat;
                hasColorAttachment = true;
            }
        }

        GLint depthStencilFormat =
            fbData->getAttachmentInternalFormat(this, GL_DEPTH_STENCIL_ATTACHMENT);

        if (depthStencilFormat) {
            combinedDepthStencil = true;
            depthInternalFormat = depthStencilFormat;
            stencilInternalFormat = depthStencilFormat;
        }

        if (!combinedDepthStencil) {
            depthInternalFormat =
                fbData->getAttachmentInternalFormat(this, GL_DEPTH_ATTACHMENT);
            stencilInternalFormat =
                fbData->getAttachmentInternalFormat(this, GL_STENCIL_ATTACHMENT);
        }
    }

    FramebufferChannelBits res =
        glFormatToChannelBits(colorInternalFormat,
                              depthInternalFormat,
                              stencilInternalFormat);

    switch (pname) {
    case GL_RED_BITS:
        return res.red;
    case GL_GREEN_BITS:
        return res.green;
    case GL_BLUE_BITS:
        return res.blue;
    case GL_ALPHA_BITS:
        return res.alpha;
    case GL_DEPTH_BITS:
        return res.depth;
    case GL_STENCIL_BITS:
        return res.stencil;
    }

    return 0;
}

static const char kTexImageEmulationVShaderSrc[] = R"(
precision highp float;
out vec2 v_texcoord;
void main() {
    const vec2 quad_pos[6] = vec2[6](
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0));

    gl_Position = vec4((quad_pos[gl_VertexID] * 2.0) - 1.0, 0.0, 1.0);
    v_texcoord = quad_pos[gl_VertexID];
})";

static const char kTexImageEmulationVShaderSrcFlipped[] = R"(
precision highp float;
layout (location = 0) in vec2 a_pos;
out vec2 v_texcoord;
void main() {
    gl_Position = vec4((a_pos.xy) * 2.0 - 1.0, 0.0, 1.0);
    v_texcoord = a_pos;
    v_texcoord.y = 1.0 - v_texcoord.y;
})";

static const char kTexImageEmulationFShaderSrc[] = R"(
precision highp float;
uniform sampler2D source_tex;
in vec2 v_texcoord;
out vec4 color;
void main() {
   color = texture(source_tex, v_texcoord);
})";

void GLEScontext::initTexImageEmulation() {
    if (m_textureEmulationProg) return;

    auto& gl = dispatcher();

    std::string vshaderSrc = isCoreProfile() ? "#version 330 core\n" : "#version 300 es\n";
    vshaderSrc += kTexImageEmulationVShaderSrc;
    std::string fshaderSrc = isCoreProfile() ? "#version 330 core\n" : "#version 300 es\n";
    fshaderSrc += kTexImageEmulationFShaderSrc;

    GLuint vshader =
        compileAndValidateCoreShader(GL_VERTEX_SHADER,
                                     vshaderSrc.c_str());
    GLuint fshader =
        compileAndValidateCoreShader(GL_FRAGMENT_SHADER,
                                     fshaderSrc.c_str());
    m_textureEmulationProg = linkAndValidateProgram(vshader, fshader);
    m_textureEmulationSamplerLoc =
        gl.glGetUniformLocation(m_textureEmulationProg, "source_tex");

    gl.glGenFramebuffers(1, &m_textureEmulationFBO);
    gl.glGenTextures(2, m_textureEmulationTextures);
    gl.glGenVertexArrays(1, &m_textureEmulationVAO);
}

void GLEScontext::copyTexImageWithEmulation(
        TextureData* texData,
        bool isSubImage,
        GLenum target,
        GLint level,
        GLenum internalformat,
        GLint xoffset, GLint yoffset,
        GLint x, GLint y,
        GLsizei width, GLsizei height,
        GLint border) {

    // Create objects used for emulation if they don't exist already.
    initTexImageEmulation();
    auto& gl = dispatcher();

    // Save all affected state.
    ScopedGLState state;
    state.pushForCoreProfileTextureEmulation();

    // render to an intermediate texture with the same format:
    // 1. Get the format
    FramebufferData* fbData =
        getFBOData(getFramebufferBinding(GL_READ_FRAMEBUFFER));
    GLint readFbInternalFormat =
        fbData ? fbData->getAttachmentInternalFormat(this, GL_COLOR_ATTACHMENT0) :
                 m_defaultFBOColorFormat;

    // 2. Create the texture for textures[0] with this format, and initialize
    // it to the current FBO read buffer.
    gl.glBindTexture(GL_TEXTURE_2D, m_textureEmulationTextures[0]);
    gl.glCopyTexImage2D(GL_TEXTURE_2D, 0, readFbInternalFormat,
                        x, y, width, height, 0);

    // 3. Set swizzle of textures[0] so they are read in the right way
    // when drawing to textures[1].
    TextureSwizzle swz = getInverseSwizzleForEmulatedFormat(texData->format);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, swz.toRed);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, swz.toGreen);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, swz.toBlue);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, swz.toAlpha);
    // Also, nearest filtering
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // 4. Initialize textures[1] with same width/height, and use it to back
    // the FBO that holds the swizzled results.
    gl.glBindTexture(GL_TEXTURE_2D, m_textureEmulationTextures[1]);
    gl.glTexImage2D(GL_TEXTURE_2D, 0, readFbInternalFormat, width, height, 0,
                    baseFormatOfInternalFormat(readFbInternalFormat),
                    accurateTypeOfInternalFormat(readFbInternalFormat),
                    nullptr);
    // Also, nearest filtering
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    gl.glBindFramebuffer(GL_FRAMEBUFFER, m_textureEmulationFBO);
    gl.glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        m_textureEmulationTextures[1], 0);

    // 5. Draw textures[0] to our FBO, making sure all state is compatible.
    gl.glDisable(GL_BLEND);
    gl.glDisable(GL_SCISSOR_TEST);
    gl.glDisable(GL_DEPTH_TEST);
    gl.glDisable(GL_STENCIL_TEST);
    gl.glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    gl.glDisable(GL_SAMPLE_COVERAGE);
    gl.glDisable(GL_CULL_FACE);
    gl.glDisable(GL_POLYGON_OFFSET_FILL);
    gl.glDisable(GL_RASTERIZER_DISCARD);

    gl.glViewport(0, 0, width, height);

    if (isGles2Gles()) {
        gl.glDepthRangef(0.0f, 1.0f);
    } else {
        gl.glDepthRange(0.0f, 1.0f);
    }

    gl.glColorMask(1, 1, 1, 1);

    gl.glBindTexture(GL_TEXTURE_2D, m_textureEmulationTextures[0]);
    GLint texUnit; gl.glGetIntegerv(GL_ACTIVE_TEXTURE, &texUnit);

    gl.glUseProgram(m_textureEmulationProg);
    gl.glUniform1i(m_textureEmulationSamplerLoc, texUnit - GL_TEXTURE0);

    gl.glBindVertexArray(m_textureEmulationVAO);

    gl.glDrawArrays(GL_TRIANGLES, 0, 6);

    // now the emulated version has been rendered and written to the read FBO
    // with the correct swizzle.
    if (isCubeMapFaceTarget(target)) {
        gl.glBindTexture(GL_TEXTURE_CUBE_MAP, texData->getGlobalName());
    } else {
        gl.glBindTexture(target, texData->getGlobalName());
    }

    if (isSubImage) {
        gl.glCopyTexSubImage2D(target, level, xoffset, yoffset, 0, 0, width, height);
    } else {
        gl.glCopyTexImage2D(target, level, internalformat, 0, 0, width, height, border);
    }
}

// static
GLuint GLEScontext::compileAndValidateCoreShader(GLenum shaderType, const char* src) {
    GLDispatch& gl = dispatcher();

    GLuint shader = gl.glCreateShader(shaderType);
    gl.glShaderSource(shader, 1, (const GLchar* const*)&src, nullptr);
    gl.glCompileShader(shader);

    GLint compileStatus;
    gl.glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

    if (compileStatus != GL_TRUE) {
        GLsizei infoLogLength = 0;
        gl.glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<char> infoLog(infoLogLength + 1, 0);
        gl.glGetShaderInfoLog(shader, infoLogLength, nullptr, &infoLog[0]);
        fprintf(stderr, "%s: fail to compile. infolog %s\n", __func__, &infoLog[0]);
    }

    return shader;
}

// static
GLuint GLEScontext::linkAndValidateProgram(GLuint vshader, GLuint fshader) {
    GLDispatch& gl = dispatcher();

    GLuint program = gl.glCreateProgram();
    gl.glAttachShader(program, vshader);
    gl.glAttachShader(program, fshader);
    gl.glLinkProgram(program);

    GLint linkStatus;
    gl.glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

    if (linkStatus != GL_TRUE) {
        GLsizei infoLogLength = 0;
        gl.glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<char> infoLog(infoLogLength + 1, 0);
        gl.glGetProgramInfoLog(program, infoLogLength, nullptr, &infoLog[0]);

        fprintf(stderr, "%s: fail to link program. infolog: %s\n", __func__,
                &infoLog[0]);
    }

    gl.glDeleteShader(vshader);
    gl.glDeleteShader(fshader);

    return program;
}

int GLEScontext::getReadBufferSamples() {
    GLuint readFboBinding = getFramebufferBinding(GL_READ_FRAMEBUFFER);
    bool defaultFboReadBufferBound = readFboBinding == 0;
    if (defaultFboReadBufferBound) {
        return m_defaultFBOSamples;
    } else {
        FramebufferData* fbData = (FramebufferData*)(getFBODataPtr(readFboBinding).get());
        return fbData ? fbData->getAttachmentSamples(this, fbData->getReadBuffer()) : 0;
    }
}

int GLEScontext::getReadBufferInternalFormat() {
    GLuint readFboBinding = getFramebufferBinding(GL_READ_FRAMEBUFFER);
    bool defaultFboReadBufferBound = readFboBinding == 0;
    if (defaultFboReadBufferBound) {
        return m_defaultFBOColorFormat;
    } else {
        FramebufferData* fbData = (FramebufferData*)(getFBODataPtr(readFboBinding).get());
        return fbData ? fbData->getAttachmentInternalFormat(this, fbData->getReadBuffer()) : 0;
    }
}

void GLEScontext::getReadBufferDimensions(GLint* width, GLint* height) {
    GLuint readFboBinding = getFramebufferBinding(GL_READ_FRAMEBUFFER);
    bool defaultFboReadBufferBound = readFboBinding == 0;
    if (defaultFboReadBufferBound) {
        *width = m_defaultFBOWidth;
        *height = m_defaultFBOHeight;
    } else {
        FramebufferData* fbData = (FramebufferData*)(getFBODataPtr(readFboBinding).get());
        if (fbData) {
            fbData->getAttachmentDimensions(
                this, fbData->getReadBuffer(), width, height);
        }
    }
}

void GLEScontext::setupImageBlitState() {
    auto& gl = dispatcher();
    m_blitState.prevSamples = m_blitState.samples;
    m_blitState.samples = getReadBufferSamples();

    if (m_blitState.program) return;

    std::string vshaderSrc =
        isCoreProfile() ? "#version 330 core\n" : "#version 300 es\n";
    vshaderSrc += kTexImageEmulationVShaderSrcFlipped;

    std::string fshaderSrc =
        isCoreProfile() ? "#version 330 core\n" : "#version 300 es\n";
    fshaderSrc += kTexImageEmulationFShaderSrc;

    GLuint vshader =
        compileAndValidateCoreShader(GL_VERTEX_SHADER, vshaderSrc.c_str());
    GLuint fshader =
        compileAndValidateCoreShader(GL_FRAGMENT_SHADER, fshaderSrc.c_str());

    m_blitState.program = linkAndValidateProgram(vshader, fshader);
    m_blitState.samplerLoc =
        gl.glGetUniformLocation(m_blitState.program, "source_tex");

    gl.glGenFramebuffers(1, &m_blitState.fbo);
    gl.glGenFramebuffers(1, &m_blitState.resolveFbo);
    gl.glGenTextures(1, &m_blitState.tex);
    gl.glGenVertexArrays(1, &m_blitState.vao);

    gl.glGenBuffers(1, &m_blitState.vbo);
    float blitVbo[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
    };

    GLint buf;
    gl.glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buf);

    gl.glBindBuffer(GL_ARRAY_BUFFER, m_blitState.vbo);
    gl.glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), blitVbo, GL_STATIC_DRAW);

    gl.glBindVertexArray(m_blitState.vao);
    gl.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    gl.glEnableVertexAttribArray(0);

    gl.glBindBuffer(GL_ARRAY_BUFFER, buf);
}

bool GLEScontext::setupImageBlitForTexture(uint32_t width,
                                           uint32_t height,
                                           GLint internalFormat) {
    GLint sizedInternalFormat = GL_RGBA8;
    if (internalFormat != GL_RGBA8 &&
        internalFormat != GL_RGB8 &&
        internalFormat != GL_RGB565) {
        switch (internalFormat) {
        case GL_RGB:
            sizedInternalFormat = GL_RGB8;
            break;
        case GL_RGBA:
            sizedInternalFormat = GL_RGBA8;
            break;
        default:
            break;
        }
    }

    auto& gl = dispatcher();
    gl.glBindTexture(GL_TEXTURE_2D, m_blitState.tex);

    GLint read_iformat = getReadBufferInternalFormat();
    GLint read_format = baseFormatOfInternalFormat(read_iformat);

    if (isIntegerInternalFormat(read_iformat) ||
        read_iformat == GL_RGB10_A2) {
        // Is not a blittable format. Just create the texture for now to
        // make image blit state consistent.
        gl.glTexImage2D(GL_TEXTURE_2D, 0, sizedInternalFormat, width, height, 0,
                baseFormatOfInternalFormat(internalFormat), GL_UNSIGNED_BYTE, 0);
        return false;
    }

    if (width != m_blitState.width || height != m_blitState.height ||
        internalFormat != m_blitState.internalFormat ||
        m_blitState.samples != m_blitState.prevSamples) {

        m_blitState.width = width;
        m_blitState.height = height;
        m_blitState.internalFormat = internalFormat;

        gl.glTexImage2D(GL_TEXTURE_2D, 0,
                        read_iformat, width, height, 0, read_format, GL_UNSIGNED_BYTE, 0);
        if (m_blitState.samples > 0) {
            gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_blitState.resolveFbo);
            gl.glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_2D, m_blitState.tex, 0);
        }

        gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // In eglSwapBuffers, the surface must be bound as the draw surface of
    // the current context, which corresponds to m_defaultFBO here.
    //
    // EGL 1.4 spec:
    //
    // 3.9.3 Posting Semantics surface must be bound to the draw surface of the
    // calling thread’s current context, for the current rendering API. This
    // restriction may be lifted in future EGL revisions.
    //
    const GLuint readFboBinding = getFramebufferBinding(GL_READ_FRAMEBUFFER);
    if (readFboBinding != 0) {
        gl.glBindFramebuffer(GL_READ_FRAMEBUFFER, m_defaultFBO);
    }

    if (m_blitState.samples > 0) {
        GLint rWidth = width;
        GLint rHeight = height;
        getReadBufferDimensions(&rWidth, &rHeight);
        gl.glBindTexture(GL_TEXTURE_2D, 0);
        gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_blitState.resolveFbo);
        gl.glBlitFramebuffer(0, 0, rWidth, rHeight, 0, 0, rWidth, rHeight,
                GL_COLOR_BUFFER_BIT, GL_NEAREST);
        gl.glBindTexture(GL_TEXTURE_2D, m_blitState.tex);
    } else {
        gl.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);
    }

    return true;
}

void GLEScontext::blitFromReadBufferToTextureFlipped(GLuint globalTexObj,
                                                     GLuint width,
                                                     GLuint height,
                                                     GLint internalFormat,
                                                     GLenum format,
                                                     GLenum type) {
    // TODO: these might also matter
    (void)format;
    (void)type;

    auto& gl = dispatcher();
    GLint prevViewport[4];
    getViewport(prevViewport);

    setupImageBlitState();
    bool shouldBlit = setupImageBlitForTexture(width, height, internalFormat);

    if (!shouldBlit) return;

    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_blitState.fbo);
    gl.glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D, globalTexObj, 0);

    gl.glDisable(GL_BLEND);
    gl.glDisable(GL_SCISSOR_TEST);
    gl.glDisable(GL_DEPTH_TEST);
    gl.glDisable(GL_STENCIL_TEST);
    gl.glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    gl.glDisable(GL_SAMPLE_COVERAGE);
    gl.glDisable(GL_CULL_FACE);
    gl.glDisable(GL_POLYGON_OFFSET_FILL);
    gl.glDisable(GL_RASTERIZER_DISCARD);

    gl.glViewport(0, 0, width, height);
    if (isGles2Gles()) {
        gl.glDepthRangef(0.0f, 1.0f);
    } else {
        gl.glDepthRange(0.0f, 1.0f);
    }
    gl.glColorMask(1, 1, 1, 1);

    gl.glUseProgram(m_blitState.program);
    gl.glUniform1i(m_blitState.samplerLoc, m_activeTexture);

    gl.glBindVertexArray(m_blitState.vao);
    gl.glDrawArrays(GL_TRIANGLES, 0, 6);

    // state restore
    const GLuint globalProgramName = shareGroup()->getGlobalName(
        NamedObjectType::SHADER_OR_PROGRAM, m_useProgram);
    gl.glUseProgram(globalProgramName);

    gl.glBindVertexArray(getVAOGlobalName(m_currVaoState.vaoId()));

    gl.glBindTexture(
        GL_TEXTURE_2D,
        shareGroup()->getGlobalName(
            NamedObjectType::TEXTURE,
            getTextureLocalName(GL_TEXTURE_2D,
                                getBindedTexture(GL_TEXTURE_2D))));

    GLuint drawFboBinding = getFramebufferBinding(GL_DRAW_FRAMEBUFFER);
    GLuint readFboBinding = getFramebufferBinding(GL_READ_FRAMEBUFFER);

    gl.glBindFramebuffer(
        GL_DRAW_FRAMEBUFFER,
        drawFboBinding ? getFBOGlobalName(drawFboBinding) : m_defaultFBO);
    gl.glBindFramebuffer(
        GL_READ_FRAMEBUFFER,
        readFboBinding ? getFBOGlobalName(readFboBinding) : m_defaultReadFBO);

    if (isEnabled(GL_BLEND)) gl.glEnable(GL_BLEND);
    if (isEnabled(GL_SCISSOR_TEST)) gl.glEnable(GL_SCISSOR_TEST);
    if (isEnabled(GL_DEPTH_TEST)) gl.glEnable(GL_DEPTH_TEST);
    if (isEnabled(GL_STENCIL_TEST)) gl.glEnable(GL_STENCIL_TEST);
    if (isEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE)) gl.glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    if (isEnabled(GL_SAMPLE_COVERAGE)) gl.glEnable(GL_SAMPLE_COVERAGE);
    if (isEnabled(GL_CULL_FACE)) gl.glEnable(GL_CULL_FACE);
    if (isEnabled(GL_POLYGON_OFFSET_FILL)) gl.glEnable(GL_POLYGON_OFFSET_FILL);
    if (isEnabled(GL_RASTERIZER_DISCARD)) gl.glEnable(GL_RASTERIZER_DISCARD);

    gl.glViewport(prevViewport[0], prevViewport[1],
                  prevViewport[2], prevViewport[3]);

    if (isGles2Gles()) {
        gl.glDepthRangef(m_zNear, m_zFar);
    } else {
        gl.glDepthRange(m_zNear, m_zFar);
    }

    gl.glColorMask(m_colorMaskR, m_colorMaskG, m_colorMaskB, m_colorMaskA);

    gl.glFlush();
}

// Primitive restart emulation
#define GL_PRIMITIVE_RESTART              0x8F9D
#define GL_PRIMITIVE_RESTART_INDEX        0x8F9E

void GLEScontext::setPrimitiveRestartEnabled(bool enabled) {
    auto& gl = dispatcher();

    if (enabled) {
        gl.glEnable(GL_PRIMITIVE_RESTART);
    } else {
        gl.glDisable(GL_PRIMITIVE_RESTART);
    }

    m_primitiveRestartEnabled = enabled;
}

void GLEScontext::updatePrimitiveRestartIndex(GLenum type) {
    auto& gl = dispatcher();
    switch (type) {
    case GL_UNSIGNED_BYTE:
        gl.glPrimitiveRestartIndex(0xff);
        break;
    case GL_UNSIGNED_SHORT:
        gl.glPrimitiveRestartIndex(0xffff);
        break;
    case GL_UNSIGNED_INT:
        gl.glPrimitiveRestartIndex(0xffffffff);
        break;
    }
}

bool GLEScontext::isVAO(ObjectLocalName p_localName) {
    VAOStateMap::iterator it = m_vaoStateMap.find(p_localName);
    if (it == m_vaoStateMap.end()) return false;
    VAOStateRef vao(it);
    return vao.isEverBound();
}

ObjectLocalName GLEScontext::genVAOName(ObjectLocalName p_localName,
        bool genLocal) {
    return m_vaoNameSpace->genName(GenNameInfo(NamedObjectType::VERTEX_ARRAY_OBJECT),
            p_localName, genLocal);
}

void GLEScontext::deleteVAO(ObjectLocalName p_localName) {
    m_vaoNameSpace->deleteName(p_localName);
}

unsigned int GLEScontext::getVAOGlobalName(ObjectLocalName p_localName) {
    return m_vaoNameSpace->getGlobalName(p_localName);
}

ObjectLocalName GLEScontext::getVAOLocalName(unsigned int p_globalName) {
    return m_vaoNameSpace->getLocalName(p_globalName);
}

void GLEScontext::setDefaultFBODrawBuffer(GLenum buffer) {
    m_defaultFBODrawBuffer = buffer;
}

void GLEScontext::setDefaultFBOReadBuffer(GLenum buffer) {
    m_defaultFBOReadBuffer = buffer;
}
