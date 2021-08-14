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
static intptr_t mgl_gfx_tex_int_id;
static bool mgl_gfx_tex_have; // true если текстура mgl_gfx_tex_int_id установлена через glBindTexture

static bool MGL_CALLCONV mglGfxInitGfxApi(int win_width, int win_height, int viewport_width, int viewport_height, int bkg_red, int bkg_green, int bkg_blue, HDC wnd_dc);
static void MGL_CALLCONV mglGfxDestroyGfxApi(void);
static void MGL_CALLCONV mglGfxSetScreen(int win_width, int win_height, int viewport_width, int viewport_height);
static void MGL_CALLCONV mglGfxClearScreen(int bkg_red, int bkg_green, int bkg_blue);
static void MGL_CALLCONV mglGfxSwapBuffers(HDC wnd_dc);
static bool MGL_CALLCONV mglGfxCreateTexture(unsigned int tex_width, unsigned int tex_height, int tex_format, int tex_filters, void *buffer, uintptr_t *tex_int);
static void MGL_CALLCONV mglGfxDestroyTexture(uintptr_t tex_int_id);
static void MGL_CALLCONV mglGfxDrawTriangle(bool no_texture, uintptr_t tex_int_id,
	float x1, float y1, float tex_x1, float tex_y1, float col_r1, float col_g1, float col_b1,
	float x2, float y2, float tex_x2, float tex_y2, float col_r2, float col_g2, float col_b2,
	float x3, float y3, float tex_x3, float tex_y3, float col_r3, float col_g3, float col_b3);
static wchar_t * MGL_CALLCONV mglGfxGetInfo(void);
static void MGL_CALLCONV mglGfxDestroyInfo(wchar_t* info);

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
	api.CreateTexture = mglGfxCreateTexture;
	api.DestroyTexture = mglGfxDestroyTexture;
	api.DrawTriangle = mglGfxDrawTriangle;
	api.GetInfo = mglGfxGetInfo;
	api.DestroyInfo = mglGfxDestroyInfo;

	return api;
}

static bool MGL_CALLCONV mglGfxInitGfxApi(int win_width, int win_height, int viewport_width, int viewport_height, int bkg_red, int bkg_green, int bkg_blue, HDC wnd_dc)
{
	PIXELFORMATDESCRIPTOR pfd;
	int pixelformat;

	// Choose and set pixel format

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

	mglGfxSetScreen(win_width, win_height, viewport_width, viewport_height);
	mglGfxClearScreen(bkg_red, bkg_green, bkg_blue);

	mgl_gfx_tex_int_id = 0;
	mgl_gfx_tex_have = false;

	return true;
}

static void MGL_CALLCONV mglGfxDestroyGfxApi(void)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(mgl_gfx_wnd_glctx);
}

static void MGL_CALLCONV mglGfxSetScreen(int win_width, int win_height, int viewport_width, int viewport_height)
{
	glViewport(0, 0, win_width, win_height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluOrtho2D(0, viewport_width, viewport_height, 0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static void MGL_CALLCONV mglGfxClearScreen(int bkg_red, int bkg_green, int bkg_blue)
{
	glClearColor(bkg_red / 255.0f, bkg_green / 255.0f, bkg_blue / 255.0f, 0.0f);

	glClear(GL_COLOR_BUFFER_BIT);
}

static GLint mglGfxTexGetIntFormat(int tex_format)
{
	switch(tex_format) {
		case MGL_GFX_TEX_FORMAT_R8G8B8:
			return GL_RGB;
		case MGL_GFX_TEX_FORMAT_R8G8B8A8:
			return GL_RGBA;
		case MGL_GFX_TEX_FORMAT_GRAYSCALE:
			return GL_LUMINANCE8;
		default:
			return 0;
	}
}

static GLint mglGfxTexGetFormat(int tex_format)
{
	switch(tex_format) {
		case MGL_GFX_TEX_FORMAT_R8G8B8:
			return GL_RGB;
		case MGL_GFX_TEX_FORMAT_R8G8B8A8:
			return GL_RGBA;
		case MGL_GFX_TEX_FORMAT_GRAYSCALE:
			return GL_LUMINANCE;
		default:
			return 0;
	}
}

static size_t mglGfxTexGetBytesPerPixel(int tex_format)
{
	switch(tex_format) {
		case MGL_GFX_TEX_FORMAT_R8G8B8:
			return 3;
		case MGL_GFX_TEX_FORMAT_R8G8B8A8:
			return 4;
		case MGL_GFX_TEX_FORMAT_GRAYSCALE:
			return 1;
		default:
			return 0;
	}
}

static size_t mglGfxTexGetBytesPerRow(unsigned int tex_width, int tex_format)
{
	size_t bpp;

	bpp = mglGfxTexGetBytesPerPixel(tex_format);

	if(SIZE_MAX / bpp < tex_width)
		return 0;
	else
		return tex_width * bpp;
}

static bool MGL_CALLCONV mglGfxCreateTexture(unsigned int tex_width, unsigned int tex_height, int tex_format, int tex_filters, void *buffer, uintptr_t *tex_int)
{
	GLuint gl_tex_id;

	// Создание текстуры

	glGetError(); // Сбрасываем флаги ошибки

	glGenTextures(1, &gl_tex_id);
	if(glGetError()) {
		*tex_int = 0;

		return false;
	}

	glBindTexture(GL_TEXTURE_2D, gl_tex_id);

	*tex_int = gl_tex_id;

	if(tex_filters & MGL_GFX_TEX_FILTER_LINEAR_MIN)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	if(tex_filters & MGL_GFX_TEX_FILTER_LINEAR_MAG)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if(tex_filters & MGL_GFX_TEX_FILTER_REPEAT) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	if(mglGfxTexGetBytesPerRow(tex_width, tex_format) % 4 == 0)
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	else
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexImage2D(GL_TEXTURE_2D, 0, mglGfxTexGetIntFormat(tex_format), tex_width, tex_height, 0, mglGfxTexGetFormat(tex_format), GL_UNSIGNED_BYTE, buffer);

	if(mgl_gfx_tex_have)
		glBindTexture(GL_TEXTURE_2D, (GLuint)(mgl_gfx_tex_int_id));

	return true;
}

static void MGL_CALLCONV mglGfxSwapBuffers(HDC wnd_dc)
{
	SwapBuffers(wnd_dc);
}

static void MGL_CALLCONV mglGfxDestroyTexture(uintptr_t tex_int_id)
{
	GLuint gl_tex_id;

	gl_tex_id = (GLuint)(tex_int_id);

	glDeleteTextures(1, &gl_tex_id);
}

static void MGL_CALLCONV mglGfxDrawTriangle(bool no_texture, uintptr_t tex_int_id,
	float x1, float y1, float tex_x1, float tex_y1, float col_r1, float col_g1, float col_b1,
	float x2, float y2, float tex_x2, float tex_y2, float col_r2, float col_g2, float col_b2,
	float x3, float y3, float tex_x3, float tex_y3, float col_r3, float col_g3, float col_b3)
{
	// Установка текстуры
	if(no_texture == true) {
		if(mgl_gfx_tex_have == true) {
			glDisable(GL_TEXTURE_2D);
			mgl_gfx_tex_have = false;
		}
	}
	else {
		if(mgl_gfx_tex_have == false) {
			glEnable(GL_TEXTURE_2D);
			mgl_gfx_tex_have = true;
			glBindTexture(GL_TEXTURE_2D, (GLuint)(tex_int_id));
			mgl_gfx_tex_int_id = tex_int_id;
		} else if(mgl_gfx_tex_int_id != tex_int_id) {
			glBindTexture(GL_TEXTURE_2D, (GLuint)(tex_int_id));
			mgl_gfx_tex_int_id = tex_int_id;
		}
	}

	// Отрисовка изображения
	glBegin(GL_TRIANGLES);
	if(no_texture == false) glTexCoord2f(tex_x1, tex_y1);
	glColor4f(col_r1, col_g1, col_b1, 1.0f);
	glVertex2f(x1, y1);

	if(no_texture == false) glTexCoord2f(tex_x2, tex_y2);
	glColor4f(col_r2, col_g2, col_b2, 1.0f);
	glVertex2f(x2, y2);

	if(no_texture == false) glTexCoord2f(tex_x3, tex_y3);
	glColor4f(col_r3, col_g3, col_b3, 1.0f);
	glVertex2f(x3, y3);
	glEnd();
}

wchar_t * MGL_CALLCONV mglGfxGetInfo(void)
{
	wchar_t *info;
	const wchar_t vendor[] = L"Vendor: ";
	const wchar_t renderer[] = L"Renderer: ";
	const wchar_t version[] = L"Version: ";
	const wchar_t extensions[] = L"Extensions: ";
	size_t infosize, vendor_size, renderer_size, version_size, ext_size, vendor_label_size, renderer_label_size, version_label_size, ext_label_size, temp_size, temp2_size;

	vendor_size = strlen(glGetString(GL_VENDOR));
	vendor_label_size = wcslen(vendor);
	renderer_size = strlen(glGetString(GL_RENDERER));
	renderer_label_size = wcslen(renderer);
	version_size = strlen(glGetString(GL_VERSION));
	version_label_size = wcslen(version);
	ext_size = strlen(glGetString(GL_EXTENSIONS));
	ext_label_size = wcslen(extensions);
	infosize = vendor_label_size + vendor_size +
		renderer_label_size + renderer_size + 
		version_label_size + version_size +
		ext_label_size + ext_size + 3;
	info = malloc(infosize*sizeof(wchar_t));
	if(info) {
		wcscpy(info, vendor);
		temp2_size = wcslen(info);
		temp_size = temp2_size+vendor_size;
		mbstowcs(info+temp2_size, glGetString(GL_VENDOR), vendor_size);
		info[temp_size] = 0;
		wcscat(info, L"\n");

		wcscat(info, renderer);
		temp2_size = temp_size+1+renderer_label_size;
		temp_size += 1+renderer_label_size+renderer_size;
		mbstowcs(info+temp2_size, glGetString(GL_RENDERER), renderer_size);
		info[temp_size] = 0;
		wcscat(info, L"\n");

		wcscat(info, version);
		temp2_size = temp_size+1+version_label_size;
		temp_size += 1+version_label_size+version_size;
		mbstowcs(info+temp2_size, glGetString(GL_VERSION), version_size);
		info[temp_size] = 0;
		wcscat(info, L"\n");

		wcscat(info, extensions);
		temp2_size = temp_size+ext_label_size+1;
		temp_size += 1+ext_label_size+ext_size;
		mbstowcs(info+temp2_size, glGetString(GL_EXTENSIONS), ext_size);
	}

	return info;
}

void MGL_CALLCONV mglGfxDestroyInfo(wchar_t *info)
{
	if (info)
		free(info);
}
