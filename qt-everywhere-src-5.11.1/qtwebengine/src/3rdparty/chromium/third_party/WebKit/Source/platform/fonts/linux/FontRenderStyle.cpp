// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/fonts/linux/FontRenderStyle.h"

#include "build/build_config.h"
#include "platform/LayoutTestSupport.h"
#include "platform/fonts/FontDescription.h"
#include "public/platform/Platform.h"
#include "public/platform/linux/WebFontRenderStyle.h"
#include "public/platform/linux/WebSandboxSupport.h"

#include "ui/gfx/font_render_params.h"
#include "ui/gfx/font.h"

namespace blink {

namespace {

// These functions are also implemented in sandbox_ipc_linux.cc
// Converts gfx::FontRenderParams::Hinting to WebFontRenderStyle::hintStyle.
// Returns an int for serialization, but the underlying Blink type is a char.
int ConvertHinting(gfx::FontRenderParams::Hinting hinting) {
  switch (hinting) {
    case gfx::FontRenderParams::HINTING_NONE:   return 0;
    case gfx::FontRenderParams::HINTING_SLIGHT: return 1;
    case gfx::FontRenderParams::HINTING_MEDIUM: return 2;
    case gfx::FontRenderParams::HINTING_FULL:   return 3;
  }
  NOTREACHED() << "Unexpected hinting value " << hinting;
  return 0;
}

// Converts gfx::FontRenderParams::SubpixelRendering to
// WebFontRenderStyle::useSubpixelRendering. Returns an int for serialization,
// but the underlying Blink type is a char.
int ConvertSubpixelRendering(
    gfx::FontRenderParams::SubpixelRendering rendering) {
  switch (rendering) {
    case gfx::FontRenderParams::SUBPIXEL_RENDERING_NONE: return 0;
    case gfx::FontRenderParams::SUBPIXEL_RENDERING_RGB:  return 1;
    case gfx::FontRenderParams::SUBPIXEL_RENDERING_BGR:  return 1;
    case gfx::FontRenderParams::SUBPIXEL_RENDERING_VRGB: return 1;
    case gfx::FontRenderParams::SUBPIXEL_RENDERING_VBGR: return 1;
  }
  NOTREACHED() << "Unexpected subpixel rendering value " << rendering;
  return 0;
}

SkPaint::Hinting g_skia_hinting = SkPaint::kNormal_Hinting;
bool g_use_skia_auto_hint = true;
bool g_use_skia_bitmaps = true;
bool g_use_skia_anti_alias = true;
bool g_use_skia_subpixel_rendering = false;

}  // namespace

// static
void FontRenderStyle::SetHinting(SkPaint::Hinting hinting) {
  g_skia_hinting = hinting;
}

// static
void FontRenderStyle::SetAutoHint(bool use_auto_hint) {
  g_use_skia_auto_hint = use_auto_hint;
}

// static
void FontRenderStyle::SetUseBitmaps(bool use_bitmaps) {
  g_use_skia_bitmaps = use_bitmaps;
}

// static
void FontRenderStyle::SetAntiAlias(bool use_anti_alias) {
  g_use_skia_anti_alias = use_anti_alias;
}

// static
void FontRenderStyle::SetSubpixelRendering(bool use_subpixel_rendering) {
  g_use_skia_subpixel_rendering = use_subpixel_rendering;
}

// static
FontRenderStyle FontRenderStyle::QuerySystem(const CString& family,
                                             float text_size,
                                             SkFontStyle font_style) {
  WebFontRenderStyle style;
#if defined(OS_ANDROID)
  style.SetDefaults();
#else
  // If the the sandbox is disabled, we can query font parameters directly.
  if (!Platform::Current()->GetSandboxSupport()) {
    gfx::FontRenderParamsQuery query;
    if (family.length())
      query.families.push_back(family.data());
    query.pixel_size = text_size;
    switch (font_style.slant()) {
    case SkFontStyle::kUpright_Slant:
        query.style = gfx::Font::NORMAL;
        break;
    case SkFontStyle::kItalic_Slant:
    case SkFontStyle::kOblique_Slant:
        query.style = gfx::Font::ITALIC;
        break;
    }
    query.weight = (gfx::Font::Weight)font_style.weight();
    const gfx::FontRenderParams params = gfx::GetFontRenderParams(query, NULL);
    style.use_bitmaps = params.use_bitmaps;
    style.use_auto_hint = params.autohinter;
    style.use_hinting = params.hinting != gfx::FontRenderParams::HINTING_NONE;
    style.hint_style = ConvertHinting(params.hinting);
    style.use_anti_alias = params.antialiasing;
    style.use_subpixel_rendering = ConvertSubpixelRendering(params.subpixel_rendering);
    style.use_subpixel_positioning = params.subpixel_positioning;
  } else {
    bool is_bold = font_style.weight() >= SkFontStyle::kSemiBold_Weight;
    bool is_italic = font_style.slant() != SkFontStyle::kUpright_Slant;
    const int size_and_style = (((int)text_size) << 2) | (((int)is_bold) << 1) |
                               (((int)is_italic) << 0);
    Platform::Current()->GetSandboxSupport()->GetWebFontRenderStyleForStrike(
        family.data(), size_and_style, &style);
  }
#endif

  FontRenderStyle result;
  style.ToFontRenderStyle(&result);

  // Fix FontRenderStyle::NoPreference to actual styles.
  if (result.use_anti_alias == FontRenderStyle::kNoPreference)
    result.use_anti_alias = g_use_skia_anti_alias;

  if (!result.use_hinting)
    result.hint_style = SkPaint::kNo_Hinting;
  else if (result.use_hinting == FontRenderStyle::kNoPreference)
    result.hint_style = g_skia_hinting;

  if (result.use_bitmaps == FontRenderStyle::kNoPreference)
    result.use_bitmaps = g_use_skia_bitmaps;
  if (result.use_auto_hint == FontRenderStyle::kNoPreference)
    result.use_auto_hint = g_use_skia_auto_hint;
  if (result.use_anti_alias == FontRenderStyle::kNoPreference)
    result.use_anti_alias = g_use_skia_anti_alias;
  if (result.use_subpixel_rendering == FontRenderStyle::kNoPreference)
    result.use_subpixel_rendering = g_use_skia_subpixel_rendering;

  // TestRunner specifically toggles the subpixel positioning flag.
  if (result.use_subpixel_positioning == FontRenderStyle::kNoPreference ||
      LayoutTestSupport::IsRunningLayoutTest())
    result.use_subpixel_positioning = FontDescription::SubpixelPositioning();

  return result;
}

void FontRenderStyle::ApplyToPaintFont(PaintFont& font,
                                       float device_scale_factor) const {
  auto sk_hint_style = static_cast<SkPaint::Hinting>(hint_style);
  font.SetAntiAlias(use_anti_alias);
  font.SetHinting(sk_hint_style);
  font.SetEmbeddedBitmapText(use_bitmaps);
  font.SetAutohinted(use_auto_hint);
  if (use_anti_alias)
    font.SetLcdRenderText(use_subpixel_rendering);

  // Do not enable subpixel text on low-dpi if normal or full hinting is requested.
  bool use_subpixel_text =
      (sk_hint_style < SkPaint::kNormal_Hinting || device_scale_factor > 1.0f);

  // TestRunner specifically toggles the subpixel positioning flag.
  if (use_subpixel_text && !LayoutTestSupport::IsRunningLayoutTest())
    font.SetSubpixelText(true);
  else
    font.SetSubpixelText(use_subpixel_positioning);
}

}  // namespace blink
