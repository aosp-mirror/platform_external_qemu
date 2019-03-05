// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/base/GLObjectCounter.h"

#include "android/base/memory/LazyInstance.h"

#include <atomic>
#include <sstream>

namespace android {
namespace base {

enum class NamedObjectType : int {
    NULLTYPE,
    VERTEXBUFFER,
    TEXTURE,
    RENDERBUFFER,
    FRAMEBUFFER,
    SHADER_OR_PROGRAM,
    SAMPLER,
    QUERY,
    VERTEX_ARRAY_OBJECT,
    TRANSFORM_FEEDBACK,
    NUM_OBJECT_TYPES  // Must be last
};

struct NamedObjectCounter {
    std::atomic<size_t> mVertexBuffer{0};
    std::atomic<size_t> mTexture{0};
    std::atomic<size_t> mRenderBuffer{0};
    std::atomic<size_t> mFrameBuffer{0};
    std::atomic<size_t> mShaderOrProgram{0};
    std::atomic<size_t> mSampler{0};
    std::atomic<size_t> mQuery{0};
    std::atomic<size_t> mVertexArrayObject{0};
    std::atomic<size_t> mTransformFeedback{0};
};

class GLObjectCounter::Impl {
public:
    void incCount(int type) {
        switch ((NamedObjectType)type) {
            case NamedObjectType::VERTEXBUFFER:
                mCounter.mVertexBuffer += 1;
                break;
            case NamedObjectType::TEXTURE:
                mCounter.mTexture += 1;
                break;
            case NamedObjectType::RENDERBUFFER:
                mCounter.mRenderBuffer += 1;
                break;
            case NamedObjectType::FRAMEBUFFER:
                mCounter.mFrameBuffer += 1;
                break;
            case NamedObjectType::SHADER_OR_PROGRAM:
                mCounter.mShaderOrProgram += 1;
                break;
            case NamedObjectType::SAMPLER:
                mCounter.mSampler += 1;
                break;
            case NamedObjectType::QUERY:
                mCounter.mQuery += 1;
                break;
            case NamedObjectType::VERTEX_ARRAY_OBJECT:
                mCounter.mVertexArrayObject += 1;
                break;
            case NamedObjectType::TRANSFORM_FEEDBACK:
                mCounter.mTransformFeedback += 1;
                break;
            default:
                break;
        }
    }

    void decCount(int type) {
        switch ((NamedObjectType)type) {
            case NamedObjectType::VERTEXBUFFER:
                mCounter.mVertexBuffer -= 1;
                break;
            case NamedObjectType::TEXTURE:
                mCounter.mTexture -= 1;
                break;
            case NamedObjectType::RENDERBUFFER:
                mCounter.mRenderBuffer -= 1;
                break;
            case NamedObjectType::FRAMEBUFFER:
                mCounter.mFrameBuffer -= 1;
                break;
            case NamedObjectType::SHADER_OR_PROGRAM:
                mCounter.mShaderOrProgram -= 1;
                break;
            case NamedObjectType::SAMPLER:
                mCounter.mSampler -= 1;
                break;
            case NamedObjectType::QUERY:
                mCounter.mQuery -= 1;
                break;
            case NamedObjectType::VERTEX_ARRAY_OBJECT:
                mCounter.mVertexArrayObject -= 1;
                break;
            case NamedObjectType::TRANSFORM_FEEDBACK:
                mCounter.mTransformFeedback -= 1;
                break;
            default:
                break;
        }
    }

    std::vector<size_t> getCounts() {
        return {mCounter.mVertexBuffer.load(), mCounter.mTexture.load(), mCounter.mRenderBuffer.load(),
                mCounter.mFrameBuffer.load(),  mCounter.mShaderOrProgram.load(),
                mCounter.mSampler.load(),      mCounter.mQuery.load(),
                mCounter.mVertexArrayObject.load(), mCounter.mTransformFeedback.load()};
    }

    std::string printUsage() {
        std::stringstream ss;
        ss << "VertexBuffer: " << mCounter.mVertexBuffer.load();
        ss << " Texture: " << mCounter.mTexture.load();
        ss << " RenderBuffer: " << mCounter.mRenderBuffer.load();
        ss << " FrameBuffer: " << mCounter.mFrameBuffer.load();
        ss << " ShaderOrProgram: " << mCounter.mShaderOrProgram.load();
        ss << " Sampler: " << mCounter.mSampler.load();
        ss << " Query: " << mCounter.mQuery.load();
        ss << " VertexArrayObject: " << mCounter.mVertexArrayObject.load();
        ss << " TransformFeedback: " << mCounter.mTransformFeedback.load() << "\n";
        return ss.str();
    }

private:
    NamedObjectCounter mCounter;
};

static android::base::LazyInstance<GLObjectCounter> sGlobal;

GLObjectCounter::GLObjectCounter() : mImpl(new GLObjectCounter::Impl()) {}

void GLObjectCounter::incCount(int type) {
    mImpl->incCount(type);
}

void GLObjectCounter::decCount(int type) {
    mImpl->decCount(type);
}

std::vector<size_t> GLObjectCounter::getCounts() {
    return mImpl->getCounts();
}

std::string GLObjectCounter::printUsage() {
    return mImpl->printUsage();
}

GLObjectCounter* GLObjectCounter::get() {
    return sGlobal.ptr();
}

}  // namespace opengl
}  // namespace android