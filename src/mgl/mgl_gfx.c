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

#define _WIN32_WINNT 0x1000

#include "../../include/mgl/mgl.h"

#include "mgl_gfx_opengl1.h"

#include <Windows.h>
#include <windowsx.h>

#include <gl/GLU.h>

typedef struct {
	unsigned int tex_width, tex_height;
	uintptr_t tex_int_id;
	bool tex_used;
} mgl_gfx_textures_type;

typedef struct {
	mgl_gfx_api_type gfx_api;
	mgl_gfx_textures_type *textures;
	size_t textures_max;
	int win_width, win_height, win_virt_width, win_virt_height;
	int win_dpix, win_dpiy;
	int win_mode;
	int mouse_x, mouse_y, mouse_wheel, mouse_virt_x, mouse_virt_y;
	int mouse_key_l, mouse_key_r, mouse_key_m, mouse_key_4, mouse_key_5;
	int bkg_red, bkg_green, bkg_blue;
	char win_keys[256];
	wchar_t *win_input_chars;
	size_t win_input_chars_max;
	HWND wnd_handle;
	HDC wnd_dc;
	ATOM wnd_class_atom;
	HGLRC wnd_glctx;
	bool mgl_init;
	bool mgl_need_exit;
} mgl_gfx_type;

static mgl_gfx_type mgl_gfx;

static LRESULT CALLBACK mglGfxMainWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static void mglGfxSetWindowSize(DWORD style, DWORD ex_style);
static DWORD mglGfxGetStyle(int mode);
static DWORD mglGfxGetExStyle(int mode);
static UINT mglGfxGetDpiForWindow(HWND hwnd);
static void mglGfxLoadSystemFuncs(void);

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);

#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

typedef BOOL (WINAPI *SetProcessDpiAwarenessContext_type)(DPI_AWARENESS_CONTEXT value);
typedef BOOL (WINAPI *AdjustWindowRectExForDpi_type)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
typedef UINT (WINAPI *GetDpiForWindow_type)(HWND hwnd);
static SetProcessDpiAwarenessContext_type SetProcessDpiAwarenessContext_ptr;
static AdjustWindowRectExForDpi_type AdjustWindowRectExForDpi_ptr;
static GetDpiForWindow_type GetDpiForWindow_ptr;

bool mglGfxInit(void)
{
	WNDCLASSW wnd_class;
	HINSTANCE instance;
	DWORD style, ex_style;

	if(mgl_gfx.mgl_init == true)
		return false;

	mglGfxLoadSystemFuncs();

	if(SetProcessDpiAwarenessContext_ptr)
		SetProcessDpiAwarenessContext_ptr(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	instance = GetModuleHandle(NULL);

	mgl_gfx.gfx_api = mglGfxGetOGL1Api();

	mgl_gfx.win_input_chars_max = 2;
	mgl_gfx.win_input_chars = malloc(mgl_gfx.win_input_chars_max * sizeof(wchar_t));
	if(!mgl_gfx.win_input_chars)
		return false;
	mgl_gfx.win_input_chars[0] = 0;

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
	if (mgl_gfx.win_virt_width <= 0)
		mgl_gfx.win_virt_width = 640;

	if (mgl_gfx.win_virt_height <= 0)
		mgl_gfx.win_virt_height = 480;

	if(mgl_gfx.win_mode <= 0)
		mgl_gfx.win_mode = MGL_GFX_WINDOW_MODE_WINDOWED;

	ex_style = mglGfxGetExStyle(mgl_gfx.win_mode);
	style = mglGfxGetStyle(mgl_gfx.win_mode);

	// Create window
	mgl_gfx.wnd_handle = CreateWindowExW(
		ex_style,
		(LPCWSTR)(mgl_gfx.wnd_class_atom),
		L"MGL",
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		0, 0, //wnd_rect.right-wnd_rect.left, wnd_rect.bottom-wnd_rect.top,
		NULL,
		NULL,
		instance,
		NULL
	);

	if (mgl_gfx.wnd_handle == NULL) {
		UnregisterClassW(L"MGLWindowClass", instance);

		return false;
	}

	mgl_gfx.win_dpix = mgl_gfx.win_dpiy = mglGfxGetDpiForWindow(mgl_gfx.wnd_handle);

	mgl_gfx.win_width = mgl_gfx.win_virt_width * mgl_gfx.win_dpix / 96;
	mgl_gfx.win_height = mgl_gfx.win_virt_height * mgl_gfx.win_dpiy / 96;

	mglGfxSetWindowSize(style, ex_style);

	// Choose and set pixel format
	mgl_gfx.wnd_dc = GetDC(mgl_gfx.wnd_handle);
	if(mgl_gfx.wnd_dc == NULL) {
		DestroyWindow(mgl_gfx.wnd_handle);
		UnregisterClassW(L"MGLWindowClass", instance);

		return false;
	}

	if(!mgl_gfx.gfx_api.InitGfxApi(mgl_gfx.win_width, mgl_gfx.win_height, mgl_gfx.win_width * 96 / mgl_gfx.win_dpix, mgl_gfx.win_height * 96 / mgl_gfx.win_dpiy, mgl_gfx.bkg_red, mgl_gfx.bkg_green, mgl_gfx.bkg_blue, mgl_gfx.wnd_dc)) {
		ReleaseDC(mgl_gfx.wnd_handle, mgl_gfx.wnd_dc);
		DestroyWindow(mgl_gfx.wnd_handle);
		UnregisterClassW(L"MGLWindowClass", instance);

		return false;
	}

	ShowWindow(mgl_gfx.wnd_handle, SW_SHOW);
	SetForegroundWindow(mgl_gfx.wnd_handle);
	SetFocus(mgl_gfx.wnd_handle);

	mgl_gfx.textures = 0;
	mgl_gfx.textures_max = 0;

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

	free(mgl_gfx.win_input_chars);

	mgl_gfx.gfx_api.DestroyGfxApi();
	ReleaseDC(mgl_gfx.wnd_handle, mgl_gfx.wnd_dc);
	DestroyWindow(mgl_gfx.wnd_handle);
	UnregisterClassW(L"MGLWindowClass", instance);

	mgl_gfx.mgl_init = false;
}

void mglGfxUpdate(void)
{
	MSG msg;
	int i;

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

	mgl_gfx.win_input_chars[0] = 0;

	mgl_gfx.gfx_api.SwapBuffers(mgl_gfx.wnd_dc);
	mgl_gfx.gfx_api.ClearScreen(mgl_gfx.bkg_red, mgl_gfx.bkg_green, mgl_gfx.bkg_blue);

	while(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}


}

wchar_t *mglGfxGetParamw(int param)
{
	switch(param) {
		case MGL_GFX_PARAMW_WIN_INPUT_CHARS:
			return mgl_gfx.win_input_chars;
		default:
			return 0;
	}
}

bool mglGfxSetParamw(int param, wchar_t *value)
{
	(void)value;

	switch(param) {
		case MGL_GFX_PARAMW_WIN_INPUT_CHARS:
		default:
			return false;
	}
}

int mglGfxGetParami(int param)
{
	switch(param) {
		case MGL_GFX_PARAMI_INIT:
			return mgl_gfx.mgl_init;
		case MGL_GFX_PARAMI_WIN_WIDTH:
			return mgl_gfx.win_virt_width;
		case MGL_GFX_PARAMI_WIN_HEIGHT:
			return mgl_gfx.win_virt_height;
		case MGL_GFX_PARAMI_NEED_EXIT:
			return mgl_gfx.mgl_need_exit;
		case MGL_GFX_PARAMI_BKG_RED:
			return mgl_gfx.bkg_red;
		case MGL_GFX_PARAMI_BKG_GREEN:
			return mgl_gfx.bkg_green;
		case MGL_GFX_PARAMI_BKG_BLUE:
			return mgl_gfx.bkg_blue;
		case MGL_GFX_PARAMI_MOUSE_X:
			return mgl_gfx.mouse_virt_x;
		case MGL_GFX_PARAMI_MOUSE_Y:
			return mgl_gfx.mouse_virt_y;
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
		case MGL_GFX_PARAMI_WIN_DPIX:
			return mgl_gfx.win_dpix;
		case MGL_GFX_PARAMI_WIN_DPIY:
			return mgl_gfx.win_dpiy;
		case MGL_GFX_PARAMI_WIN_MODE:
			return mgl_gfx.win_mode;
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
			return mglGfxSetScreen(value, mgl_gfx.win_virt_height, MGL_GFX_WINDOW_MODE_WINDOWED, 0);
		case MGL_GFX_PARAMI_WIN_HEIGHT:
			return mglGfxSetScreen(mgl_gfx.win_virt_width, value, MGL_GFX_WINDOW_MODE_WINDOWED, 0);
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
		case MGL_GFX_PARAMI_WIN_DPIX:
		case MGL_GFX_PARAMI_WIN_DPIY:
			return false;
		case MGL_GFX_PARAMI_WIN_MODE:
			return mglGfxSetScreen(mgl_gfx.win_virt_width, mgl_gfx.win_virt_height, value, 0);
		default:
			return false;
	}
}

bool mglGfxSetScreen(int winx, int winy, int mode, int flags)
{
	(void)flags;

	if(mode != MGL_GFX_WINDOW_MODE_WINDOWED)
		return false;

	if(winx < 0 || winy < 0)
		return false;

	if(winx == 0 && mgl_gfx.win_width != 0)
		return false;

	if(winy == 0 && mgl_gfx.win_height != 0)
		return false;

	mgl_gfx.win_mode = mode;
	mgl_gfx.win_virt_width = winx;
	mgl_gfx.win_virt_height = winy;

	mgl_gfx.win_width = mgl_gfx.win_virt_width * mgl_gfx.win_dpix / 96;
	mgl_gfx.win_height = mgl_gfx.win_virt_height * mgl_gfx.win_dpiy / 96;

	if(mgl_gfx.mgl_init)
		mglGfxSetWindowSize(mglGfxGetStyle(mgl_gfx.win_mode), mglGfxGetExStyle(mgl_gfx.win_mode));

	return true;
}

int mglGfxGetKey(int key)
{
	if(key >= 0 && key < 256)
		return mgl_gfx.win_keys[key];

	return MGL_GFX_KEY_UP;
}

size_t mglGfxCreateTextureFromMemory(unsigned int tex_width, unsigned int tex_height, int tex_format, int tex_filters, void *buffer)
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

	if(mgl_gfx.gfx_api.CreateTexture(tex_width, tex_height, tex_format, tex_filters, buffer, &tex_int_id) == false)
		return 0;

	mgl_gfx.textures[tex_id].tex_int_id = tex_int_id;
	mgl_gfx.textures[tex_id].tex_width = tex_width;
	mgl_gfx.textures[tex_id].tex_height = tex_height;
	mgl_gfx.textures[tex_id].tex_used = true;

	return tex_id + 1;
}

void mglGfxDestroyTexture(size_t tex_id)
{
	if(tex_id == 0 || tex_id > mgl_gfx.textures_max)
		return;

	if(mgl_gfx.textures[tex_id - 1].tex_used == false)
		return;

	mgl_gfx.gfx_api.DestroyTexture(mgl_gfx.textures[tex_id - 1].tex_int_id);

	mgl_gfx.textures[tex_id - 1].tex_used = false;
}

bool mglGfxDrawPicture(size_t tex_id, int off_x, int off_y, int toff_x, int toff_y, int size_x, int size_y, float scale_x, float scale_y, int col_r , int col_g, int col_b)
{
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
	}

	// Отрисовка изображения
	mgl_gfx.gfx_api.DrawTriangle(no_texture, no_texture?0:mgl_gfx.textures[tex_id - 1].tex_int_id,
		(float)off_x, (float)off_y, tex_start_x, tex_start_y, col_r / 255.0f, col_g / 255.0f, col_b / 255.0f,
		(float)off_x, (float)off_y + pic_height, tex_start_x, tex_end_y, col_r / 255.0f, col_g / 255.0f, col_b / 255.0f,
		(float)off_x + pic_width, (float)off_y + pic_height, tex_end_x, tex_end_y, col_r / 255.0f, col_g / 255.0f, col_b / 255.0f);
	mgl_gfx.gfx_api.DrawTriangle(no_texture, no_texture?0:mgl_gfx.textures[tex_id - 1].tex_int_id,
		(float)off_x, (float)off_y, tex_start_x, tex_start_y, col_r / 255.0f, col_g / 255.0f, col_b / 255.0f,
		(float)off_x + pic_width, (float)off_y + pic_height, tex_end_x, tex_end_y, col_r / 255.0f, col_g / 255.0f, col_b / 255.0f,
		(float)off_x + pic_width, (float)off_y, tex_end_x, tex_start_y, col_r / 255.0f, col_g / 255.0f, col_b / 255.0f);

	if(off_x <= mgl_gfx.mouse_virt_x && off_x + pic_width > mgl_gfx.mouse_virt_x &&
		off_y <= mgl_gfx.mouse_virt_y && off_y + pic_height > mgl_gfx.mouse_virt_y)
		return true;
	else
		return false;
}

static LRESULT CALLBACK mglGfxMainWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	RECT *rect;

	switch (msg) {
		case WM_CHAR:
		{
			size_t i;

			for(i = 0; i < mgl_gfx.win_input_chars_max-1; i++) {
				if(mgl_gfx.win_input_chars[i] == 0)
					break;
			}
			if(i == mgl_gfx.win_input_chars_max - 1) {
				wchar_t *_input_chars;
				size_t _input_chars_max;

				_input_chars_max = mgl_gfx.win_input_chars_max * 2;
				_input_chars = realloc(mgl_gfx.win_input_chars, _input_chars_max * sizeof(wchar_t));
				if(!_input_chars)
					return 0;

				mgl_gfx.win_input_chars = _input_chars;
				mgl_gfx.win_input_chars_max = _input_chars_max;
			}

			mgl_gfx.win_input_chars[i] = (wchar_t)wparam;
			mgl_gfx.win_input_chars[i + 1] = 0;

			return 0;
		}
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
			mgl_gfx.mouse_virt_x = mgl_gfx.mouse_x * 96 / mgl_gfx.win_dpix;
			mgl_gfx.mouse_virt_y = mgl_gfx.mouse_y * 96 / mgl_gfx.win_dpiy;

			return 0;
		case WM_CLOSE:
			mgl_gfx.mgl_need_exit = true;
			return 0;
		case WM_SIZE:
			mgl_gfx.win_width = LOWORD(lparam);
			mgl_gfx.win_height = HIWORD(lparam);
			mgl_gfx.win_virt_width = mgl_gfx.win_width * 96 / mgl_gfx.win_dpix;
			mgl_gfx.win_virt_height = mgl_gfx.win_height * 96 / mgl_gfx.win_dpiy;
			mgl_gfx.gfx_api.SetScreen(mgl_gfx.win_width, mgl_gfx.win_height, mgl_gfx.win_virt_width, mgl_gfx.win_virt_height);
			return 0;
		case WM_DPICHANGED:
			mgl_gfx.win_dpix = LOWORD(wparam);
			mgl_gfx.win_dpiy = HIWORD(wparam);
			mgl_gfx.mouse_virt_x = mgl_gfx.mouse_x * 96 / mgl_gfx.win_dpix;
			mgl_gfx.mouse_virt_y = mgl_gfx.mouse_y * 96 / mgl_gfx.win_dpiy;

			rect = (RECT *)lparam;
			SetWindowPos(mgl_gfx.wnd_handle, NULL, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);

			return 0;
		default:
			return DefWindowProcW(hwnd, msg, wparam, lparam);
	}
}

static void mglGfxSetWindowSize(DWORD style, DWORD ex_style)
{
	RECT wnd_rect;

	memset(&wnd_rect, 0, sizeof(RECT));
	wnd_rect.left = wnd_rect.top = 0;
	wnd_rect.right = mgl_gfx.win_width;
	wnd_rect.bottom = mgl_gfx.win_height;
	if(AdjustWindowRectExForDpi_ptr)
		AdjustWindowRectExForDpi_ptr(&wnd_rect, style, 0, ex_style, mgl_gfx.win_dpix);
	else
		AdjustWindowRectEx(&wnd_rect, style, 0, ex_style);

	SetWindowPos(mgl_gfx.wnd_handle, NULL, 0, 0, wnd_rect.right - wnd_rect.left, wnd_rect.bottom - wnd_rect.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
}

static DWORD mglGfxGetStyle(int mode)
{
	switch(mode) {
		case MGL_GFX_WINDOW_MODE_WINDOWED:
		default:
			return WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}
}

static DWORD mglGfxGetExStyle(int mode)
{
	switch(mode) {
		case MGL_GFX_WINDOW_MODE_WINDOWED:
		default:
			return WS_EX_OVERLAPPEDWINDOW;
	}
}

static UINT mglGfxGetDpiForWindow(HWND hwnd)
{
	if(GetDpiForWindow_ptr)
		return GetDpiForWindow_ptr(hwnd);
	else
		return 96;
}

static void mglGfxLoadSystemFuncs(void)
{
	HANDLE /*shcore, */user32;

	user32 = GetModuleHandleW(L"User32.dll");
	if(user32) {
		SetProcessDpiAwarenessContext_ptr = (SetProcessDpiAwarenessContext_type)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
		AdjustWindowRectExForDpi_ptr = (AdjustWindowRectExForDpi_type)GetProcAddress(user32, "AdjustWindowRectExForDpi");
		GetDpiForWindow_ptr = (GetDpiForWindow_type)GetProcAddress(user32, "GetDpiForWindow");
	}

	/*shcore = GetModuleHandleW(L"Shcore.dll");
	if(!shcore) shcore = LoadLibraryW(L"Shcore.dll");
	if(shcore) {
		//	SetProcessDpiAwarenessContext_ptr = (SetProcessDpiAwarenessContext_type)GetProcAddress(shcore, "SetProcessDpiAwarenessContext");
	}*/
}
