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

#include "mgl_gfx.h"

MGL_API void MGL_APIENTRY mglGfxDrawTriangle(size_t tex_id,
	float x1, float y1, float tex_x1, float tex_y1, float col_r1, float col_g1, float col_b1,
	float x2, float y2, float tex_x2, float tex_y2, float col_r2, float col_g2, float col_b2,
	float x3, float y3, float tex_x3, float tex_y3, float col_r3, float col_g3, float col_b3)
{
	bool no_texture;

	if(!mgl_gfx.mgl_init)
		return;

	if(tex_id == 0 || tex_id > mgl_gfx.textures_max)
		no_texture = true;
	else if (mgl_gfx.textures[tex_id - 1].tex_used == false)
		no_texture = true;
	else
		no_texture = false;

	mgl_gfx.gfx_api.DrawTriangle(no_texture, no_texture?0:mgl_gfx.textures[tex_id - 1].tex_int_id,
		x1, y1, tex_x1, tex_y1, col_r1, col_g1, col_b1,
		x2, y2, tex_x2, tex_y2, col_r2, col_g2, col_b2,
		x3, y3, tex_x3, tex_y3, col_r3, col_g3, col_b3);
}

MGL_API bool MGL_APIENTRY mglGfxDrawPicture(size_t tex_id, int off_x, int off_y, int toff_x, int toff_y, int size_x, int size_y, float scale_x, float scale_y, int col_r, int col_g, int col_b)
{
	uintptr_t tex_int_id;
	float pic_width, pic_height;
	float tex_start_x = 0.0f, tex_end_x = 0.0f, tex_start_y = 0.0f, tex_end_y = 0.0f;
	bool no_texture;

	if(!mgl_gfx.mgl_init)
		return false;

	if(tex_id == 0 || tex_id > mgl_gfx.textures_max)
		no_texture = true;
	else if(mgl_gfx.textures[tex_id - 1].tex_used == false)
		no_texture = true;
	else
		no_texture = false;

	if(size_x <= 0 || size_y <= 0 || scale_x <= 0 || scale_y <= 0)
		return false;

	if(col_r < 0 || col_r > 255 || col_g < 0 || col_g > 255 || col_b < 0 || col_b > 255)
		return false;

	pic_width = size_x * scale_x;
	pic_height = size_y * scale_y;

	if(no_texture == false) {
		tex_start_x = (float)(toff_x) / mgl_gfx.textures[tex_id - 1].tex_width;
		tex_end_x = (float)(toff_x + size_x) / mgl_gfx.textures[tex_id - 1].tex_width;
		tex_start_y = (float)(toff_y) / mgl_gfx.textures[tex_id - 1].tex_height;
		tex_end_y = (float)(toff_y + size_y) / mgl_gfx.textures[tex_id - 1].tex_height;
		tex_int_id = mgl_gfx.textures[tex_id - 1].tex_int_id;
	} else tex_int_id = 0;

	// Отрисовка изображения
	if (!no_texture && mgl_gfx.gfx_drawtexture
		&& col_r == 255 && col_g == 255 && col_b == 255
		&& mgl_gfx.win_virt_width == mgl_gfx.win_width
		&& mgl_gfx.win_virt_height == mgl_gfx.win_height) {
		mgl_gfx.gfx_api.DrawTexture(tex_int_id,
			(float)off_x, (float)off_y,
			tex_start_x, tex_start_y,
			(float)off_x + pic_width, (float)off_y + pic_height,
			tex_end_x, tex_end_y,
			(float)mgl_gfx.win_virt_height);
	} else {
		mgl_gfx.gfx_api.DrawTriangle(no_texture, tex_int_id,
			(float)off_x, (float)off_y, tex_start_x, tex_start_y, col_r / 255.0f, col_g / 255.0f, col_b / 255.0f,
			(float)off_x, (float)off_y + pic_height, tex_start_x, tex_end_y, col_r / 255.0f, col_g / 255.0f, col_b / 255.0f,
			(float)off_x + pic_width, (float)off_y + pic_height, tex_end_x, tex_end_y, col_r / 255.0f, col_g / 255.0f, col_b / 255.0f);
		mgl_gfx.gfx_api.DrawTriangle(no_texture, tex_int_id,
			(float)off_x, (float)off_y, tex_start_x, tex_start_y, col_r / 255.0f, col_g / 255.0f, col_b / 255.0f,
			(float)off_x + pic_width, (float)off_y + pic_height, tex_end_x, tex_end_y, col_r / 255.0f, col_g / 255.0f, col_b / 255.0f,
			(float)off_x + pic_width, (float)off_y, tex_end_x, tex_start_y, col_r / 255.0f, col_g / 255.0f, col_b / 255.0f);
	}

	if (off_x <= mgl_gfx.mouse_virt_x && off_x + pic_width > mgl_gfx.mouse_virt_x &&
		off_y <= mgl_gfx.mouse_virt_y && off_y + pic_height > mgl_gfx.mouse_virt_y)
		return true;
	else
		return false;
}
