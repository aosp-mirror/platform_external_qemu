// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details
//
// window.h

#pragma once

#include <pthread.h>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct AVFrame;

namespace android {
namespace display {

class SDLRender;
class DisplayWindow;

struct PRIVATE_DATA
{
	int width;
	int height;
	DisplayWindow* wfi;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

class DisplayWindow
{
public:	
	DisplayWindow() = default;
	virtual ~DisplayWindow() = default;

	static DisplayWindow* create(int width, int height);
	void remove();
	void resize(int width, int height);	
	void update(unsigned char* rgb, int size, int width, int height);
	void update_yuv(AVFrame* frame, 
		int y_data_size, unsigned char* y_data,
		int u_data_size, unsigned char* u_data,
		int v_data_size, unsigned char* v_data,
		int width, int height);

public:
	int width = 1280;
	int height = 720;
	SDL_Window* win = nullptr;
	SDL_Renderer* ren = nullptr;
	SDL_Texture* tex = nullptr;

	pthread_mutex_t frame_mutex;
	pthread_cond_t frame_cond;

	//friend SDLRender;
};


} // namespace display
} // namespace android
