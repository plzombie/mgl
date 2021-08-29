/*
BSD 2-Clause License

Copyright (c) 2021, Mikhail Morozov
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
#include "../mgl/mgl_stb_image.h"

int main(void)
{
	int winx, winy, winmode = MGL_GFX_WINDOW_MODE_WINDOWED, mouse_x, mouse_y, mouse_bl, color_red = 160, color_green = 64, color_blue = 192;
	wchar_t *gfx_info;
	size_t pic, i;

	if(!mglGfxSetParami(MGL_GFX_PARAMI_WIN_WIDTH, 800))
		wprintf(L"Error setting window width\n");
	if(!mglGfxSetParami(MGL_GFX_PARAMI_WIN_HEIGHT, 560))
		wprintf(L"Error setting window height\n");
	if(!mglGfxSetParami(MGL_GFX_PARAMI_WIN_MODE, winmode))
		wprintf(L"Error setting window mode\n");

	mglGfxSetParami(MGL_GFX_PARAMI_BKG_RED, color_red);
	mglGfxSetParami(MGL_GFX_PARAMI_BKG_GREEN, color_green);
	mglGfxSetParami(MGL_GFX_PARAMI_BKG_BLUE, color_blue);

	if (mglGfxInit() == true) {
		wprintf(L"Window created\n");

		if((gfx_info = mglGfxGetParamw(MGL_GFX_PARAMW_INFO)) != 0)
			wprintf(L"Gfx info:\n%ls\n", gfx_info);
		else
			wprintf(L"Can't get gfx info\n");

		pic = mglGfxCreateTextureWithStbImage("IMG_8315.jpg", MGL_GFX_TEX_FILTER_LINEAR_MAG | MGL_GFX_TEX_FILTER_LINEAR_MAG | MGL_GFX_TEX_FILTER_REPEAT);
		if(pic)
			wprintf(L"Pictures created\n");
		else
			wprintf(L"Pictures not created\n");

		while(!mglGfxGetParami(MGL_GFX_PARAMI_NEED_EXIT)) {
			if(mglGfxGetKey(VK_ESCAPE) != MGL_GFX_KEY_UP)
				break;

			if(mglGfxGetKey(VK_F1) == MGL_GFX_KEY_JUST_PRESSED)
				mglGfxSetScreen(320, 240, winmode, 0);
			else if(mglGfxGetKey(VK_F2) == MGL_GFX_KEY_JUST_PRESSED)
				mglGfxSetScreen(640, 480, winmode, 0);
			else if(mglGfxGetKey(VK_F3) == MGL_GFX_KEY_JUST_PRESSED)
				mglGfxSetScreen(800, 600, winmode, 0);
			else if(mglGfxGetKey(VK_F4) == MGL_GFX_KEY_JUST_PRESSED)
				mglGfxSetScreen(1024, 768, winmode, 0);

			winx = mglGfxGetParami(MGL_GFX_PARAMI_WIN_WIDTH);
			winy = mglGfxGetParami(MGL_GFX_PARAMI_WIN_HEIGHT);

			if(mglGfxGetKey(VK_F5) == MGL_GFX_KEY_JUST_PRESSED) {
				winmode = MGL_GFX_WINDOW_MODE_WINDOWED;
				mglGfxSetScreen(winx, winy, winmode, 0);
			} else if(mglGfxGetKey(VK_F6) == MGL_GFX_KEY_JUST_PRESSED) {
				winmode = MGL_GFX_WINDOW_MODE_WINDOWED_FIXED;
				mglGfxSetScreen(winx, winy, winmode, 0);
			} else if(mglGfxGetKey(VK_F7) == MGL_GFX_KEY_JUST_PRESSED) {
				winmode = MGL_GFX_WINDOW_MODE_FULLSCREEN;
				mglGfxSetScreen(winx, winy, winmode, 0);
			}

			mouse_x = mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_X);
			mouse_y = mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_Y);
			mouse_bl = mglGfxGetParami(MGL_GFX_PARAMI_MOUSE_KEY_LEFT);
			
			for(i = 0; i < 100000; i++)
				mglGfxDrawPicture(pic, rand()%winx, rand()%winy, rand()%256, rand()%256, 256, 256, (float)(rand())/(float)RAND_MAX, (float)(rand())/(float)RAND_MAX, 255, 255, 255);		
			
			mglGfxDrawTriangle(0, (float)mouse_x, (float)mouse_y, 0.0f, 0.0f, 0.25f, 0.75f, 0.5f,
				(float)mouse_x+32.0f, (float)mouse_y, 0.0f, 0.0f, 0.75f, 0.25f, 0.5f,
				(float)mouse_x, (float)mouse_y+32.0f, 0.0f, 0.0f, 0.25f, 0.5f, 0.75f);

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
