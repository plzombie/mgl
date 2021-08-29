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

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"

#include "../../include/mgl/mgl.h"
#include "../mgl/mgl_stb_image.h"

size_t mglGfxCreateTextureWithStbImage(char *filename, int tex_filters)
{
	int width, height, channels, tex_format;
	unsigned char *buffer;
	size_t tex_id;

	buffer = stbi_load(filename, &width, &height, &channels, 0);

	if(!buffer)
		return 0;

	switch(channels) {
		case 1:
			tex_format = MGL_GFX_TEX_FORMAT_GRAYSCALE;
			break;
		case 3:
			tex_format = MGL_GFX_TEX_FORMAT_R8G8B8;
			break;
		case 4:
			tex_format = MGL_GFX_TEX_FORMAT_R8G8B8A8;
			break;
		default:
			free(buffer);
			return 0;
	}

	tex_id = mglGfxCreateTextureFromMemory(width, height, tex_format, tex_filters, buffer);

	free(buffer);

	return tex_id;
}
