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
#include "android/opengl/GLObjectCounter.h"

#include "android/base/memory/LazyInstance.h"

#include <atomic>

namespace android {
namespace opengl {

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
    std::atomic<size_t> mVertexArrayBuffer{0};
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
                mCounter.mVertexArrayBuffer += 1;
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
                mCounter.mVertexArrayBuffer -= 1;
                break;
            default:
                break;
        }
    }

    std::vector<size_t> getCounts() {
        return {mCounter.mVertexBuffer.load(), mCounter.mTexture.load(),
                mCounter.mFrameBuffer.load(),  mCounter.mShaderOrProgram.load(),
                mCounter.mSampler.load(),      mCounter.mQuery.load(),
                mCounter.mVertexBuffer.load()};
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

GLObjectCounter* GLObjectCounter::get() {
    return sGlobal.ptr();
}

}  // namespace opengl
}  // namespace android