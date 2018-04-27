/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef GRAPHICS_TRANSLATION_GLES_TEXTURE_DATA_H_
#define GRAPHICS_TRANSLATION_GLES_TEXTURE_DATA_H_

#include <GLES/gl.h>
#include <vector>

#include "gles/egl_image.h"
#include "gles/object_data.h"

class TextureData : public ObjectData {
public:
    explicit TextureData(ObjectLocalName name);
    ~TextureData() override;

    void Bind(GLenum target, GLint max_levels);
    void Set(GLint level,
             GLuint width,
             GLuint height,
             GLenum format,
             GLenum type);

    void AttachEglImage(const EglImagePtr& image);
    void DetachEglImage();
    bool IsEglImageAttached() const;
    const EglImagePtr& GetAttachedEglImage() const;

    bool IsAutoMipmap() const { return auto_mip_map_; }
    void SetAutoMipmap(bool req) { auto_mip_map_ = req; }

    void SetCropRect(const GLint* crop_rect) {
        crop_rect_[0] = crop_rect[0];
        crop_rect_[1] = crop_rect[1];
        crop_rect_[2] = crop_rect[2];
        crop_rect_[3] = crop_rect[3];
    }
    const GLint* GetCropRect() const { return crop_rect_; }

    GLenum GetTarget() const { return target_; }
    GLuint GetWidth(int level = 0) const { return level_map_[level].width; }
    GLuint GetHeight(int level = 0) const { return level_map_[level].height; }
    GLenum GetType(int level = 0) const { return level_map_[level].type; }
    GLenum GetFormat(int level = 0) const { return level_map_[level].format; }

private:
    struct LevelInfo {
        LevelInfo() {}

        GLenum format{0};
        GLenum type{0};
        GLuint width{0};
        GLuint height{0};
    };
    using LevelMap = std::vector<LevelInfo>;

    GLenum target_;
    EglImagePtr image_;
    LevelMap level_map_;
    bool auto_mip_map_;
    GLint crop_rect_[4];

    TextureData(const TextureData&) = delete;
    TextureData& operator=(TextureData&) = delete;
};

using TextureDataPtr = android::sp<TextureData>;

#endif  // GRAPHICS_TRANSLATION_GLES_TEXTURE_DATA_H_
