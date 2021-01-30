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

#include "mgl_gfx_opengl1.h"

#include <gl/GLU.h>

static HGLRC mgl_gfx_wnd_glctx;

static bool mglGfxInitGfxApi(int win_width, int win_height, int bkg_red, int bkg_green, int bkg_blue, HDC wnd_dc);
static void mglGfxDestroyGfxApi(void);
static void mglGfxSetScreen(int win_width, int win_height);
static void mglGfxClearScreen(int bkg_red, int bkg_green, int bkg_blue);
static void mglGfxSwapBuffers(HDC wnd_dc);
static void mglGfxDestroyTexture(uintptr_t tex_int_id);

mgl_gfx_api_type mglGfxGetOGL1Api(void)
{
	mgl_gfx_api_type api;

	memset(&api, 0, sizeof(mgl_gfx_api_type));
	api.size = sizeof(mgl_gfx_api_type);
	api.InitGfxApi = mglGfxInitGfxApi;
	api.DestroyGfxApi = mglGfxDestroyGfxApi;
	api.SetScreen = mglGfxSetScreen;
	api.ClearScreen = mglGfxClearScreen;
	api.SwapBuffers = mglGfxSwapBuffers;
	//api.CreateTexture
	api.DestroyTexture = mglGfxDestroyTexture;
	//api.DrawPicture

	return api;
}

static bool mglGfxInitGfxApi(int win_width, int win_height, int bkg_red, int bkg_green, int bkg_blue, HDC wnd_dc)
{
	PIXELFORMATDESCRIPTOR pfd;
	int pixelformat;

	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL || PFD_DOUBLEBUFFER || PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 32;

	pixelformat = ChoosePixelFormat(wnd_dc, &pfd);
	if(!pixelformat) return false;

	if(!SetPixelFormat(wnd_dc, pixelformat, &pfd)) return false;

	mgl_gfx_wnd_glctx = wglCreateContext(wnd_dc);

	if(!mgl_gfx_wnd_glctx) return false;

	wglMakeCurrent(wnd_dc, mgl_gfx_wnd_glctx);

	glDisable(GL_DEPTH_TEST);

	mglGfxSetScreen(win_width, win_height);
	mglGfxClearScreen(bkg_red, bkg_green, bkg_blue);

	return true;
}

static void mglGfxDestroyGfxApi(void)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(mgl_gfx_wnd_glctx);
}

static void mglGfxSetScreen(int win_width, int win_height)
{
	glViewport(0, 0, win_width, win_height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluOrtho2D(0, win_width, win_height, 0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static void mglGfxClearScreen(int bkg_red, int bkg_green, int bkg_blue)
{
	glClearColor(bkg_red / 255.0f, bkg_green / 255.0f, bkg_blue / 255.0f, 0.0f);

	glClear(GL_COLOR_BUFFER_BIT);
}

static void mglGfxSwapBuffers(HDC wnd_dc)
{
	SwapBuffers(wnd_dc);
}

static void mglGfxDestroyTexture(uintptr_t tex_int_id)
{
	GLuint gl_tex_id;

	gl_tex_id = (GLuint)(tex_int_id);

	glDeleteTextures(1, &gl_tex_id);
}
