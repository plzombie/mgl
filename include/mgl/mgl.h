/*
BSD 2-Clause License

Copyright (c) 2020-2021, Mikhail Morozov
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MGL_H
#define MGL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mgl_declspec.h"

#include "mgl_gfx_api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

enum {
	MGL_GFX_PARAMI_INIT,
	MGL_GFX_PARAMI_WIN_WIDTH,
	MGL_GFX_PARAMI_WIN_HEIGHT,
	MGL_GFX_PARAMI_NEED_EXIT,
	MGL_GFX_PARAMI_BKG_RED,
	MGL_GFX_PARAMI_BKG_GREEN,
	MGL_GFX_PARAMI_BKG_BLUE,
	MGL_GFX_PARAMI_MOUSE_X,
	MGL_GFX_PARAMI_MOUSE_Y,
	MGL_GFX_PARAMI_MOUSE_KEY_LEFT,
	MGL_GFX_PARAMI_MOUSE_KEY_RIGHT,
	MGL_GFX_PARAMI_MOUSE_KEY_MIDDLE,
	MGL_GFX_PARAMI_MOUSE_KEY_4,
	MGL_GFX_PARAMI_MOUSE_KEY_5,
	MGL_GFX_PARAMI_MOUSE_WHEEL,
	MGL_GFX_PARAMI_WIN_DPIX,
	MGL_GFX_PARAMI_WIN_DPIY,
	MGL_GFX_PARAMI_WIN_MODE
};

enum {
	MGL_GFX_PARAMW_WIN_INPUT_CHARS,
	MGL_GFX_PARAMW_INFO
};

enum {
	MGL_GFX_KEY_UP = 0,
	MGL_GFX_KEY_JUST_PRESSED = 1, // 0b01
	MGL_GFX_KEY_PRESSED = 3, // 0b11
	MGL_GFX_KEY_RELEASED = 2 // 0b10
};

enum {
	MGL_GFX_WINDOW_MODE_WINDOWED,
	MGL_GFX_WINDOW_MODE_WINDOWED_FIXED,
	MGL_GFX_WINDOW_MODE_FULLSCREEN
};

MGL_API bool MGL_APIENTRY mglGfxInit(void);

MGL_API void MGL_APIENTRY mglGfxClose(void);

MGL_API void MGL_APIENTRY mglGfxUpdate(void);

MGL_API int MGL_APIENTRY mglGfxGetParami(int param);

MGL_API bool MGL_APIENTRY mglGfxSetParami(int param, int value);

MGL_API wchar_t * MGL_APIENTRY mglGfxGetParamw(int param);

MGL_API bool MGL_APIENTRY mglGfxSetParamw(int param, wchar_t *value);

MGL_API bool MGL_APIENTRY mglGfxSetScreen(int winx, int winy, int mode, int flags);

MGL_API int MGL_APIENTRY mglGfxGetKey(int key);

MGL_API size_t MGL_APIENTRY mglGfxCreateTextureFromMemory(unsigned int tex_width, unsigned int tex_height, int tex_format, int tex_filters, void *buffer);

MGL_API void MGL_APIENTRY mglGfxDestroyTexture(size_t tex_id);

MGL_API void MGL_APIENTRY mglGfxDrawTriangle(size_t tex_id,
	float x1, float y1, float tex_x1, float tex_y1, float col_r1, float col_g1, float col_b1,
	float x2, float y2, float tex_x2, float tex_y2, float col_r2, float col_g2, float col_b2,
	float x3, float y3, float tex_x3, float tex_y3, float col_r3, float col_g3, float col_b3);

MGL_API bool MGL_APIENTRY mglGfxDrawPicture(size_t tex_id, int off_x, int off_y, int toff_x, int toff_y, int size_x, int size_y, float scale_x, float scale_y, int col_r, int col_g, int col_b);

#ifdef __cplusplus
}
#endif

#endif
