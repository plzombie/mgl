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
static bool mgl_gfx_tex_have; // true ���� �������� mgl_gfx_tex_int_id ����������� ����� glBindTexture

#define MGL_GFX_VARRAY_SIZE 30000
#define MGL_GFX_VARRAY_THRESHOLD 18

typedef struct {
	float s, t;
	float r, g, b;
	float x, y, z;
} mgl_gfx_varray_type; // GL_T2F_C3F_V3F
static mgl_gfx_varray_type mgl_gfx_varray[MGL_GFX_VARRAY_SIZE];
static size_t mgl_gfx_varray_counter = 0;
static size_t mgl_gfx_varray_max = MGL_GFX_VARRAY_SIZE;

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
static void MGL_CALLCONV mglGfxDrawTexture(uintptr_t tex_int_id, float x1, float y1, float tex_x1, float tex_y1, float x2, float y2, float tex_x2, float tex_y2, float winy);
static wchar_t * MGL_CALLCONV mglGfxGetInfo(void);
static void MGL_CALLCONV mglGfxDestroyInfo(wchar_t* info);
static bool MGL_CALLCONV mglGfxGetCaps(unsigned int capability);

typedef void (__stdcall* glDrawTextureNV_type)(GLuint texture, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1);
static glDrawTextureNV_type glDrawTextureNV_ptr;

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
	api.DrawTexture = mglGfxDrawTexture;
	api.GetInfo = mglGfxGetInfo;
	api.DestroyInfo = mglGfxDestroyInfo;
	api.GetCaps = mglGfxGetCaps;
	
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
	mgl_gfx_varray_counter = 0;

	if(strstr(glGetString(GL_EXTENSIONS), "GL_NV_draw_texture"))
		glDrawTextureNV_ptr = (glDrawTextureNV_type)wglGetProcAddress("glDrawTextureNV");
	else
		glDrawTextureNV_ptr = 0;

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

	// �������� ��������

	glGetError(); // ���������� ����� ������

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

static void mglGfxFlushDrawBuffer(void)
{
	if(mgl_gfx_varray_counter > 0) {
		if(mgl_gfx_varray_counter > MGL_GFX_VARRAY_THRESHOLD) {
			glInterleavedArrays(GL_T2F_C3F_V3F, 0, mgl_gfx_varray);
			glDrawArrays(GL_TRIANGLES, 0, (GLsizei)mgl_gfx_varray_counter);
		} else { // glBegin works faster
			size_t i;

			glBegin(GL_TRIANGLES);
			for(i = 0; i < mgl_gfx_varray_counter; i++) {
				if(mgl_gfx_tex_have) glTexCoord2f(mgl_gfx_varray[i].s, mgl_gfx_varray[i].t);
				glColor4f(mgl_gfx_varray[i].r, mgl_gfx_varray[i].g, mgl_gfx_varray[i].b, 1.0f);
				glVertex3f(mgl_gfx_varray[i].x, mgl_gfx_varray[i].y, mgl_gfx_varray[i].z);
			}
			glEnd();
		}
		mgl_gfx_varray_counter = 0;
	}
}

static void MGL_CALLCONV mglGfxSwapBuffers(HDC wnd_dc)
{
	mglGfxFlushDrawBuffer();

	SwapBuffers(wnd_dc);
}

static void MGL_CALLCONV mglGfxDestroyTexture(uintptr_t tex_int_id)
{
	GLuint gl_tex_id;

	if(mgl_gfx_tex_have = true && mgl_gfx_tex_int_id == tex_int_id) {
		mglGfxFlushDrawBuffer();
		glDisable(GL_TEXTURE_2D);
		mgl_gfx_tex_have = false;
	}

	gl_tex_id = (GLuint)(tex_int_id);

	glDeleteTextures(1, &gl_tex_id);
}

static void MGL_CALLCONV mglGfxDrawTriangle(bool no_texture, uintptr_t tex_int_id,
	float x1, float y1, float tex_x1, float tex_y1, float col_r1, float col_g1, float col_b1,
	float x2, float y2, float tex_x2, float tex_y2, float col_r2, float col_g2, float col_b2,
	float x3, float y3, float tex_x3, float tex_y3, float col_r3, float col_g3, float col_b3)
{
	// ��������� ��������
	if(no_texture == true) {
		if(mgl_gfx_tex_have == true) {
			mglGfxFlushDrawBuffer();
			glDisable(GL_TEXTURE_2D);
			mgl_gfx_tex_have = false;
		}
	}
	else {
		if(mgl_gfx_tex_have == false) {
			mglGfxFlushDrawBuffer();
			glEnable(GL_TEXTURE_2D);
			mgl_gfx_tex_have = true;
			glBindTexture(GL_TEXTURE_2D, (GLuint)(tex_int_id));
			mgl_gfx_tex_int_id = tex_int_id;
		} else if(mgl_gfx_tex_int_id != tex_int_id) {
			mglGfxFlushDrawBuffer();
			glBindTexture(GL_TEXTURE_2D, (GLuint)(tex_int_id));
			mgl_gfx_tex_int_id = tex_int_id;
		}
	}

	// ��������� �����������
	if(mgl_gfx_varray_max-mgl_gfx_varray_counter < 3) {
		mglGfxFlushDrawBuffer();
	}
	mgl_gfx_varray[mgl_gfx_varray_counter].s = tex_x1;
	mgl_gfx_varray[mgl_gfx_varray_counter].t = tex_y1;
	mgl_gfx_varray[mgl_gfx_varray_counter].r = col_r1;
	mgl_gfx_varray[mgl_gfx_varray_counter].g = col_g1;
	mgl_gfx_varray[mgl_gfx_varray_counter].b = col_b1;
	mgl_gfx_varray[mgl_gfx_varray_counter].x = x1;
	mgl_gfx_varray[mgl_gfx_varray_counter].y = y1;
	mgl_gfx_varray[mgl_gfx_varray_counter].z = 0;
	mgl_gfx_varray_counter++;

	mgl_gfx_varray[mgl_gfx_varray_counter].s = tex_x2;
	mgl_gfx_varray[mgl_gfx_varray_counter].t = tex_y2;
	mgl_gfx_varray[mgl_gfx_varray_counter].r = col_r2;
	mgl_gfx_varray[mgl_gfx_varray_counter].g = col_g2;
	mgl_gfx_varray[mgl_gfx_varray_counter].b = col_b2;
	mgl_gfx_varray[mgl_gfx_varray_counter].x = x2;
	mgl_gfx_varray[mgl_gfx_varray_counter].y = y2;
	mgl_gfx_varray[mgl_gfx_varray_counter].z = 0;
	mgl_gfx_varray_counter++;

	mgl_gfx_varray[mgl_gfx_varray_counter].s = tex_x3;
	mgl_gfx_varray[mgl_gfx_varray_counter].t = tex_y3;
	mgl_gfx_varray[mgl_gfx_varray_counter].r = col_r3;
	mgl_gfx_varray[mgl_gfx_varray_counter].g = col_g3;
	mgl_gfx_varray[mgl_gfx_varray_counter].b = col_b3;
	mgl_gfx_varray[mgl_gfx_varray_counter].x = x3;
	mgl_gfx_varray[mgl_gfx_varray_counter].y = y3;
	mgl_gfx_varray[mgl_gfx_varray_counter].z = 0;
	mgl_gfx_varray_counter++;
}

static void MGL_CALLCONV mglGfxDrawTexture(uintptr_t tex_int_id, float x1, float y1, float tex_x1, float tex_y1, float x2, float y2, float tex_x2, float tex_y2, float winy)
{
	if(glDrawTextureNV_ptr)
		glDrawTextureNV_ptr((GLuint)tex_int_id, 0, x1, winy-y2, x2, winy-y1, 0, tex_x1, tex_y2, tex_x2, tex_y1);
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
		ext_label_size + ext_size + 4;
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
		info[temp_size] = 0;
	}

	return info;
}

void MGL_CALLCONV mglGfxDestroyInfo(wchar_t *info)
{
	if(info)
		free(info);
}

bool MGL_CALLCONV mglGfxGetCaps(unsigned int capability)
{
	switch(capability) {
		case MGL_GFX_CAPS_TEXTURE_NPOT:
			if(strstr(glGetString(GL_EXTENSIONS), "GL_ARB_texture_non_power_of_two")) return true;
			return false;
		case MGL_GFX_CAPS_TEXTURE_ANISOTROPIC:
			if(strstr(glGetString(GL_EXTENSIONS), "GL_ARB_texture_filter_anisotropic")) return true;
			if(strstr(glGetString(GL_EXTENSIONS), "GL_EXT_texture_filter_anisotropic")) return true;
			return false;
		case MGL_GFX_CAPS_MULTITEXTURE:
			if(strstr(glGetString(GL_EXTENSIONS), "GL_ARB_multitexture")) return true;
			return false;
		case MGL_GFX_CAPS_TEXTURE_RECTANGLE:
			if(strstr(glGetString(GL_EXTENSIONS), "GL_ARB_texture_rectangle")) return true;
			if(strstr(glGetString(GL_EXTENSIONS), "GL_NV_texture_rectangle")) return true;
			return false;
		case MGL_GFX_CAPS_DRAW_TEXTURE:
			//if(strstr(glGetString(GL_EXTENSIONS), "GL_NV_draw_texture")) return true; // Toooooo slow
			return false;
		default:
			return false;
	}
}
