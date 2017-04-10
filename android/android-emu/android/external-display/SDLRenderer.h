// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct AVFrame;

namespace android {
namespace display {

struct PrivateData;
class DisplayWindow;

class SDLRenderer {
public:
    static DisplayWindow* recreateSDLWindow(int width, int height);
    static void draw_frame(DisplayWindow* window, AVFrame* frame);
    static void resize_sdl_window(DisplayWindow* window, int width, int height);
    static void delete_sdl_window(DisplayWindow* window);
    static void draw_frame_internal(DisplayWindow* pWin, AVFrame* frame);
};

void sdl_thread_func(PrivateData* pData);

}  // namespace display
}  // namespace android
