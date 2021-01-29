/*
BSD 2-Clause License

Copyright (c) 2020, Mikhail Morozov
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

#include "../../include/mgl/mgl.h"

#include <stdio.h>
#include <wchar.h>

static void PrintWindowParams(void);

int main(void)
{
	int winx, winy, color_red = 160, color_green = 64, color_blue = 192;

	if(!mglGfxSetParami(MGL_GFX_PARAM_WIN_WIDTH, 800))
		wprintf(L"Error setting window width\n");
	if(!mglGfxSetParami(MGL_GFX_PARAM_WIN_HEIGHT, 560))
		wprintf(L"Error setting window height\n");

	mglGfxSetParami(MGL_GFX_PARAM_BKG_RED, color_red);
	mglGfxSetParami(MGL_GFX_PARAM_BKG_GREEN, color_green);
	mglGfxSetParami(MGL_GFX_PARAM_BKG_BLUE, color_blue);

	if (mglGfxInit() == true) {
		wprintf(L"Window created\n");
		while(!mglGfxGetParami(MGL_GFX_PARAM_NEED_EXIT)) {
			PrintWindowParams();

			winx = mglGfxGetParami(MGL_GFX_PARAM_WIN_WIDTH);
			winy = mglGfxGetParami(MGL_GFX_PARAM_WIN_HEIGHT);

			mglGfxUpdate();
		}
	} else
		wprintf(L"Can't create window\n");

	mglGfxClose();

	wprintf(L"Window closed\n");

	return 0;
}

static void PrintWindowParams(void)
{
	static int winx = 0, winy = 0;

	if(mglGfxGetParami(MGL_GFX_PARAM_WIN_WIDTH) != winx) {
		winx = mglGfxGetParami(MGL_GFX_PARAM_WIN_WIDTH);
		wprintf(L"winx: %d\n", winx);
	}

	if(mglGfxGetParami(MGL_GFX_PARAM_WIN_HEIGHT) != winy) {
		winy = mglGfxGetParami(MGL_GFX_PARAM_WIN_HEIGHT);
		wprintf(L"winy: %d\n", winy);
	}
}