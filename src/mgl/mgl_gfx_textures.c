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

MGL_API size_t MGL_APIENTRY mglGfxCreateTextureFromMemory(unsigned int tex_width, unsigned int tex_height, int tex_format, int tex_filters, void *buffer)
{
	size_t tex_id = 0;
	uintptr_t tex_int_id = 0;

	// Аллокатор для текстур
	if(mgl_gfx.textures_max == 0) {
		mgl_gfx.textures_max = 2;
		mgl_gfx.textures = malloc(mgl_gfx.textures_max * sizeof(mgl_gfx_textures_type));
		if(!mgl_gfx.textures)
			return 0;

		memset(mgl_gfx.textures, 0, mgl_gfx.textures_max * sizeof(mgl_gfx_textures_type));
	}
	else {
		size_t i;

		for(i = 0; i < mgl_gfx.textures_max; i++)
			if(mgl_gfx.textures[i].tex_used == false)
				break;

		if(i < mgl_gfx.textures_max)
			tex_id = i;
		else {
			mgl_gfx_textures_type *_textures;
			size_t _textures_max;

			_textures_max = mgl_gfx.textures_max * 2;
			_textures = realloc(mgl_gfx.textures, _textures_max * sizeof(mgl_gfx_textures_type));
			if(!_textures)
				return 0;

			tex_id = mgl_gfx.textures_max;
			memset(mgl_gfx.textures + mgl_gfx.textures_max, 0, (_textures_max - mgl_gfx.textures_max) * sizeof(mgl_gfx_textures_type));

			mgl_gfx.textures = _textures;
			mgl_gfx.textures_max = _textures_max;
		}
	}

	if(mgl_gfx.gfx_api.CreateTexture(tex_width, tex_height, tex_format, tex_filters, buffer, &tex_int_id) == false)
		return 0;

	mgl_gfx.textures[tex_id].tex_int_id = tex_int_id;
	mgl_gfx.textures[tex_id].tex_width = tex_width;
	mgl_gfx.textures[tex_id].tex_height = tex_height;
	mgl_gfx.textures[tex_id].tex_used = true;

	return tex_id + 1;
}

MGL_API void MGL_APIENTRY mglGfxDestroyTexture(size_t tex_id)
{
	if(tex_id == 0 || tex_id > mgl_gfx.textures_max)
		return;

	if(mgl_gfx.textures[tex_id - 1].tex_used == false)
		return;

	mgl_gfx.gfx_api.DestroyTexture(mgl_gfx.textures[tex_id - 1].tex_int_id);

	mgl_gfx.textures[tex_id - 1].tex_used = false;
}
