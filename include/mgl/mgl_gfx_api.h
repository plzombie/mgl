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

#ifndef MGL_GFX_API_H
#define MGL_GFX_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mgl_declspec.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>
#include <Windows.h>

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

enum {
	MGL_GFX_CAPS_TEXTURE_NPOT,
	MGL_GFX_CAPS_TEXTURE_ANISOTROPIC,
	MGL_GFX_CAPS_MULTITEXTURE,
	MGL_GFX_CAPS_TEXTURE_RECTANGLE,
	MGL_GFX_CAPS_DRAW_TEXTURE
};

typedef struct {
	size_t size;
	bool (MGL_CALLCONV *InitGfxApi)(int win_width, int win_height, int viewport_width, int viewport_height, int bkg_red, int bkg_green, int bkg_blue, HDC wnd_dc);
	void (MGL_CALLCONV *DestroyGfxApi)(void);
	void (MGL_CALLCONV *SetScreen)(int win_width, int win_height, int viewport_width, int viewport_height);
	void (MGL_CALLCONV *ClearScreen)(int bkg_red, int bkg_green, int bkg_blue);
	void (MGL_CALLCONV *SwapBuffers)(HDC wnd_dc);
	bool (MGL_CALLCONV *CreateTexture)(unsigned int tex_width, unsigned int tex_height, int tex_format, int tex_filters, void *buffer, uintptr_t *tex_int);
	void (MGL_CALLCONV *DestroyTexture)(uintptr_t tex_int_id);
	void (MGL_CALLCONV *DrawTriangle)(bool no_texture, uintptr_t tex_int_id,
		float x1, float y1, float tex_x1, float tex_y1, float col_r1, float col_g1, float col_b1,
		float x2, float y2, float tex_x2, float tex_y2, float col_r2, float col_g2, float col_b2,
		float x3, float y3, float tex_x3, float tex_y3, float col_r3, float col_g3, float col_b3);
	void (MGL_CALLCONV *DrawTexture)(uintptr_t tex_int_id, float x1, float y1, float tex_x1, float tex_y1, float x2, float y2, float tex_x2, float tex_y2, float winy);
	wchar_t * (MGL_CALLCONV *GetInfo)(void);
	void (MGL_CALLCONV *DestroyInfo)(wchar_t* info);
	bool (MGL_CALLCONV *GetCaps)(unsigned int capability);
} mgl_gfx_api_type;

#ifdef __cplusplus
}
#endif

#endif
