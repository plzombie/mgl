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
	MGL_GFX_PARAMI_MOUSE_WHEEL
};

enum {
	MGL_GFX_PARAMW_WIN_INPUT_CHARS
};

enum {
	MGL_GFX_KEY_UP = 0,
	MGL_GFX_KEY_JUST_PRESSED = 1, // 0b01
	MGL_GFX_KEY_PRESSED = 3, // 0b11
	MGL_GFX_KEY_RELEASED = 2 // 0b10
};

enum {
	MGL_GFX_WINDOW_MODE_WINDOWED
};

enum {
	MGL_GFX_TEX_FORMAT_R8G8B8,
	MGL_GFX_TEX_FORMAT_R8G8B8A8,
	MGL_GFX_TEX_FORMAT_GRAYSCALE
};

enum {
	MGL_GFX_TEX_FILTER_REPEAT = 0x1,
	MGL_GFX_TEX_FILTER_LINEAR_MIN = 0x2,
	MGL_GFX_TEX_FILTER_LINEAR_MAG = 0x4
};

extern bool mglGfxInit(void);

extern void mglGfxClose(void);

extern void mglGfxUpdate(void);

extern int mglGfxGetParami(int param);

extern bool mglGfxSetParami(int param, int value);

extern wchar_t *mglGfxGetParamw(int param);

extern bool mglGfxSetParamw(int param, wchar_t *value);

extern bool mglGfxSetScreen(int winx, int winy, int mode, int flags);

extern int mglGfxGetKey(int key);

extern size_t mglGfxCreateTextureFromMemory(unsigned int tex_width, unsigned int tex_height, int tex_format, int tex_filters, void *buffer);

extern void mglGfxDestroyTexture(size_t tex_id);

extern bool mglGfxDrawPicture(size_t tex_id, int off_x, int off_y, int toff_x, int toff_y, int size_x, int size_y, float scale_x, float scale_y, int col_r, int col_g, int col_b);

#ifdef __cplusplus
}
#endif

#endif
