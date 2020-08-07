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

#include <Windows.h>

typedef struct {
	int win_width, win_height;
	HWND wnd_handle;
	HDC wnd_dc;
	ATOM wnd_class_atom;
} mgl_gfx_type;

static mgl_gfx_type mgl_gfx;

static LRESULT CALLBACK mglGfxMainWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

bool mglGfxInit(void)
{
	WNDCLASSW wnd_class;
	HINSTANCE instance;
	DWORD style, ex_style;
	RECT wnd_rect;
	
	instance = GetModuleHandle(NULL);

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
	style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

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

	ShowWindow(mgl_gfx.wnd_handle, SW_SHOW);

	return true;
}

void mglGfxUpdate(void)
{
	MSG msg;

	while(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

static LRESULT CALLBACK mglGfxMainWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}
