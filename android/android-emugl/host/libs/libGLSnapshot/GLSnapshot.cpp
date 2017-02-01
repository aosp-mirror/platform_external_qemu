#include "GLSnapshot.h"

#include "android/base/synchronization/Lock.h"

#include "emugl/common/OpenGLDispatchLoader.h"

#include <GLES2/gl2.h>
#include <GLES3/gl31.h>

#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG 1

#if DEBUG

#define D(fmt,...) do { \
    FILE* fh = fopen("dlog.txt", "a"); \
    fprintf(fh, "%s:%s:%d:%p " fmt "\n", __FILE__, __FUNCTION__, __LINE__, this, ##__VA_ARGS__); \
    fclose(fh); \
} while(0) \

#else
#define D(...)
#endif

namespace GLSnapshot {

static android::base::Lock sLock;
static uint32_t sNextUid = 1;

template <class K, class V>
static void saveMap_32_32(const std::unordered_map<K, V>& m, android::base::Stream* stream) {
    stream->putBe32(m.size());
    for (const auto& it : m) {
        stream->putBe32(it.first);
        stream->putBe32(it.second);
    }
}

template <class K>
static void saveMap_32_string(const std::unordered_map<K, std::string>& m, android::base::Stream* stream) {
    stream->putBe32(m.size());
    for (const auto& it : m) {
        stream->putBe32(it.first);
        stream->putString(it.second.c_str());
    }
}

template <class K, class V>
static void saveMap_32_Custom(const std::unordered_map<K, V> m, android::base::Stream* stream) {
    stream->putBe32(m.size());
    for (const auto& it : m) {
        stream->putBe32(it.first);
        it.second.toStream(stream);
    }
}

template <class K, class V>
static void loadMap_32_32(std::unordered_map<K, V>& m, android::base::Stream* stream) {
    uint32_t count = stream->getBe32();
    for (uint32_t i = 0; i < count; i++) {
        K fst; V snd;
        fst = stream->getBe32();
        snd = stream->getBe32();
        m[fst] = snd;
    }
}

template <class K>
static void loadMap_32_string(std::unordered_map<K, std::string>& m, android::base::Stream* stream) {
    stream->putBe32(m.size());
    for (auto& it : m) {
        stream->putBe32(it.first);
        it.second = stream->getString();
    }
}

template <class K, class V>
static void loadMap_32_Custom(std::unordered_map<K, V> m, android::base::Stream* stream) {
    uint32_t count = stream->getBe32();
    for (uint32_t i = 0; i < count; i++) {
        K fst; V snd;
        fst = stream->getBe32();
        snd.fromStream(stream);
        m[fst] = snd;
    }
}

static std::string getIndent(int indentLvl) {
    std::string indent;
    for (int i = 0; i < indentLvl; i++) {
        indent += "    ";
    }
    return indent;
}

template <class K, class V>
static void summarizeMap_32_32(const char* name, const std::unordered_map<K, V>& m, std::ostringstream& ss, int indentLvl) {
    std::string indent = getIndent(indentLvl);
    ss << indent << "32/32 map: [" << name << "]\n";
    for (const auto& it : m) {
        ss << indent << it.first << " -> " << it.second << "\n";
    }
}

template <class K>
static void summarizeMap_32_string(const char* name, const std::unordered_map<K, std::string>& m, std::ostringstream& ss, int indentLvl) {
    std::string indent = getIndent(indentLvl);
    ss << indent << "32/string map: [" << name << "]\n";
    for (const auto& it : m) {
        ss << indent << it.first << " -> string[\n";
        ss << indent << it.second << "]\n";
    }
}

template <class K, class V>
static void summarizeMap_32_Custom(const char* name, const std::unordered_map<K, V>& m, std::ostringstream& ss, int indentLvl) {
    std::string indent = getIndent(indentLvl);
    ss << indentLvl << "32/custom map: [" << name << "]\n";
    for (const auto& it : m) {
        ss << indentLvl << it.first << " -> {\n";
        it.second.summarize(ss, indentLvl + 1);
        ss << indentLvl << "}\n";
    }
}

template <class T>
static void summarizeVal(const char* name, T val, std::ostringstream& ss, int indentLvl) {
    ss << getIndent(indentLvl) << name << ": " << val << "\n";
}

template <class T>
static void summarizeMultiVal(const char* name, const std::vector<T>& vals, std::ostringstream& ss, int indentLvl) {
    if (vals.size()) {
        ss << getIndent(indentLvl) << name << ": ";
        for (const auto& elt : vals) {
            ss << elt << " ";
        }
        ss << "\n";
    }
}

template <class T>
static void summarizeGLBuffer(const char* name, void* ptr, T sizeExpr, std::ostringstream& ss, int indentLvl) {
    ss << getIndent(indentLvl) << name << ": @" << ptr << " sz " << sizeExpr << "\n";
}

static void saveGLBuffer(void* src, uint32_t sz, android::base::Stream* stream) {
    stream->putString((char*)src, sz);
}

static void loadGLBuffer(void** dst, uint32_t sz, android::base::Stream* stream) {
    std::string datastr = stream->getString();
    if (sz) {
        *dst = malloc(sz);
        memcpy((char*)(*dst), datastr.c_str(), sz);
    }
}

void GLValue::toStream(android::base::Stream* stream) const {
    stream->putBe32(enums.size());
    for (const auto elt: enums) 
        stream->putBe32(elt);
    stream->putBe32(bytes.size());
    for (const auto elt: bytes)
        stream->putByte(elt);
    stream->putBe32(shorts.size());
    for (const auto elt: shorts)
        stream->putBe16(elt);
    stream->putBe32(ints.size());
    for (const auto elt: ints)
        stream->putBe32(elt);
    stream->putBe32(floats.size());
    for (const auto elt: floats)
        stream->putFloat(elt);
    stream->putBe32(int64s.size());
    for (const auto elt: int64s)
        stream->putBe64(elt);
}
void GLValue::fromStream(android::base::Stream* stream) {
    enums.resize(stream->getBe32());
    for (auto& elt: enums) 
        elt = stream->getBe32();
    bytes.resize(stream->getBe32());
    for (auto& elt: bytes)
        elt = stream->getByte();
    shorts.resize(stream->getBe32());
    for (auto& elt: shorts)
        elt = stream->getBe16();
    ints.resize(stream->getBe32());
    for (auto& elt: ints)
        elt = stream->getBe32();
    floats.resize(stream->getBe32());
    for (auto& elt: floats)
        elt = stream->getFloat();
    int64s.resize(stream->getBe32());
    for (auto& elt: int64s)
        elt = stream->getBe64();
}

void GLValue::summarize(std::ostringstream& ss, int indentLvl) const {
    summarizeMultiVal("enum", enums, ss, indentLvl);
    summarizeMultiVal("bytes", bytes, ss, indentLvl);
    summarizeMultiVal("shorts", shorts, ss, indentLvl);
    summarizeMultiVal("ints", ints, ss, indentLvl);
    summarizeMultiVal("floats", floats, ss, indentLvl);
    summarizeMultiVal("int64s", int64s, ss, indentLvl);
}

void GLShaderState::toStream(android::base::Stream* stream) const {
    stream->putBe32(type);
    stream->putString(source.c_str());
    stream->putBe32(compileStatus);
}
void GLShaderState::fromStream(android::base::Stream* stream) {
    type = stream->getBe32();
    source = stream->getString();
    compileStatus = stream->getBe32();
}
void GLShaderState::summarize(std::ostringstream& ss, int indentLvl) const {
    summarizeVal("type", type, ss, indentLvl);
    summarizeVal("src", source, ss, indentLvl);
    summarizeVal("compileStatus", compileStatus, ss, indentLvl);
}

void GLUniformDesc::toStream(android::base::Stream* stream) const {
    stream->putBe32(count);
    stream->putBe32(transpose);
    stream->putBe32(valtype);
    val.toStream(stream);
}
void GLUniformDesc::fromStream(android::base::Stream* stream) {
    count = stream->getBe32();
    transpose = stream->getBe32();
    valtype = (GLValueType)stream->getBe32();
    val.fromStream(stream);
}
void GLUniformDesc::summarize(std::ostringstream& ss, int indentLvl) const {
    summarizeVal("count", count, ss, indentLvl);
    summarizeVal("transpose", transpose, ss, indentLvl);
    summarizeVal("valtype", valtype, ss, indentLvl);
    val.summarize(ss, indentLvl);
}

void GLProgramState::toStream(android::base::Stream* stream) const {
    saveMap_32_32(linkage, stream);
    saveMap_32_string(boundAttribLocs, stream);
    saveMap_32_Custom(uniformDescs, stream);
    stream->putBe32(linkStatus);
}
void GLProgramState::fromStream(android::base::Stream* stream) {
    loadMap_32_32(linkage, stream);
    loadMap_32_string(boundAttribLocs, stream);
    loadMap_32_Custom(uniformDescs, stream);
    linkStatus = stream->getBe32();
}
void GLProgramState::summarize(std::ostringstream& ss, int indentLvl) const {
    summarizeMap_32_32("linkage", linkage, ss, indentLvl);
    summarizeMap_32_32("boundAttribLocs", boundAttribLocs, ss, indentLvl);
    summarizeMap_32_Custom("uniformDescs", uniformDescs, ss, indentLvl);
    summarizeVal("linkStatus", linkStatus, ss, indentLvl);
}

void GLBufferState::toStream(android::base::Stream* stream) const {
    stream->putBe32(size);
    saveGLBuffer(data, size, stream);
    stream->putBe32(usage);
    stream->putBe32(mapped);
    stream->putBe32(mappedAccess);
    stream->putBe64(mappedOffset);
    stream->putBe64(mappedLength);
}
void GLBufferState::fromStream(android::base::Stream* stream) {
    size = stream->getBe32();
    loadGLBuffer(&data, size, stream);
    usage = stream->getBe32();
    mapped = stream->getBe32();
    mappedAccess = stream->getBe32();
    mappedOffset = stream->getBe64();
    mappedLength = stream->getBe64();
}
void GLBufferState::summarize(std::ostringstream& ss, int indentLvl) const {
    summarizeVal("size", size, ss, indentLvl);
    summarizeGLBuffer("data", data, size, ss, indentLvl);
    summarizeVal("usage", usage, ss, indentLvl);
    summarizeVal("mapped", mapped, ss, indentLvl);
    if (mapped) {
        summarizeVal("mappedAccess", mappedAccess, ss, indentLvl);
        summarizeVal("mappedOffset", mappedOffset, ss, indentLvl);
        summarizeVal("mappedLength", mappedLength, ss, indentLvl);
    }
}

void GLTextureAttachmentInfo::toStream(android::base::Stream* stream) const {
    stream->putBe32(textarget);
    stream->putBe32(texture);
    stream->putBe32(level);
    stream->putBe32(samples);
}
void GLTextureAttachmentInfo::fromStream(android::base::Stream* stream) {
    textarget = stream->getBe32();
    texture = stream->getBe32();
    level = stream->getBe32();
    samples = stream->getBe32();
}
void GLTextureAttachmentInfo::summarize(std::ostringstream& ss, int indentLvl) const {
    summarizeVal("textarget", textarget, ss, indentLvl);
    summarizeVal("texture", texture, ss, indentLvl);
    summarizeVal("level", level, ss, indentLvl);
    if (samples) summarizeVal("samples", samples, ss, indentLvl);
}

void GLRenderbufferAttachmentInfo::toStream(android::base::Stream* stream) const {
    stream->putBe32(internalformat);
    stream->putBe32(width);
    stream->putBe32(height);
    stream->putBe32(samples);
}
void GLRenderbufferAttachmentInfo::fromStream(android::base::Stream* stream) {
    internalformat = stream->getBe32();
    width = stream->getBe32();
    height = stream->getBe32();
    samples = stream->getBe32();
}
void GLRenderbufferAttachmentInfo::summarize(std::ostringstream& ss, int indentLvl) const {
    summarizeVal("internalformat", internalformat, ss, indentLvl);
    summarizeVal("width", width, ss, indentLvl);
    summarizeVal("height", height, ss, indentLvl);
    if (samples) summarizeVal("samples", samples, ss, indentLvl);
}

void GLFBOAttachmentInfo::toStream(android::base::Stream* stream) const {
    stream->putBe32(type);
    if (type == GL_TEXTURE_2D)
        texture.toStream(stream);
    else
        renderbuffer.toStream(stream);
}
void GLFBOAttachmentInfo::fromStream(android::base::Stream* stream) {
    type = stream->getBe32();
    if (type == GL_TEXTURE_2D)
        texture.fromStream(stream);
    else
        renderbuffer.fromStream(stream);
}
void GLFBOAttachmentInfo::summarize(std::ostringstream& ss, int indentLvl) const {
    summarizeVal("type", type, ss, indentLvl);
    if (type == GL_TEXTURE_2D)
        texture.summarize(ss, indentLvl);
    else
        renderbuffer.summarize(ss, indentLvl);
}

void GLFBOState::toStream(android::base::Stream* stream) const {
    saveMap_32_Custom(attachments, stream);
}
void GLFBOState::fromStream(android::base::Stream* stream) {
    loadMap_32_Custom(attachments, stream);
}
void GLFBOState::summarize(std::ostringstream& ss, int indentLvl) const {
    summarizeMap_32_Custom("attachments", attachments, ss, indentLvl);
}

void GLTextureState::toStream(android::base::Stream* stream) const {
    for (uint32_t i = 0; i < SNAPSHOT_MAX_TEXTURE_LEVELS; i++) {
        stream->putBe32(nbytes[i]);
        stream->putBe32(width[i]);
        stream->putBe32(height[i]);
        saveGLBuffer(data[i], nbytes[i], stream);
    }

    stream->putBe32(internalformat);
    stream->putBe32(format);
    stream->putBe32(type);

    stream->putBe32(magFilter);
    stream->putBe32(minFilter);
    stream->putBe32(wrapS);
    stream->putBe32(wrapT);
    stream->putBe32(boundTarget);
    stream->putBe32(unpackAlignment);
}
void GLTextureState::fromStream(android::base::Stream* stream) {
    for (uint32_t i = 0; i < SNAPSHOT_MAX_TEXTURE_LEVELS; i++) {
        nbytes[i] = stream->getBe32();
        width[i] = stream->getBe32();
        height[i] = stream->getBe32();
        loadGLBuffer(data + i, nbytes[i], stream);
    }

    internalformat = stream->getBe32();
    format = stream->getBe32();
    type = stream->getBe32();

    magFilter = stream->getBe32();
    minFilter = stream->getBe32();
    wrapS = stream->getBe32();
    wrapT = stream->getBe32();
    boundTarget = stream->getBe32();
    unpackAlignment = stream->getBe32();
}
void GLTextureState::summarize(std::ostringstream& ss, int indentLvl) const {
    for (uint32_t i = 0; i < SNAPSHOT_MAX_TEXTURE_LEVELS; i++) {
        if (nbytes[i]) {
            summarizeVal("storage for lvl", i, ss, indentLvl + 1);
            summarizeVal("nbytes", nbytes, ss, indentLvl + 1);
            summarizeVal("w", width[i], ss, indentLvl + 1);
            summarizeVal("h", height[i], ss, indentLvl + 1);
        }
    }

    summarizeVal("internalformat", internalformat, ss, indentLvl);
    summarizeVal("format", format, ss, indentLvl);
    summarizeVal("type", type, ss, indentLvl);
    summarizeVal("magFilter", magFilter, ss, indentLvl);
    summarizeVal("minFilter", minFilter, ss, indentLvl);
    summarizeVal("wrapS", wrapS, ss, indentLvl);
    summarizeVal("wrapT", wrapT, ss, indentLvl);
    summarizeVal("boundTarget", boundTarget, ss, indentLvl);
    summarizeVal("unpackAlignment", unpackAlignment, ss, indentLvl);
}

void GLVertexAttribState::toStream(android::base::Stream* stream) const {
    stream->putBe32(enabled);
    stream->putBe32(size);
    stream->putBe32(type);
    stream->putBe32(normalized);
    stream->putBe32(stride);
    stream->putBe32(arrayBuffer);
    stream->putBe32(offset);
    stream->putBe32(datalen);
    saveGLBuffer(data, datalen, stream);
}
void GLVertexAttribState::fromStream(android::base::Stream* stream) {
    enabled = stream->getBe32();
    size = stream->getBe32();
    type = stream->getBe32();
    normalized = stream->getBe32();
    stride = stream->getBe32();
    arrayBuffer = stream->getBe32();
    offset = stream->getBe32();
    datalen = stream->getBe32();
    loadGLBuffer(&data, datalen, stream);
}
void GLVertexAttribState::summarize(std::ostringstream& ss, int indentLvl) const {
    summarizeVal("enabled", enabled, ss, indentLvl);
    summarizeVal("size", size, ss, indentLvl);
    summarizeVal("type", type, ss, indentLvl);
    summarizeVal("normalized", normalized, ss, indentLvl);
    summarizeVal("stride", stride, ss, indentLvl);
    summarizeVal("arrayBuffer", arrayBuffer, ss, indentLvl);

    if (arrayBuffer) {
        summarizeVal("offset", offset, ss, indentLvl);
    } else {
        summarizeGLBuffer("data", data, datalen, ss, indentLvl);
    }
}

void GLVAOState::toStream(android::base::Stream* stream) const {
    for (uint32_t i = 0; i < SNAPSHOT_MAX_VERTEX_ATTRIBS; i++) {
        attribs[i].toStream(stream);
    }
}
void GLVAOState::fromStream(android::base::Stream* stream) {
    for (uint32_t i = 0; i < SNAPSHOT_MAX_VERTEX_ATTRIBS; i++) {
        attribs[i].fromStream(stream);
    }
}
void GLVAOState::summarize(std::ostringstream& ss, int indentLvl) const {
    for (uint32_t i = 0; i < SNAPSHOT_MAX_VERTEX_ATTRIBS; i++) {
        summarizeVal("attribIndex", i, ss, indentLvl);
        attribs[i].summarize(ss, indentLvl + 1);
    }
}

GLSnapshotState::GLSnapshotState() : mGL(LazyLoadedGLESv2Dispatch::get()) {
    D("init snapshot state");

    mUid = sNextUid++;

    for (unsigned int i = 0; i < ObjectType::NUM_OBJECT_TYPES; i++) {
        mNextName[i] = 1;
    }
}

GLSnapshotState::GLSnapshotState(const GLESv2Dispatch* gl) : mGL(gl) {
    D("init snapshot state");

    mUid = sNextUid++;

    for (unsigned int i = 0; i < ObjectType::NUM_OBJECT_TYPES; i++) {
        mNextName[i] = 1;
    }
}

void GLSnapshotState::getGlobalStateEnum(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<GLenum>& store = mGlobals[name].enums;
    store.resize(size);
    mGL->glGetIntegerv(name, (GLint*)&store[0]);
}

void GLSnapshotState::getGlobalStateByte(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<unsigned char>& store = mGlobals[name].bytes;
    store.resize(size);
    mGL->glGetBooleanv(name, (GLboolean*)&store[0]);
}

void GLSnapshotState::getGlobalStateInt(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<uint32_t>& store = mGlobals[name].ints;
    store.resize(size);
    mGL->glGetIntegerv(name, (GLint*)&store[0]);
}

void GLSnapshotState::getGlobalStateFloat(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<float>& store = mGlobals[name].floats;
    store.resize(size);
    mGL->glGetFloatv(name, (GLfloat*)&store[0]);
}

void GLSnapshotState::getGlobalStateInt64(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<uint64_t>& store = mGlobals[name].int64s;
    store.resize(size);
    mGL->glGetInteger64v(name, (GLint64*)&store[0]);
}

void GLSnapshotState::getGlobalStateEnable(GLenum name) {
    D("save 0x%x", name);
    mEnables[name] = mGL->glIsEnabled(name) == GL_TRUE;
}

void GLSnapshotState::saveUniform(
         GLint location, const GLsizei count, const GLboolean transpose,
         const GLValueType& type, const GLValue& value) {

    GLProgramState& prog = mPrograms[mCurrentProgram];
    GLUniformDesc res;
    res.count = count;
    res.transpose = transpose;
    res.valtype = type;
    res.val = value;

    prog.uniformDescs[location] = res;
}

void GLSnapshotState::restoreUniform(GLint location, const GLUniformDesc& desc) {
    GLsizei count = desc.count;
    GLboolean transpose = desc.transpose;
    GLValueType type = desc.valtype;
    GLValue value = desc.val;
    
    switch (type) {
    case GLValueType::VALTYPE_1I:
        mGL->glUniform1i(location, value.ints[0]);
        break;
    case GLValueType::VALTYPE_1F:
        mGL->glUniform1f(location, value.floats[0]);
        break;
    case GLValueType::VALTYPE_4F:
        mGL->glUniform4f(location,
                         value.floats[0],
                         value.floats[1],
                         value.floats[2],
                         value.floats[3]);
        break;
    case GLValueType::VALTYPE_4FV:
        mGL->glUniform4fv(location,
                          count,
                          &value.floats[0]);
        break;
    case GLValueType::VALTYPE_M4FV:
        mGL->glUniformMatrix4fv(location,
                                count,
                                transpose,
                                &value.floats[0]);
        break;
        // TODO: rest of the uniform types
    default:
        break;
    }
}

GLuint GLSnapshotState::toPhysName(ObjectType type, GLuint name) {
    if (!name) return name;
    return mNames[type][name];
}

GLuint GLSnapshotState::toVirtName(ObjectType type, GLuint name) {
    if (!name) return name;
    return mNamesBack[type][name];
}

std::string GLSnapshotState::summarizeState(const char* tag) const {
    std::ostringstream ss;
    int indentLvl = 0;

    summarizeVal("UID", mUid, ss, indentLvl);
    summarizeMap_32_Custom("Globals", mGlobals, ss, indentLvl);
    summarizeMap_32_32("Enables", mEnables, ss, indentLvl);

    for (uint32_t i = 0; i < NUM_OBJECT_TYPES; i++) {
        std::string typeLabel;
        switch (i) {
        case PROGRAM:
            typeLabel = "Programs"; break;
        case BUFFER:
            typeLabel = "Buffers"; break;
        case FRAMEBUFFER:
            typeLabel = "Framebuffers"; break;
        case RENDERBUFFER:
            typeLabel = "Renderbuffers"; break;
        case TEXTURE:
            typeLabel = "Textures"; break;
        case VERTEXARRAYOBJECT:
            typeLabel = "VAOs"; break;
        case TRANSFORMFEEDBACK:
            typeLabel = "Transformfeedbacks"; break;
        case SAMPLER:
            typeLabel = "Samplers"; break;
        case QUERY:
            typeLabel = "Queries"; break;
        default:
            break;
        }
        summarizeVal("Object names", typeLabel, ss, indentLvl);
        summarizeMap_32_32("Virt->phys", mNames[i], ss, indentLvl + 1);
    }

    summarizeVal("Tracked state" , "", ss, indentLvl);

    summarizeVal("mCurrentProgram", mCurrentProgram, ss, indentLvl);
    summarizeVal("mPackAlignment", mPackAlignment, ss, indentLvl);
    summarizeVal("mUnpackAlignment", mUnpackAlignment, ss, indentLvl);
    summarizeVal("mActiveTexture", mActiveTexture, ss, indentLvl);
    summarizeVal("mCurrVAO", mCurrVAO, ss, indentLvl);

    summarizeMap_32_32("buffer bindings", mBufferBindings, ss, indentLvl);

    for (int i = 0; i < SNAPSHOT_MAX_TEXTURE_UNITS; i++) {
        summarizeVal("Texture unit info", i, ss, indentLvl);
        summarizeMap_32_32("texture bindings", mTextureBindings[i], ss, indentLvl + 1);
    }

    summarizeMap_32_32("mFBOBindings", mFBOBindings, ss, indentLvl);

    summarizeMap_32_Custom("mShaders", mShaders, ss, indentLvl);
    summarizeMap_32_Custom("mPrograms", mPrograms, ss, indentLvl);
    summarizeMap_32_Custom("mBuffers", mBuffers, ss, indentLvl);
    summarizeMap_32_Custom("mFBOS", mFBOs, ss, indentLvl);
    summarizeMap_32_Custom("mTextures", mTextures, ss, indentLvl);
    summarizeMap_32_Custom("mVAOS", mVAOs, ss, indentLvl);

    std::string res = ss.str();

    std::ostringstream fnss;
    fnss << "gl-state-summaries/summary-" << mUid << "-" << tag << ".txt";
    std::string fn = fnss.str();
    FILE* fh = fopen(fn.c_str(), "w");
    fprintf(fh, "%s", res.c_str());
    fclose(fh);
    return res;
}

void GLSnapshotState::onSave(android::base::Stream* stream) {
    D("Saving easily-queryable GL state.");
    save();
    D("Saved easily-queryable GL state.");
    
    D("Saving UID.");
    stream->putBe32(mUid);

    D("Saving globals and enables.");
    saveMap_32_Custom(mGlobals, stream);
    saveMap_32_32(mEnables, stream);

    D("Saving virt/phys name map.");
    for (uint32_t i = 0; i < NUM_OBJECT_TYPES; i++) {
        saveMap_32_32(mNames[i], stream);
        saveMap_32_32(mNamesBack[i], stream);
        stream->putBe32(mNextName[i]);
    }

    D("Saving tracked state.");
    stream->putBe32(mCurrentProgram);
    stream->putBe32(mPackAlignment);
    stream->putBe32(mUnpackAlignment);
    stream->putBe32(mActiveTexture);
    stream->putBe32(mCurrVAO);

    saveMap_32_32(mBufferBindings, stream);

    for (int i = 0; i < SNAPSHOT_MAX_TEXTURE_UNITS; i++) {
        saveMap_32_32(mTextureBindings[i], stream);
    }

    saveMap_32_32(mFBOBindings, stream);

    D("Saving state objects.");
    saveMap_32_Custom(mShaders, stream);
    saveMap_32_Custom(mPrograms, stream);
    saveMap_32_Custom(mBuffers, stream);
    saveMap_32_Custom(mFBOs, stream);
    saveMap_32_Custom(mTextures, stream);
    saveMap_32_Custom(mVAOs, stream);

    D("Done with save.");
}

bool GLSnapshotState::onLoad(android::base::Stream* stream) {
    D("Loading back GL state. Loading UID, globals and enables");
    mUid = stream->getBe32();

    sLock.lock();
    sNextUid = ((mUid + 1) > sNextUid) ? (mUid + 1) : sNextUid;
    sLock.unlock();

    loadMap_32_Custom(mGlobals, stream);
    loadMap_32_32(mEnables, stream);

    D("Loading virt/phys name map.");
    for (uint32_t i = 0; i < NUM_OBJECT_TYPES; i++) {
        loadMap_32_32(mNames[i], stream);
        loadMap_32_32(mNamesBack[i], stream);
        mNextName[i] = stream->getBe32();
    }

    D("Loading tracked state.");
    mCurrentProgram = stream->getBe32();
    mPackAlignment = stream->getBe32();
    mUnpackAlignment = stream->getBe32();
    mActiveTexture = stream->getBe32();
    mCurrVAO = stream->getBe32();

    loadMap_32_32(mBufferBindings, stream);

    for (int i = 0; i < SNAPSHOT_MAX_TEXTURE_UNITS; i++) {
        loadMap_32_32(mTextureBindings[i], stream);
    }

    loadMap_32_32(mFBOBindings, stream);

    D("Loading tracked state objects.");
    loadMap_32_Custom(mShaders, stream);
    loadMap_32_Custom(mPrograms, stream);
    loadMap_32_Custom(mBuffers, stream);
    loadMap_32_Custom(mFBOs, stream);
    loadMap_32_Custom(mTextures, stream);
    loadMap_32_Custom(mVAOs, stream);

    D("Restoring GL state.");
    restore();

    return true;
}

void GLSnapshotState::save() {
    // Save all global state:
    // Enables/disables
    getGlobalStateEnable(GL_DEPTH_TEST);
    getGlobalStateEnable(GL_SCISSOR_TEST);
    getGlobalStateEnable(GL_BLEND);

    // Clear color, blend func, depth range
    getGlobalStateFloat(GL_COLOR_CLEAR_VALUE, 4);

    getGlobalStateInt(GL_BLEND_SRC_RGB, 1);
    getGlobalStateInt(GL_BLEND_SRC_ALPHA, 1);
    getGlobalStateInt(GL_BLEND_DST_RGB, 1);
    getGlobalStateInt(GL_BLEND_DST_ALPHA, 1);

    // Viewport / scissor
    getGlobalStateInt(GL_VIEWPORT, 4);
    getGlobalStateInt(GL_SCISSOR_BOX, 4);

    // Global program state
    getGlobalStateInt(GL_CURRENT_PROGRAM, 1);

    // Global buffer bindings
    getGlobalStateInt(GL_ARRAY_BUFFER_BINDING, 1);
    getGlobalStateInt(GL_ELEMENT_ARRAY_BUFFER_BINDING, 1);
    // TODO
    // getGlobalStateInt(GL_COPY_READ_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_COPY_WRITE_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_PIXEL_PACK_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_PIXEL_UNPACK_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_UNIFORM_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_SHADER_STORAGE_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_ATOMIC_COUNTER_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_DISPATCH_INDIRECT_BUFFER_BINDING, 1);
    // getGlobalStateInt(GL_DRAW_INDIRECT_BUFFER_BINDING, 1);

    // Global texture state
    getGlobalStateInt(GL_ACTIVE_TEXTURE, 1);
    getGlobalStateInt(GL_TEXTURE_BINDING_2D, 1);

    getGlobalStateInt(GL_FRAMEBUFFER_BINDING, 1);

    // TODO: For performance, delay saving uniforms until snapshot save
    // Save program uniform state for all programs
    // char* uniformNameBuf = nullptr;
    // for (const auto& it : mPrograms) {
    //     GLuint program_v = it.first;
    //     GLuint program_p = toPhysName(ObjectType::PROGRAM, program_v);

    //     GLint numActiveUniforms;
    //     GLint activeUniformMaxLen;

    //     glGetProgramiv(program_p, GL_ACTIVE_UNIFORMS, &numActiveUniforms);

    //     if (!numActiveUniforms) continue;

    //     glGetProgramiv(program_p, GL_ACTIVE_UNIFORMS, &activeUniformMaxLen);

    //     uniformNameBuf = new char[activeUniformMaxLen];

    //     for (int i = 0; i < numActiveUniforms; i++) {
    //         GLint uniformCount;
    //         GLenum uniformType;
    //         mGL->glGetActiveUniform(
    //                 program_p, i,
    //                 activeUniformMaxLen,
    //                 NULL,
    //                 &uniformCount,
    //                 &uniformType,
    //                 uniformNameBuf);

    //         // Skip built-in uniform variables
    //         if (!strncmp(uniformNameBuf, "gl_", 3)) continue;

    //         switch (uniformType) {
    //         case GL_FLOAT:
    //         case GL_FLOAT_VEC2:
    //         case GL_FLOAT_VEC3:
    //         case GL_FLOAT_VEC4:
    //         case GL_INT:
    //         case GL_INT_VEC2:
    //         case GL_INT_VEC3:
    //         case GL_INT_VEC4:
    //         case GL_BOOL:
    //         case GL_BOOL_VEC2:
    //         case GL_BOOL_VEC3:
    //         case GL_BOOL_VEC4:
    //         case GL_FLOAT_MAT2:
    //         case GL_FLOAT_MAT3:
    //         case GL_FLOAT_MAT4:
    //         case GL_SAMPLER_2D:
    //         case GL_SAMPLER_CUBE:
    //         default:
    //             break;
    //         }
    //     }
    //     delete [] uniformNameBuf;
    // }
}

void GLSnapshotState::restore() {

    // First do all enables + objects, then do globals, since globals
    // end up binding stuf.

    D("Restoring enables.");
    for (const auto& it : mEnables) {
        if (it.second) {
            mGL->glEnable(it.first);
        } else {
            mGL->glDisable(it.first);
        }
    }

    // Copy current set of names
    GLTypedNameMap currNames(mNames);

    D("Restoring shaders.");
    // Shader objects
    for (auto& it: currNames[ObjectType::PROGRAM]) {
        GLuint shaderName = it.first;
        bool isShader = mShaders.find(shaderName) != mShaders.end();

        if (!isShader) continue;

        const GLShaderState& shaderState = mShaders[shaderName];

        GLuint newPhysName = mGL->glCreateShader(shaderState.type);
        refreshName(ObjectType::PROGRAM, shaderName, newPhysName);

        if (shaderState.source.size()) {
            GLint len = shaderState.source.size();
            const char* source = shaderState.source.c_str();
            const char** sources = &source;
            mGL->glShaderSource(newPhysName, 1, sources, &len);
        }
        if (shaderState.compileStatus) {
            mGL->glCompileShader(newPhysName);
        }
    }
    
    // Program objects
    D("Restoring programs.");
    for (auto& it: currNames[ObjectType::PROGRAM]) {
        GLuint programName = it.first;
        bool isProgram = mPrograms.find(programName) != mPrograms.end();

        if (!isProgram) continue;

        GLuint newPhysName = mGL->glCreateProgram();
        refreshName(ObjectType::PROGRAM, programName, newPhysName);

        const GLProgramState& programState = mPrograms[programName];

        for (const auto& jt : programState.linkage) {
            GLuint shaderPhys = toPhysName(ObjectType::PROGRAM, jt.second);
            mGL->glAttachShader(newPhysName, shaderPhys);
        }

        if (programState.linkStatus) {
            mGL->glLinkProgram(newPhysName);
            mGL->glUseProgram(newPhysName);

            for (const auto& jt : programState.boundAttribLocs) {
                mGL->glBindAttribLocation(newPhysName, jt.first, jt.second.c_str());
            }

            for (const auto& jt : programState.uniformDescs) {
                GLint uniformLoc = jt.first;
                const GLUniformDesc& u = jt.second;
                restoreUniform(uniformLoc, u);
            }

            mGL->glUseProgram(0);
        }
    }

    D("Restoring buffers.");
    // Buffer objects
    for (auto& it: currNames[ObjectType::BUFFER]) {
        GLuint bufferName = it.first;

        GLuint newPhysName; mGL->glGenBuffers(1, &newPhysName);
        refreshName(ObjectType::BUFFER, bufferName, newPhysName);

        const GLBufferState& bufferState = mBuffers[bufferName];

        if (!bufferState.size) continue;

        // It's OK; restore buffer binding state later
        mGL->glBindBuffer(GL_ARRAY_BUFFER, newPhysName);
        mGL->glBufferData(GL_ARRAY_BUFFER, bufferState.size, bufferState.data, bufferState.usage);
        mGL->glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    D("Restoring textures.");
    mGL->glActiveTexture(GL_TEXTURE0);

    // Texture objects (must be before FBOs in case they use a texture-backed FBO)
    for (auto& it: currNames[ObjectType::TEXTURE]) {
        GLuint textureName = it.first;

        GLuint newPhysName; mGL->glGenTextures(1, &newPhysName);
        refreshName(ObjectType::TEXTURE, textureName, newPhysName);

        const GLTextureState& textureState = mTextures[textureName];

        mGL->glBindTexture(textureState.boundTarget, newPhysName);
        mGL->glPixelStorei(GL_UNPACK_ALIGNMENT, textureState.unpackAlignment);
        for (int level = 0; level < SNAPSHOT_MAX_TEXTURE_LEVELS; level++) {
            if (!textureState.data[level]) continue;
            mGL->glTexImage2D(
                    textureState.boundTarget,
                    level,
                    textureState.internalformat,
                    textureState.width[level],
                    textureState.height[level],
                    0,
                    textureState.format,
                    textureState.type,
                    textureState.data[level]);
        }
        mGL->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        mGL->glBindTexture(textureState.boundTarget, 0);
    }

    // TODO: Texture binding state w.r.t all texture units
    // In boot, only GL_TEXTURE0 is used.

    D("Restoring FBOs.");
    // FBO
    for (auto& it: currNames[ObjectType::FRAMEBUFFER]) {
        GLuint fboName = it.first;

        GLuint newPhysName; mGL->glGenFramebuffers(1, &newPhysName);
        refreshName(ObjectType::FRAMEBUFFER, fboName, newPhysName);

        const GLFBOState& fboState = mFBOs[fboName];

        mGL->glBindFramebuffer(GL_FRAMEBUFFER, newPhysName);
        for (const auto& jt : fboState.attachments) {
            GLenum attachment = jt.first;

            const GLFBOAttachmentInfo& info = jt.second;

            switch (info.type) {
            case GL_TEXTURE_2D:
                mGL->glFramebufferTexture2D(
                        GL_FRAMEBUFFER,
                        attachment,
                        info.texture.textarget,
                        toPhysName(ObjectType::TEXTURE, info.texture.texture),
                        info.texture.level);
            case GL_RENDERBUFFER:
            default:
                break;
            }
        }
        mGL->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // TODO: FBO binding state w.r.t. all FBO targets
    // In boot, only GL_FRAMEBUFFER is used.

    D("Restoring global state and buffer bindings.");

    GLenum blendRGBSrc = GL_ONE;
    GLenum blendRGBDst = GL_ONE;

    GLenum blendAlphaSrc = GL_ONE;
    GLenum blendAlphaDst = GL_ONE;

    for (const auto& it : mGlobals) {
        const GLValue& val = it.second;
        std::vector<float> floats = val.floats;
        switch (it.first) {
        case GL_COLOR_CLEAR_VALUE:
            mGL->glClearColor(floats[0], floats[1], floats[2], floats[3]);
            break;
        case GL_ACTIVE_TEXTURE:
            mGL->glActiveTexture(val.ints[0]);
            break;
        case GL_TEXTURE_BINDING_2D:
            mGL->glBindTexture(GL_TEXTURE_2D, val.ints[0]);
            break;
        case GL_CURRENT_PROGRAM:
            mGL->glUseProgram(val.ints[0]);
            break;
        case GL_ARRAY_BUFFER_BINDING:
            mGL->glBindBuffer(GL_ARRAY_BUFFER, val.ints[0]);
            break;
        case GL_ELEMENT_ARRAY_BUFFER_BINDING:
            mGL->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, val.ints[0]);
            break;
        case GL_FRAMEBUFFER_BINDING:
            mGL->glBindFramebuffer(GL_FRAMEBUFFER, val.ints[0]);
            break;
        case GL_BLEND_SRC_RGB:
            blendRGBSrc = val.ints[0];
            break;
        case GL_BLEND_SRC_ALPHA:
            blendAlphaSrc = val.ints[0];
            break;
        case GL_BLEND_DST_RGB:
            blendRGBDst = val.ints[0];
            break;
        case GL_BLEND_DST_ALPHA:
            blendAlphaDst = val.ints[0];
            break;
        case GL_VIEWPORT:
            mGL->glViewport(val.ints[0], val.ints[1], val.ints[2], val.ints[3]);
            break;
        case GL_SCISSOR_BOX:
            mGL->glScissor(val.ints[0], val.ints[1], val.ints[2], val.ints[3]);
            break;
        default:
            break;
        }
    }

    if (blendRGBSrc != GL_ONE || blendRGBDst != GL_ONE)
        mGL->glBlendFunc(blendRGBSrc, blendRGBDst);

    if (blendAlphaSrc != GL_ONE || blendAlphaDst != GL_ONE)
        mGL->glBlendFunc(blendAlphaSrc, blendAlphaDst);
}

GLuint GLSnapshotState::glCreateShader(GLuint shader, GLenum shaderType) {
    D("call");
    GLuint res = genName(ObjectType::PROGRAM, shader);
    mShaders[res].type = shaderType;
    return res;
}

GLuint GLSnapshotState::glCreateProgram(GLuint program) {
    D("call");
    return genName(ObjectType::PROGRAM, program, true);
}

void GLSnapshotState::glGenBuffers(GLsizei n, GLuint* buffers) {
    D("call");
    genMulti(ObjectType::BUFFER, n, buffers, buffers);
}

void GLSnapshotState::glGenFramebuffers(GLsizei n, GLuint* fbos) {
    D("call");
    genMulti(ObjectType::FRAMEBUFFER, n, fbos, fbos);
}

void GLSnapshotState::glGenTextures(GLsizei n, GLuint* textures) {
    D("call");
    genMulti(ObjectType::TEXTURE, n, textures, textures);
}

void GLSnapshotState::glDeleteShader(GLuint shader) {
    D("call");
    cleanupName(ObjectType::PROGRAM, shader);
}

void GLSnapshotState::glDeleteProgram(GLuint program) {
    D("call");
    cleanupName(ObjectType::PROGRAM, program);
}

void GLSnapshotState::glDeleteBuffers(GLsizei n, const GLuint* buffers) {
    D("call");
    cleanupMulti(ObjectType::BUFFER, n, buffers);
}

void GLSnapshotState::glDeleteFramebuffers(GLsizei n, const GLuint* fbos) {
    D("call");
    cleanupMulti(ObjectType::FRAMEBUFFER, n, fbos);
}

void GLSnapshotState::glDeleteTextures(GLsizei n, const GLuint* textures) {
    D("call");
    cleanupMulti(ObjectType::TEXTURE, n, textures);
}

GLuint GLSnapshotState::getName(GLSnapshotState::ObjectType type, GLuint name) {
    if (!name) return name;
    return mNames[type][name];
}

void GLSnapshotState::glShaderString(GLuint shader, const GLchar* string) {
    D("call");
    mShaders[toVirtName(ObjectType::PROGRAM, shader)].source = std::string(string);
}

void GLSnapshotState::glAttachShader(GLuint program, GLuint shader) {
    D("call");
    GLuint program_v = toVirtName(ObjectType::PROGRAM, program);
    GLuint shader_v = toVirtName(ObjectType::PROGRAM, shader);
    GLenum shaderType = mShaders[shader_v].type;
    mPrograms[program_v].linkage[shaderType] = shader_v;
}

void GLSnapshotState::glCompileShader(GLuint shader) {
    D("call");
    GLuint shader_v = toVirtName(ObjectType::PROGRAM, shader);
    mShaders[shader_v].compileStatus = true;
}

void GLSnapshotState::glLinkProgram(GLuint program) {
    D("call");
    GLuint program_v = toVirtName(ObjectType::PROGRAM, program);
    mPrograms[program_v].linkStatus = true;
}

void GLSnapshotState::glUseProgram(GLuint program) {
    D("call");
    mCurrentProgram = toVirtName(ObjectType::PROGRAM, program);
}

void GLSnapshotState::glBindAttribLocation(GLuint program, GLint index, const GLchar* name) {
    D("call");
    GLuint program_v = toVirtName(ObjectType::PROGRAM, program);
    mPrograms[program_v].boundAttribLocs[index] = name;
}

void GLSnapshotState::glUniform1i(GLint location, GLint x) {
    D("call");
    GLValue val; val.ints.resize(1); val.ints[0] = x;
    saveUniform(location, 0, GL_FALSE, GLValueType::VALTYPE_1I, val);
}

void GLSnapshotState::glUniform1f(GLint location, GLfloat x) {
    D("call");
    GLValue val; val.floats.resize(1); val.floats[0] = x;
    saveUniform(location, 0, GL_FALSE, GLValueType::VALTYPE_1F, val);
}

void GLSnapshotState::glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    D("call");
    GLValue val; val.floats.resize(4);
    val.floats[0] = x; val.floats[1] = y; val.floats[2] = z; val.floats[3] = w;
    saveUniform(location, 0, GL_FALSE, GLValueType::VALTYPE_4F, val);
}

void GLSnapshotState::glUniform4fv(GLint location, GLsizei count, const GLfloat* vals) {
    D("call");
    GLValue val; val.floats.resize(count * 4);
    memcpy(&val.floats[0], vals, count * 4 * sizeof(float));
    saveUniform(location, count, GL_FALSE, GLValueType::VALTYPE_4FV, val);
}

void GLSnapshotState::glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    D("call");
    GLValue val; val.floats.resize(count * 16);
    memcpy(&val.floats[0], value, count * 16 * sizeof(float));
    saveUniform(location, count, transpose, GLValueType::VALTYPE_M4FV, val);
}

void GLSnapshotState::glBindBuffer(GLenum target, GLuint buffer) {
    D("call");
    GLuint buffer_v = toVirtName(ObjectType::BUFFER, buffer);
    mBufferBindings[target] = buffer_v;
}

void GLSnapshotState::glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) {
    D("call");
    GLuint buffer_v = mBufferBindings[target];
    GLBufferState& buf = mBuffers[buffer_v];
    buf.data = ::realloc(buf.data, size);
    if (data) memcpy(buf.data, data, size);
    buf.size = size;
    buf.usage = usage;
}

void GLSnapshotState::glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data) {
    D("call");
    GLuint buffer_v = mBufferBindings[target];
    GLBufferState& buf = mBuffers[buffer_v];
    memcpy(((char*)buf.data) + offset, data, size);
}

void GLSnapshotState::glBindTexture(GLenum target, GLuint texture) {
    D("call");
    GLuint texture_v = toVirtName(ObjectType::TEXTURE, texture);
    mTextureBindings[mActiveTexture - GL_TEXTURE0][target] = texture_v;
}

void GLSnapshotState::glActiveTexture(GLenum unit) {
    D("call");
    mActiveTexture = unit;
}

void GLSnapshotState::glPixelStorei(GLenum pname, GLint param) {
    D("call");
    switch (pname) {
    case GL_PACK_ALIGNMENT:
        mPackAlignment = param;
        break;
    case GL_UNPACK_ALIGNMENT:
        mUnpackAlignment = param;
        break;
    // TODO: other pixel store enums
    default:
        break;
    }
}

static uint32_t sAlign(uint32_t v, uint32_t align) {

    uint32_t rem = v % align;
    return rem ? (v + (align - rem)) : v;

}

static uint32_t sTexPixelSize(GLenum internalformat,
                              GLenum format,
                              GLenum type) {
    uint32_t reps = 3;
    switch (internalformat) {
    case GL_ALPHA:
        reps = 1;
        break;
    case GL_RGB:
        if (type == GL_UNSIGNED_SHORT_5_6_5)
            reps = 1;
        else
            reps = 3;
        break;
    case GL_RGBA:
        reps = 4;
        break;
    default:
        break;
    }

    uint32_t eltSize = 1;

    switch (type) {
    case GL_UNSIGNED_SHORT_5_6_5:
        eltSize = 2;
        break;
    case GL_UNSIGNED_BYTE:
        eltSize = 1;
        break;
    default:
        break;
    }

    uint32_t pixelSize = reps * eltSize;

    return pixelSize;
}

static uint32_t sTexImageSize(GLenum internalformat,
                              GLenum format,
                              GLenum type,
                              int unpackAlignment,
                              GLsizei width, GLsizei height) {

    uint32_t alignedWidth = sAlign(width, unpackAlignment);
    uint32_t pixelSize = sTexPixelSize(internalformat, format, type);
    uint32_t totalSize = pixelSize * alignedWidth * height;

    return totalSize;
}

void GLSnapshotState::glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels) {
    D("call");

    GLuint texture_v = mTextureBindings[mActiveTexture - GL_TEXTURE0][target];
    GLTextureState& tex = mTextures[texture_v];

    uint32_t totalSize = sTexImageSize(internalformat, format, type, mUnpackAlignment, width, height);

    D("texture %u: allocate %u", texture_v, totalSize);

    tex.nbytes[level] = totalSize;

    tex.data[level] = realloc(tex.data[level], totalSize);

    if (pixels) {
        memcpy(tex.data[level], pixels, totalSize);
    }

    tex.width[level] = width;
    tex.height[level] = height;

    tex.internalformat = internalformat;
    tex.format = format;
    tex.type = type;

    tex.unpackAlignment = mUnpackAlignment;
}

void GLSnapshotState::glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels) {
    D("call");

    GLuint texture_v = mTextureBindings[mActiveTexture - GL_TEXTURE0][target];
    GLTextureState& tex = mTextures[texture_v];

    if (pixels) {
        D("texture %u: subupdate begin for lvl %d.", texture_v, level);
        uint32_t srcPixelSize = sTexPixelSize(format, format, type);
        uint32_t dstPixelSize = sTexPixelSize(tex.internalformat, tex.format, tex.type);

        uint32_t alignedSrcWidth = sAlign(width, mUnpackAlignment) * srcPixelSize;
        uint32_t alignedDstWidth = sAlign(tex.width[level], mUnpackAlignment) * dstPixelSize;

        char* dst = (char*)tex.data[level];
       
        // EGL images...  if (!tex.nbytes[level]) return;

        D("dst: [%p %p]", dst, dst + tex.nbytes[level]);

        const char* src = (const char*)pixels;

        for (int y = 0; y < height; y++) {
            char* dstRowStart = dst + (yoffset + y) * alignedDstWidth + xoffset * dstPixelSize;
            const char* srcRowStart = src + y * alignedSrcWidth;
            D("update: [%p %p] -> [%p %p]", srcRowStart, srcRowStart + alignedSrcWidth, dstRowStart, dstRowStart + alignedDstWidth);
            memcpy(dstRowStart, srcRowStart, alignedSrcWidth);
        }
    }
}

void GLSnapshotState::glTexParameteri(GLenum target, GLenum pname, GLint param) {
    D("call");
    GLuint texture_v = mTextureBindings[mActiveTexture - GL_TEXTURE0][target];
    GLTextureState& tex = mTextures[texture_v];

    switch (pname) {
    case GL_TEXTURE_MAG_FILTER:
        tex.magFilter = param;
        break;
    case GL_TEXTURE_MIN_FILTER:
        tex.minFilter = param;
        break;
    case GL_TEXTURE_WRAP_S:
        tex.wrapS = param;
        break;
    case GL_TEXTURE_WRAP_T:
        tex.wrapT = param;
        break;
    default:
        break;
    }
}

void GLSnapshotState::glBindFramebuffer(GLenum target, GLuint framebuffer) {
    D("call");
    GLuint framebuffer_v = toVirtName(ObjectType::FRAMEBUFFER, framebuffer);
    mFBOBindings[target] = framebuffer_v;
}

void GLSnapshotState::glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    D("call");
    GLuint fbo_v = mFBOBindings[target];

    GLFBOState& fbo = mFBOs[fbo_v];

    GLFBOAttachmentInfo info;
    info.type = GL_TEXTURE_2D;
    info.texture.textarget = textarget;
    info.texture.texture = texture;
    info.texture.level = level;

    fbo.attachments[target] = info;
}

void GLSnapshotState::glEnableVertexAttribArray(GLuint index) {
    D("call");
    mVAOs[mCurrVAO].attribs[index].enabled = true;
}

void GLSnapshotState::glDisableVertexAttribArray(GLuint index) {
    D("call");
    mVAOs[mCurrVAO].attribs[index].enabled = false;
}

void GLSnapshotState::glVertexAttribPointerOffset(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint offset) {
    D("call");

    mVAOs[mCurrVAO].attribs[index].size = size;
    mVAOs[mCurrVAO].attribs[index].type = type;
    mVAOs[mCurrVAO].attribs[index].normalized = normalized;
    mVAOs[mCurrVAO].attribs[index].stride = stride;

    mVAOs[mCurrVAO].attribs[index].arrayBuffer = mBufferBindings[GL_ARRAY_BUFFER];

    mVAOs[mCurrVAO].attribs[index].offset = offset;
}

void GLSnapshotState::glVertexAttribPointerData(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, void* data, GLuint datalen) {
    D("call");

    mVAOs[mCurrVAO].attribs[index].size = size;
    mVAOs[mCurrVAO].attribs[index].type = type;
    mVAOs[mCurrVAO].attribs[index].normalized = normalized;
    mVAOs[mCurrVAO].attribs[index].stride = stride;

    mVAOs[mCurrVAO].attribs[index].arrayBuffer = 0;

    void* dataDst = (void*)mVAOs[mCurrVAO].attribs[index].data;
    dataDst = realloc(dataDst, datalen);
    if (data) memcpy((char*)dataDst, data, datalen);

    mVAOs[mCurrVAO].attribs[index].data = dataDst;
}

} // namespace GLSnapshot
