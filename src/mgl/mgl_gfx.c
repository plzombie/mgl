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

#define _WIN32_WINNT 0x0600

#include <Windows.h>
#include <windowsx.h>

#include <gl/GLU.h>

typedef struct {
	unsigned int tex_width, tex_height;
	uintptr_t tex_int_id;
	bool tex_used;
} mgl_gfx_textures_type;

typedef struct {
	intptr_t tex_int_id;
	mgl_gfx_textures_type *textures;
	size_t textures_max;
	bool tex_have; // true если текстура tex_int_id установлена через glBindTexture
	int win_width, win_height;
	int mouse_x, mouse_y, mouse_wheel;
	int mouse_key_l, mouse_key_r, mouse_key_m, mouse_key_4, mouse_key_5;
	int bkg_red, bkg_green, bkg_blue;
	char win_keys[256];
	HWND wnd_handle;
	HDC wnd_dc;
	ATOM wnd_class_atom;
	HGLRC wnd_glctx;
	bool mgl_init;
	bool mgl_need_exit;
} mgl_gfx_type;

static mgl_gfx_type mgl_gfx;

static void mglGfxSetOGLScreen(void);
static void mglGfxClearOGLScreen(void);
static LRESULT CALLBACK mglGfxMainWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

bool mglGfxInit(void)
{
	WNDCLASSW wnd_class;
	HINSTANCE instance;
	DWORD style, ex_style;
	RECT wnd_rect;
	PIXELFORMATDESCRIPTOR pfd;
	int pixelformat;
	
	if(mgl_gfx.mgl_init == true)
		return false;

	instance = GetModuleHandle(NULL);

	mgl_gfx.wnd_class_atom = 0;
	mgl_gfx.wnd_handle = 0;
	mgl_gfx.wnd_dc = 0;
	mgl_gfx.wnd_glctx = 0;
	mgl_gfx.mgl_need_exit = false;

	// Register window class
	memset(&wnd_class, 0, sizeof(WNDCLASSW));
	wnd_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wnd_class.lpfnWndProc = mglGfxMainWindowProc;
	wnd_class.hInstance = instance;
	wnd_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wnd_class.lpszClassName = L"MGLWindowClass";

	mgl_gfx.wnd_class_atom = RegisterClassW(&wnd_class);
	if (mgl_gfx.wnd_class_atom == 0)
		return false;

	// Get window style and size
	if (mgl_gfx.win_width <= 0)
		mgl_gfx.win_width = 640;

	if (mgl_gfx.win_height <= 0)
		mgl_gfx.win_height = 480;

	wnd_rect.left = wnd_rect.top = 0;
	wnd_rect.right = mgl_gfx.win_width;
	wnd_rect.bottom = mgl_gfx.win_height;

	ex_style = WS_EX_OVERLAPPEDWINDOW;
	style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	AdjustWindowRectEx(&wnd_rect, style, 0, ex_style);

	// Create window
	mgl_gfx.wnd_handle = CreateWindowExW(
		ex_style,
		(LPCWSTR)(mgl_gfx.wnd_class_atom),
		L"MGL",
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		wnd_rect.right-wnd_rect.left, wnd_rect.bottom-wnd_rect.top,
		NULL,
		NULL,
		instance,
		NULL
	);

	if (mgl_gfx.wnd_handle == NULL) {
		UnregisterClassW(L"MGLWindowClass", instance);

		return false;
	}

	// Choose and set pixel format
	mgl_gfx.wnd_dc = GetDC(mgl_gfx.wnd_handle);
	if(mgl_gfx.wnd_dc == NULL) {
		DestroyWindow(mgl_gfx.wnd_handle);
		UnregisterClassW(L"MGLWindowClass", instance);

		return false;
	}

	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL || PFD_DOUBLEBUFFER || PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 32;

	pixelformat = ChoosePixelFormat(mgl_gfx.wnd_dc, &pfd);
	if(!pixelformat) {
		ReleaseDC(mgl_gfx.wnd_handle, mgl_gfx.wnd_dc);
		DestroyWindow(mgl_gfx.wnd_handle);
		UnregisterClassW(L"MGLWindowClass", instance);

		return false;
	}

	if(!SetPixelFormat(mgl_gfx.wnd_dc, pixelformat, &pfd)) {
		ReleaseDC(mgl_gfx.wnd_handle, mgl_gfx.wnd_dc);
		DestroyWindow(mgl_gfx.wnd_handle);
		UnregisterClassW(L"MGLWindowClass", instance);

		return false;
	}

	mgl_gfx.wnd_glctx = wglCreateContext(mgl_gfx.wnd_dc);
	if(!mgl_gfx.wnd_glctx) {
		ReleaseDC(mgl_gfx.wnd_handle, mgl_gfx.wnd_dc);
		DestroyWindow(mgl_gfx.wnd_handle);
		UnregisterClassW(L"MGLWindowClass", instance);

		return false;
	}
	
	wglMakeCurrent(mgl_gfx.wnd_dc, mgl_gfx.wnd_glctx);

	ShowWindow(mgl_gfx.wnd_handle, SW_SHOW);
	SetForegroundWindow(mgl_gfx.wnd_handle);
	SetFocus(mgl_gfx.wnd_handle);

	glDisable(GL_DEPTH_TEST);

	mgl_gfx.textures = 0;
	mgl_gfx.textures_max = mgl_gfx.tex_int_id = 0;
	mgl_gfx.tex_have = false;

	mglGfxSetOGLScreen();
	mglGfxClearOGLScreen();

	mgl_gfx.mgl_init = true;

	return true;
}

void mglGfxClose(void)
{
	size_t i;
	HINSTANCE instance;
	
	if(mgl_gfx.mgl_init == false)
		return;

	instance = GetModuleHandle(NULL);

	for(i = 0; i < mgl_gfx.textures_max; i++)
		if(mgl_gfx.textures[i].tex_used == true)
			mglGfxDestroyTexture(i + 1);

	if(mgl_gfx.textures_max)
		free(mgl_gfx.textures);

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(mgl_gfx.wnd_glctx);
	ReleaseDC(mgl_gfx.wnd_handle, mgl_gfx.wnd_dc);
	DestroyWindow(mgl_gfx.wnd_handle);
	UnregisterClassW(L"MGLWindowClass", instance);

	mgl_gfx.mgl_init = false;
}

void mglGfxUpdate(void)
{
	MSG msg;
	int i;

	SwapBuffers(mgl_gfx.wnd_dc);

	for(i = 0; i < 256; i++) {
		if(mgl_gfx.win_keys[i] == MGL_GFX_KEY_JUST_PRESSED)
			mgl_gfx.win_keys[i] = MGL_GFX_KEY_PRESSED;
		else if(mgl_gfx.win_keys[i] == MGL_GFX_KEY_RELEASED)
			mgl_gfx.win_keys[i] = MGL_GFX_KEY_UP;
	}

	if(mgl_gfx.mouse_key_l == MGL_GFX_KEY_JUST_PRESSED)
		mgl_gfx.mouse_key_l = MGL_GFX_KEY_PRESSED;
	else if(mgl_gfx.mouse_key_l == MGL_GFX_KEY_RELEASED)
		mgl_gfx.mouse_key_l = MGL_GFX_KEY_UP;

	if(mgl_gfx.mouse_key_r == MGL_GFX_KEY_JUST_PRESSED)
		mgl_gfx.mouse_key_r = MGL_GFX_KEY_PRESSED;
	else if(mgl_gfx.mouse_key_r == MGL_GFX_KEY_RELEASED)
		mgl_gfx.mouse_key_r = MGL_GFX_KEY_UP;

	if(mgl_gfx.mouse_key_m == MGL_GFX_KEY_JUST_PRESSED)
		mgl_gfx.mouse_key_m = MGL_GFX_KEY_PRESSED;
	else if(mgl_gfx.mouse_key_m == MGL_GFX_KEY_RELEASED)
		mgl_gfx.mouse_key_m = MGL_GFX_KEY_UP;

	if(mgl_gfx.mouse_key_4 == MGL_GFX_KEY_JUST_PRESSED)
		mgl_gfx.mouse_key_4 = MGL_GFX_KEY_PRESSED;
	else if(mgl_gfx.mouse_key_4 == MGL_GFX_KEY_RELEASED)
		mgl_gfx.mouse_key_4 = MGL_GFX_KEY_UP;

	if(mgl_gfx.mouse_key_5 == MGL_GFX_KEY_JUST_PRESSED)
		mgl_gfx.mouse_key_5 = MGL_GFX_KEY_PRESSED;
	else if(mgl_gfx.mouse_key_5 == MGL_GFX_KEY_RELEASED)
		mgl_gfx.mouse_key_5 = MGL_GFX_KEY_UP;

	mgl_gfx.mouse_wheel = 0;

	mgl_gfx.mgl_need_exit = false;

	while(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	mglGfxClearOGLScreen();
}

int mglGfxGetParami(int param)
{
	switch (param) {
		case MGL_GFX_PARAMI_INIT:
			return mgl_gfx.mgl_init;
		case MGL_GFX_PARAMI_WIN_WIDTH:
			return mgl_gfx.win_width;
		case MGL_GFX_PARAMI_WIN_HEIGHT:
			return mgl_gfx.win_height;
		case MGL_GFX_PARAMI_NEED_EXIT:
			return mgl_gfx.mgl_need_exit;
		case MGL_GFX_PARAMI_BKG_RED:
			return mgl_gfx.bkg_red;
		case MGL_GFX_PARAMI_BKG_GREEN:
			return mgl_gfx.bkg_green;
		case MGL_GFX_PARAMI_BKG_BLUE:
			return mgl_gfx.bkg_blue;
		case MGL_GFX_PARAMI_MOUSE_X:
			return mgl_gfx.mouse_x;
		case MGL_GFX_PARAMI_MOUSE_Y:
			return mgl_gfx.mouse_y;
		case MGL_GFX_PARAMI_MOUSE_KEY_LEFT:
			return mgl_gfx.mouse_key_l;
		case MGL_GFX_PARAMI_MOUSE_KEY_RIGHT:
			return mgl_gfx.mouse_key_r;
		case MGL_GFX_PARAMI_MOUSE_KEY_MIDDLE:
			return mgl_gfx.mouse_key_m;
		case MGL_GFX_PARAMI_MOUSE_KEY_4:
			return mgl_gfx.mouse_key_4;
		case MGL_GFX_PARAMI_MOUSE_KEY_5:
			return mgl_gfx.mouse_key_5;
		case MGL_GFX_PARAMI_MOUSE_WHEEL:
			return mgl_gfx.mouse_wheel;
		default:
			return 0;
	}
}

bool mglGfxSetParami(int param, int value)
{
	switch (param) {
		case MGL_GFX_PARAMI_INIT:
			return false;
		case MGL_GFX_PARAMI_WIN_WIDTH:
			return mglGfxSetScreen(value, mgl_gfx.win_height, MGL_GFX_WINDOW_MODE_WINDOWED, 0);
		case MGL_GFX_PARAMI_WIN_HEIGHT:
			return mglGfxSetScreen(mgl_gfx.win_width, value, MGL_GFX_WINDOW_MODE_WINDOWED, 0);
		case MGL_GFX_PARAMI_NEED_EXIT:
			mgl_gfx.mgl_need_exit = value;
			return true;
		case MGL_GFX_PARAMI_BKG_RED:
			if(value < 0 || value > 255)
				return false;
			else {
				mgl_gfx.bkg_red = value;

				return true;
			}
		case MGL_GFX_PARAMI_BKG_GREEN:
			if(value < 0 || value > 255)
				return false;
			else {
				mgl_gfx.bkg_green = value;

				return true;
			}
		case MGL_GFX_PARAMI_BKG_BLUE:
			if(value < 0 || value > 255)
				return false;
			else {
				mgl_gfx.bkg_blue = value;

				return true;
			}
		case MGL_GFX_PARAMI_MOUSE_X:
		case MGL_GFX_PARAMI_MOUSE_Y:
		case MGL_GFX_PARAMI_MOUSE_KEY_LEFT:
		case MGL_GFX_PARAMI_MOUSE_KEY_RIGHT:
		case MGL_GFX_PARAMI_MOUSE_KEY_MIDDLE:
		case MGL_GFX_PARAMI_MOUSE_KEY_4:
		case MGL_GFX_PARAMI_MOUSE_KEY_5:
		case MGL_GFX_PARAMI_MOUSE_WHEEL:
		default:
			return false;
	}
}

bool mglGfxSetScreen(int winx, int winy, int mode, int flags)
{
	(void)flags;

	if(mgl_gfx.mgl_init)
		return false;

	if(mode != MGL_GFX_WINDOW_MODE_WINDOWED)
		return false;

	if(winx < 0 || winy < 0)
		return false;

	if(winx == 0 && mgl_gfx.win_width != 0)
		return false;

	if(winy == 0 && mgl_gfx.win_height != 0)
		return false;

	mgl_gfx.win_width = winx;
	mgl_gfx.win_height = winy;

	return true;
}

int mglGfxGetKey(int key)
{
	if(key >= 0 && key < 256)
		return mgl_gfx.win_keys[key];

	return MGL_GFX_KEY_UP;
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

size_t mglGfxCreateTextureFromMemory(unsigned int tex_width, unsigned int tex_height, int tex_format, int tex_filters, void *buffer)
{
	GLuint gl_tex_id;
	size_t tex_id = 0;

	// Аллокатор для текстур
	if(mgl_gfx.textures_max == 0) {
		mgl_gfx.textures_max = 2;
		mgl_gfx.textures = malloc(mgl_gfx.textures_max * sizeof(mgl_gfx_textures_type));
		if(!mgl_gfx.textures)
			return 0;

		memset(mgl_gfx.textures, 0, mgl_gfx.textures_max * sizeof(mgl_gfx_textures_type));
	} else {
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

	// Создание текстуры

	glGetError(); // Сбрасываем флаги ошибки

	glGenTextures(1, &gl_tex_id);
	if(glGetError())
		return 0;

	glBindTexture(GL_TEXTURE_2D, gl_tex_id);

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
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, mglGfxTexGetIntFormat(tex_format), tex_width, tex_height, 0, mglGfxTexGetFormat(tex_format), GL_UNSIGNED_BYTE, buffer);

	mgl_gfx.textures[tex_id].tex_int_id = gl_tex_id;
	mgl_gfx.textures[tex_id].tex_width = tex_width;
	mgl_gfx.textures[tex_id].tex_height = tex_height;
	mgl_gfx.textures[tex_id].tex_used = true;

	if(mgl_gfx.tex_have)
		glBindTexture(GL_TEXTURE_2D, (GLuint)(mgl_gfx.tex_int_id));

	return tex_id + 1;
}

void mglGfxDestroyTexture(size_t tex_id)
{
	GLuint gl_tex_id;

	if(tex_id == 0 || tex_id > mgl_gfx.textures_max)
		return;

	if(mgl_gfx.textures[tex_id - 1].tex_used == false)
		return;

	gl_tex_id = (GLuint)(mgl_gfx.textures[tex_id - 1].tex_int_id);

	glDeleteTextures(1, &gl_tex_id);

	mgl_gfx.textures[tex_id - 1].tex_used = false;
}

bool mglGfxDrawPicture(size_t tex_id, int off_x, int off_y, int toff_x, int toff_y, int size_x, int size_y, float scale_x, float scale_y, int col_r , int col_g, int col_b)
{
	float pic_width, pic_height;
	float tex_start_x, tex_end_x, tex_start_y, tex_end_y;
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

	// Установка текстуры
	if(no_texture == true) {
		if(mgl_gfx.tex_have == true) {
			glDisable(GL_TEXTURE_2D);
			mgl_gfx.tex_have = false;
		}
	} else {
		if(mgl_gfx.tex_have == false) {
			glEnable(GL_TEXTURE_2D);
			mgl_gfx.tex_have = true;
			glBindTexture(GL_TEXTURE_2D, (GLuint)(mgl_gfx.textures[tex_id - 1].tex_int_id));
			mgl_gfx.tex_int_id = mgl_gfx.textures[tex_id - 1].tex_int_id;
		} else if(mgl_gfx.tex_int_id != mgl_gfx.textures[tex_id - 1].tex_int_id) {
			glBindTexture(GL_TEXTURE_2D, (GLuint)(mgl_gfx.textures[tex_id - 1].tex_int_id));
			mgl_gfx.tex_int_id = mgl_gfx.textures[tex_id - 1].tex_int_id;
		}
	}

	if(no_texture == false) {
		tex_start_x = (float)(toff_x) / mgl_gfx.textures[tex_id - 1].tex_width;
		tex_end_x = (float)(toff_x + size_x) / mgl_gfx.textures[tex_id - 1].tex_width;
		tex_start_y = (float)(toff_y) / mgl_gfx.textures[tex_id - 1].tex_height;
		tex_end_y = (float)(toff_y + size_y) / mgl_gfx.textures[tex_id - 1].tex_height;
	}

	// Отрисовка изображения
	glBegin(GL_TRIANGLES);
	if(no_texture == false) glTexCoord2f(tex_start_x, tex_start_y);
	glColor4f(col_r / 255.0f, col_g / 255.0f, col_b / 255.0f, 1.0f);
	glVertex2f((float)off_x, (float)off_y);
	if(no_texture == false) glTexCoord2f(tex_start_x, tex_end_y);
	glColor4f(col_r / 255.0f, col_g / 255.0f, col_b / 255.0f, 1.0f);
	glVertex2f((float)off_x, (float)off_y + pic_height);
	if(no_texture == false) glTexCoord2f(tex_end_x, tex_end_y);
	glColor4f(col_r / 255.0f, col_g / 255.0f, col_b / 255.0f, 1.0f);
	glVertex2f((float)off_x + pic_width, (float)off_y + pic_height);

	if(no_texture == false) glTexCoord2f(tex_start_x, tex_start_y);
	glColor4f(col_r / 255.0f, col_g / 255.0f, col_b / 255.0f, 1.0f);
	glVertex2f((float)off_x, (float)off_y);
	if(no_texture == false) glTexCoord2f(tex_end_x, tex_end_y);
	glColor4f(col_r / 255.0f, col_g / 255.0f, col_b / 255.0f, 1.0f);
	glVertex2f((float)off_x + pic_width, (float)off_y + pic_height);
	if(no_texture == false) glTexCoord2f(tex_end_x, tex_start_y);
	glColor4f(col_r / 255.0f, col_g / 255.0f, col_b / 255.0f, 1.0f);
	glVertex2f((float)off_x + pic_width, (float)off_y);
	glEnd();

	if(off_x <= mgl_gfx.mouse_x && off_x + pic_width > mgl_gfx.mouse_x &&
		off_y <= mgl_gfx.mouse_y && off_y + pic_height > mgl_gfx.mouse_y)
		return true;
	else
		return false;
}

static void mglGfxSetOGLScreen(void)
{
	glViewport(0, 0, mgl_gfx.win_width, mgl_gfx.win_height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluOrtho2D(0, mgl_gfx.win_width, mgl_gfx.win_height, 0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static void mglGfxClearOGLScreen(void)
{
	glClearColor(mgl_gfx.bkg_red / 255.0f, mgl_gfx.bkg_green / 255.0f, mgl_gfx.bkg_blue / 255.0f, 0.0f);

	glClear(GL_COLOR_BUFFER_BIT);
}

static LRESULT CALLBACK mglGfxMainWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
		case WM_LBUTTONDOWN:
			mgl_gfx.mouse_key_l = MGL_GFX_KEY_JUST_PRESSED;

			return 0;
		case WM_LBUTTONUP:
			mgl_gfx.mouse_key_l = MGL_GFX_KEY_RELEASED;

			return 0;
		case WM_RBUTTONDOWN:
			mgl_gfx.mouse_key_r = MGL_GFX_KEY_JUST_PRESSED;

			return 0;
		case WM_RBUTTONUP:
			mgl_gfx.mouse_key_r = MGL_GFX_KEY_RELEASED;

			return 0;
		case WM_MBUTTONDOWN:
			mgl_gfx.mouse_key_m = MGL_GFX_KEY_JUST_PRESSED;

			return 0;
		case WM_MBUTTONUP:
			mgl_gfx.mouse_key_m = MGL_GFX_KEY_RELEASED;

			return 0;
		case WM_XBUTTONDOWN:
			if(GET_XBUTTON_WPARAM(wparam) == XBUTTON1)
				mgl_gfx.mouse_key_4 = MGL_GFX_KEY_JUST_PRESSED;
			else
				mgl_gfx.mouse_key_5 = MGL_GFX_KEY_JUST_PRESSED;

			return 0;
		case WM_XBUTTONUP:
			if(GET_XBUTTON_WPARAM(wparam) == XBUTTON1)
				mgl_gfx.mouse_key_4 = MGL_GFX_KEY_RELEASED;
			else
				mgl_gfx.mouse_key_5 = MGL_GFX_KEY_RELEASED;

			return 0;
		case WM_MOUSEWHEEL:
			mgl_gfx.mouse_wheel = GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;

			return 0;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if((lparam & 0x40000000) == 0) // bit 30
				mgl_gfx.win_keys[wparam] = MGL_GFX_KEY_JUST_PRESSED;

			return 0;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			mgl_gfx.win_keys[wparam] = MGL_GFX_KEY_RELEASED;

			return 0;
		case WM_MOUSEMOVE:
			mgl_gfx.mouse_x = GET_X_LPARAM(lparam);
			mgl_gfx.mouse_y = GET_Y_LPARAM(lparam);

			return 0;
		case WM_CLOSE:
			mgl_gfx.mgl_need_exit = true;
			return 0;
		case WM_SIZE:
			mgl_gfx.win_width = LOWORD(lparam);
			mgl_gfx.win_height = HIWORD(lparam);
			mglGfxSetOGLScreen();
			return 0;
		default:
			return DefWindowProcW(hwnd, msg, wparam, lparam);
	}
}
