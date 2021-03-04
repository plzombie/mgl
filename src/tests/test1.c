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

#include <Windows.h>
#include <gl/GL.h>

#include "../../include/mgl/mgl.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"

#include <stdio.h>
#include <wchar.h>

static void PrintWindowParams(void);
static void PrintWindowDpi(void);
static void PrintKeys(void);
static void PrintMouseButtons(void);
static void PrintInputChars(void);
static size_t CreateTextureWithStbImage(char *filename, int tex_filters);

int main(void)
{
	int winx, winy, winmode = MGL_GFX_WINDOW_MODE_WINDOWED, mouse_x, mouse_y, mouse_bl, color_red = 160, color_green = 64, color_blue = 192;
	size_t pic;
	bool block_hovered = false;

	if(!mglGfxSetParami(MGL_GFX_PARAMI_WIN_WIDTH, 800))
		wprintf(L"Error setting window width\n");
	if(!mglGfxSetParami(MGL_GFX_PARAMI_WIN_HEIGHT, 560))
		wprintf(L"Error setting window height\n");

	mglGfxSetParami(MGL_GFX_PARAMI_BKG_RED, color_red);
	mglGfxSetParami(MGL_GFX_PARAMI_BKG_GREEN, color_green);
	mglGfxSetParami(MGL_GFX_PARAMI_BKG_BLUE, color_blue);

	if (mglGfxInit() == true) {
		wprintf(L"Window created\n");

		pic = CreateTextureWithStbImage("IMG_8315.jpg", MGL_GFX_TEX_FILTER_LINEAR_MAG | MGL_GFX_TEX_FILTER_LINEAR_MAG | MGL_GFX_TEX_FILTER_REPEAT);
		if(pic)
			wprintf(L"Pictures created\n");
		else
			wprintf(L"Pictures not created\n");

		while(!mglGfxGetParami(MGL_GFX_PARAMI_NEED_EXIT)) {
			if(mglGfxGetKey(VK_F1) == MGL_GFX_KEY_JUST_PRESSED)
				mglGfxSetScreen(320, 240, winmode, 0);
			else if(mglGfxGetKey(VK_F2) == MGL_GFX_KEY_JUST_PRESSED)
				mglGfxSetScreen(640, 480, winmode, 0);
			else if(mglGfxGetKey(VK_F3) == MGL_GFX_KEY_JUST_PRESSED)
				mglGfxSetScreen(800, 600, winmode, 0);
			else if(mglGfxGetKey(VK_F4) == MGL_GFX_KEY_JUST_PRESSED)
				mglGfxSetScreen(1024, 768, winmode, 0);

			PrintWindowParams();
			PrintWindowDpi();
			PrintKeys();
			PrintMouseButtons();
			PrintInputChars();

			mouse_x = mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_X);
			mouse_y = mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_Y);
			mouse_bl = mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_KEY_LEFT);

			winx = mglGfxGetParami(MGL_GFX_PARAMI_WIN_WIDTH);
			winy = mglGfxGetParami(MGL_GFX_PARAMI_WIN_HEIGHT);

			mglGfxDrawPicture(0, mouse_x, mouse_y, 0, 0, 16, 16, 1.0f, 2.0f, 64, 192, 128);

			if(block_hovered) {
				if(mouse_bl == MGL_GFX_KEY_PRESSED)
					block_hovered = mglGfxDrawPicture(pic, 100, 100, 0, 0, 512, 512, 0.5f, 0.5f, 255, 255, 255);
				else
					block_hovered = mglGfxDrawPicture(pic, 100, 100, 0, 0, 256, 256, 1.0f, 1.0f, 255, 255, 255);
			} else
				block_hovered = mglGfxDrawPicture(pic, 100, 100, 0, 0, 256, 256, 1.0f, 1.0f, 64, 32, 80);

			if(glGetError())
				wprintf(L"OGL error occured\n");

			mglGfxUpdate();
		}
	} else
		wprintf(L"Can't create window\n");

	mglGfxClose();

	wprintf(L"Window closed\n");

	return 0;
}

size_t CreateTextureWithStbImage(char *filename, int tex_filters)
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

static void PrintWindowParams(void)
{
	static int winx = 0, winy = 0;

	if(mglGfxGetParami(MGL_GFX_PARAMI_WIN_WIDTH) != winx) {
		winx = mglGfxGetParami(MGL_GFX_PARAMI_WIN_WIDTH);
		wprintf(L"winx: %d\n", winx);
	}

	if(mglGfxGetParami(MGL_GFX_PARAMI_WIN_HEIGHT) != winy) {
		winy = mglGfxGetParami(MGL_GFX_PARAMI_WIN_HEIGHT);
		wprintf(L"winy: %d\n", winy);
	}
}

static void PrintWindowDpi(void)
{
	static int dpix = 0, dpiy = 0;

	if(mglGfxGetParami(MGL_GFX_PARAMI_WIN_DPIX) != dpix) {
		dpix = mglGfxGetParami(MGL_GFX_PARAMI_WIN_DPIX);
		wprintf(L"DpiX: %d\n", dpix);
	}

	if(mglGfxGetParami(MGL_GFX_PARAMI_WIN_DPIY) != dpiy) {
		dpiy = mglGfxGetParami(MGL_GFX_PARAMI_WIN_DPIY);
		wprintf(L"DpiY: %d\n", dpiy);
	}
}

static void PrintKeys(void)
{
	char key;

	for(key = '0'; key <= '9'; key++) {
		if(mglGfxGetKey(key) == MGL_GFX_KEY_JUST_PRESSED)
			wprintf(L"Key %c pressed\n", key);
		if(mglGfxGetKey(key) == MGL_GFX_KEY_RELEASED)
			wprintf(L"Key %c released\n", key);
	}

	for(key = 'A'; key <= 'Z'; key++) {
		if(mglGfxGetKey(key) == MGL_GFX_KEY_JUST_PRESSED)
			wprintf(L"Key %c pressed\n", key);
		if(mglGfxGetKey(key) == MGL_GFX_KEY_RELEASED)
			wprintf(L"Key %c released\n", key);
	}
}

static void PrintMouseButtons(void)
{
	if(mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_KEY_LEFT) == MGL_GFX_KEY_JUST_PRESSED)
		wprintf(L"Left mouse button pressed\n");
	else if(mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_KEY_LEFT) == MGL_GFX_KEY_RELEASED)
		wprintf(L"Left mouse button released\n");

	if(mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_KEY_RIGHT) == MGL_GFX_KEY_JUST_PRESSED)
		wprintf(L"Right mouse button pressed\n");
	else if(mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_KEY_RIGHT) == MGL_GFX_KEY_RELEASED)
		wprintf(L"Right mouse button released\n");

	if(mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_KEY_MIDDLE) == MGL_GFX_KEY_JUST_PRESSED)
		wprintf(L"Middle mouse button pressed\n");
	else if(mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_KEY_MIDDLE) == MGL_GFX_KEY_RELEASED)
		wprintf(L"Middle mouse button released\n");

	if(mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_KEY_4) == MGL_GFX_KEY_JUST_PRESSED)
		wprintf(L"4 mouse button pressed\n");
	else if(mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_KEY_4) == MGL_GFX_KEY_RELEASED)
		wprintf(L"4 mouse button released\n");

	if(mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_KEY_5) == MGL_GFX_KEY_JUST_PRESSED)
		wprintf(L"5 mouse button pressed\n");
	else if(mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_KEY_5) == MGL_GFX_KEY_RELEASED)
		wprintf(L"5 mouse button released\n");

	if(mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_WHEEL))
		wprintf(L"Mouse wheel: %d\n", mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_WHEEL));
}

static void PrintInputChars(void)
{
	wchar_t *inp;

	inp = mglGfxGetParamw(MGL_GFX_PARAMW_WIN_INPUT_CHARS);

	if(*inp)
		wprintf(L"Text input: %ls\n", inp);
}
