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
#define WINVER 0x1000

#include "../../include/mgl/mgl.h"

#include "mgl_gfx.h"
#include "mgl_gfx_opengl1.h"

#include <Windows.h>
#include <windowsx.h>

#include <gl/GLU.h>

mgl_gfx_type mgl_gfx;

static LRESULT CALLBACK mglGfxMainWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static void mglGfxSetWindowSize(DWORD style, DWORD ex_style, bool mode_changed);
static DWORD mglGfxGetStyle(int mode);
static DWORD mglGfxGetExStyle(int mode);
static void mglGfxGetDpiForWindow(void);
static void mglGfxLoadSystemFuncs(void);

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);

#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
#ifndef DPI_ENUMS_DECLARED
typedef enum PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE = 0,
	PROCESS_SYSTEM_DPI_AWARE,
	PROCESS_PER_MONITOR_DPI_AWARE
} PROCESS_DPI_AWARENESS;

typedef enum MONITOR_DPI_TYPE {
	MDT_EFFECTIVE_DPI = 0,
	MDT_ANGULAR_DPI,
	MDT_RAW_DPI,
	DT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;
#define DPI_ENUMS_DECLARED
#endif

typedef BOOL (WINAPI *SetProcessDpiAwarenessContext_type)(DPI_AWARENESS_CONTEXT value);
typedef BOOL (WINAPI *AdjustWindowRectExForDpi_type)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
typedef UINT (WINAPI *GetDpiForWindow_type)(HWND hwnd);
typedef BOOL (WINAPI *SetProcessDPIAware_type)(void);
typedef BOOL (WINAPI *EnableNonClientDpiScaling_type)(HWND hwnd);
typedef HRESULT (WINAPI *SetProcessDpiAwareness_type)(PROCESS_DPI_AWARENESS value);
typedef HRESULT (WINAPI *GetDpiForMonitor_type)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);
static SetProcessDpiAwarenessContext_type SetProcessDpiAwarenessContext_ptr;
static AdjustWindowRectExForDpi_type AdjustWindowRectExForDpi_ptr;
static GetDpiForWindow_type GetDpiForWindow_ptr;
static SetProcessDPIAware_type SetProcessDPIAware_ptr;
static EnableNonClientDpiScaling_type EnableNonClientDpiScaling_ptr;
static SetProcessDpiAwareness_type SetProcessDpiAwareness_ptr;
static GetDpiForMonitor_type GetDpiForMonitor_ptr;

MGL_API bool MGL_APIENTRY mglGfxInit(void)
{
	WNDCLASSW wnd_class;
	HINSTANCE instance;
	DWORD style, ex_style;
	int winx, winy;

	if(mgl_gfx.mgl_init == true)
		return false;

	mglGfxLoadSystemFuncs();

	if(SetProcessDpiAwarenessContext_ptr)
		SetProcessDpiAwarenessContext_ptr(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	else if(SetProcessDpiAwareness_ptr)
		SetProcessDpiAwareness_ptr(PROCESS_PER_MONITOR_DPI_AWARE);
	else if(SetProcessDPIAware_ptr)
		SetProcessDPIAware_ptr();

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

	mgl_gfx.win_dpix = mgl_gfx.win_dpiy = 96;

	ex_style = mglGfxGetExStyle(mgl_gfx.win_mode);
	style = mglGfxGetStyle(mgl_gfx.win_mode);

	// Save window size because CreateWindowExW can execute window function
	winx = mgl_gfx.win_virt_width;
	winy = mgl_gfx.win_virt_height;

	// Create window
	mgl_gfx.wnd_handle = CreateWindowExW(
		ex_style,
		(LPCWSTR)(mgl_gfx.wnd_class_atom),
		L"MGL",
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		0, 0,
		NULL,
		NULL,
		instance,
		NULL
	);

	if (mgl_gfx.wnd_handle == NULL) {
		UnregisterClassW(L"MGLWindowClass", instance);

		return false;
	}

	mgl_gfx.wnd_dc = GetDC(mgl_gfx.wnd_handle);
	if(mgl_gfx.wnd_dc == NULL) {
		DestroyWindow(mgl_gfx.wnd_handle);
		UnregisterClassW(L"MGLWindowClass", instance);

		return false;
	}

	// Restore window size
	mgl_gfx.win_virt_width = winx;
	mgl_gfx.win_virt_height = winy;

	mglGfxGetDpiForWindow();

	if(mgl_gfx.win_mode == MGL_GFX_WINDOW_MODE_FULLSCREEN) {
		ShowWindow(mgl_gfx.wnd_handle, SW_MAXIMIZE);

		mgl_gfx.win_virt_width = mgl_gfx.win_width * 96 / mgl_gfx.win_dpix;
		mgl_gfx.win_virt_height = mgl_gfx.win_height * 96 / mgl_gfx.win_dpiy;
	} else {
		mgl_gfx.win_width = mgl_gfx.win_virt_width * mgl_gfx.win_dpix / 96;
		mgl_gfx.win_height = mgl_gfx.win_virt_height * mgl_gfx.win_dpiy / 96;

		mglGfxSetWindowSize(style, ex_style, false);
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

	mgl_gfx.gfx_info = mgl_gfx.gfx_api.GetInfo();

	mgl_gfx.mgl_init = true;

	return true;
}

MGL_API void MGL_APIENTRY mglGfxClose(void)
{
	size_t i;
	HINSTANCE instance;

	if(mgl_gfx.mgl_init == false)
		return;

	mgl_gfx.gfx_api.DestroyInfo(mgl_gfx.gfx_info);

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

MGL_API void MGL_APIENTRY mglGfxUpdate(void)
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

MGL_API wchar_t * MGL_APIENTRY mglGfxGetParamw(int param)
{
	switch(param) {
		case MGL_GFX_PARAMW_WIN_INPUT_CHARS:
			return mgl_gfx.win_input_chars;
		case MGL_GFX_PARAMW_INFO:
			return mgl_gfx.gfx_info;
		default:
			return 0;
	}
}

MGL_API bool MGL_APIENTRY mglGfxSetParamw(int param, wchar_t *value)
{
	(void)value;

	switch(param) {
		case MGL_GFX_PARAMW_WIN_INPUT_CHARS:
		default:
			return false;
	}
}

MGL_API int MGL_APIENTRY mglGfxGetParami(int param)
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

MGL_API bool MGL_APIENTRY mglGfxSetParami(int param, int value)
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

MGL_API bool MGL_APIENTRY mglGfxSetScreen(int winx, int winy, int mode, int flags)
{
	bool mode_changed = false;

	(void)flags;

	switch(mode) {
		case MGL_GFX_WINDOW_MODE_WINDOWED:
		case MGL_GFX_WINDOW_MODE_WINDOWED_FIXED:
		case MGL_GFX_WINDOW_MODE_FULLSCREEN:
			break;
		default:
			return false;
	}

	if(winx < 0 || winy < 0)
		return false;

	if(mode == mgl_gfx.win_mode && mode == MGL_GFX_WINDOW_MODE_FULLSCREEN && (winx != mgl_gfx.win_virt_width || winy != mgl_gfx.win_virt_height))
		return false;

	if(winx == 0 && mgl_gfx.mgl_init == true)
		return false;

	if(winy == 0 && mgl_gfx.mgl_init == true)
		return false;

	if(mode != mgl_gfx.win_mode)
		mode_changed = true;

	mgl_gfx.win_mode = mode;
	mgl_gfx.win_virt_width = winx;
	mgl_gfx.win_virt_height = winy;

	mgl_gfx.win_width = mgl_gfx.win_virt_width * mgl_gfx.win_dpix / 96;
	mgl_gfx.win_height = mgl_gfx.win_virt_height * mgl_gfx.win_dpiy / 96;

	if(mgl_gfx.mgl_init) {
		mglGfxSetWindowSize(mglGfxGetStyle(mgl_gfx.win_mode), mglGfxGetExStyle(mgl_gfx.win_mode), mode_changed);

		if(mode == MGL_GFX_WINDOW_MODE_FULLSCREEN && mode_changed)
			ShowWindow(mgl_gfx.wnd_handle, SW_MAXIMIZE);
	}

	return true;
}

MGL_API int MGL_APIENTRY mglGfxGetKey(int key)
{
	if(key >= 0 && key < 256)
		return mgl_gfx.win_keys[key];

	return MGL_GFX_KEY_UP;
}

static LRESULT CALLBACK mglGfxMainWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	RECT *rect;

	switch (msg) {
		case WM_NCCREATE:
			if(!SetProcessDpiAwarenessContext_ptr)
				if(EnableNonClientDpiScaling_ptr)
					EnableNonClientDpiScaling_ptr(hwnd);

			return DefWindowProcW(hwnd, msg, wparam, lparam);
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

static void mglGfxSetWindowSize(DWORD style, DWORD ex_style, bool mode_changed)
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

	if(mode_changed) {
		SetWindowLongW(mgl_gfx.wnd_handle, GWL_STYLE, style);
		SetWindowLongW(mgl_gfx.wnd_handle, GWL_EXSTYLE, ex_style);
	}

	SetWindowPos(mgl_gfx.wnd_handle, NULL, 0, 0, wnd_rect.right - wnd_rect.left, wnd_rect.bottom - wnd_rect.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | (mode_changed?(SWP_FRAMECHANGED | SWP_SHOWWINDOW):0));
}

static DWORD mglGfxGetStyle(int mode)
{
	switch(mode) {
		case MGL_GFX_WINDOW_MODE_WINDOWED_FIXED:
			return WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		case MGL_GFX_WINDOW_MODE_FULLSCREEN:
			return WS_POPUP;
		case MGL_GFX_WINDOW_MODE_WINDOWED:
		default:
			return WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}
}

static DWORD mglGfxGetExStyle(int mode)
{
	switch(mode) {
		case MGL_GFX_WINDOW_MODE_FULLSCREEN:
			return 0;
		case MGL_GFX_WINDOW_MODE_WINDOWED_FIXED:
		case MGL_GFX_WINDOW_MODE_WINDOWED:
		default:
			return WS_EX_OVERLAPPEDWINDOW;
	}
}

static void mglGfxGetDpiForWindow(void)
{
	if(GetDpiForWindow_ptr)
		mgl_gfx.win_dpix = mgl_gfx.win_dpiy = GetDpiForWindow_ptr(mgl_gfx.wnd_handle);
	else if(GetDpiForMonitor_ptr) {
		HMONITOR hmonitor;
		UINT dpix, dpiy;

		hmonitor = MonitorFromWindow(mgl_gfx.wnd_handle, MONITOR_DEFAULTTONEAREST);

		if(GetDpiForMonitor_ptr(hmonitor, MDT_EFFECTIVE_DPI, &dpix, &dpiy) != S_OK) {
			mgl_gfx.win_dpix = GetDeviceCaps(mgl_gfx.wnd_dc, LOGPIXELSX);
			mgl_gfx.win_dpiy = GetDeviceCaps(mgl_gfx.wnd_dc, LOGPIXELSY);
		} else {
			mgl_gfx.win_dpix = dpix;
			mgl_gfx.win_dpiy = dpiy;
		}
	} else {
		mgl_gfx.win_dpix = GetDeviceCaps(mgl_gfx.wnd_dc, LOGPIXELSX);
		mgl_gfx.win_dpiy = GetDeviceCaps(mgl_gfx.wnd_dc, LOGPIXELSY);
	}
}

static void mglGfxLoadSystemFuncs(void)
{
	HANDLE shcore, user32;

	user32 = GetModuleHandleW(L"User32.dll");
	if(user32) {
		SetProcessDpiAwarenessContext_ptr = (SetProcessDpiAwarenessContext_type)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
		AdjustWindowRectExForDpi_ptr = (AdjustWindowRectExForDpi_type)GetProcAddress(user32, "AdjustWindowRectExForDpi");
		GetDpiForWindow_ptr = (GetDpiForWindow_type)GetProcAddress(user32, "GetDpiForWindow");
		SetProcessDPIAware_ptr = (SetProcessDPIAware_type)GetProcAddress(user32, "SetProcessDPIAware");
		EnableNonClientDpiScaling_ptr = (EnableNonClientDpiScaling_type)GetProcAddress(user32, "EnableNonClientDpiScaling");
	}

	shcore = GetModuleHandleW(L"Shcore.dll");
	if(!shcore) shcore = LoadLibraryW(L"Shcore.dll");
	if(shcore) {
		SetProcessDpiAwareness_ptr = (SetProcessDpiAwareness_type)GetProcAddress(shcore, "SetProcessDpiAwareness");
		GetDpiForMonitor_ptr = (GetDpiForMonitor_type)GetProcAddress(shcore, "GetDpiForMonitor");
	}
}
