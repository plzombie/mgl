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

#ifndef MGL_GFX_H
#define MGL_GFX_H

#include "../../include/mgl/mgl_gfx_api.h"

typedef struct {
	unsigned int tex_width, tex_height;
	uintptr_t tex_int_id;
	bool tex_used;
} mgl_gfx_textures_type;

typedef struct {
	mgl_gfx_api_type gfx_api;
	mgl_gfx_textures_type *textures;
	size_t textures_max;
	int win_width, win_height, win_virt_width, win_virt_height;
	int win_dpix, win_dpiy;
	int win_mode;
	int mouse_x, mouse_y, mouse_wheel, mouse_virt_x, mouse_virt_y;
	int mouse_key_l, mouse_key_r, mouse_key_m, mouse_key_4, mouse_key_5;
	int bkg_red, bkg_green, bkg_blue;
	char win_keys[256];
	wchar_t *win_input_chars;
	size_t win_input_chars_max;
	HWND wnd_handle;
	HDC wnd_dc;
	ATOM wnd_class_atom;
	HGLRC wnd_glctx;
	bool mgl_init;
	bool mgl_need_exit;
} mgl_gfx_type;

extern mgl_gfx_type mgl_gfx;

#endif
