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
	ATOM wnd_class_atom;
} mgl_gfx_type;

mgl_gfx_type mgl_gfx;

LRESULT CALLBACK mglGfxMainWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

bool mglGfxInit(void)
{
	WNDCLASSW wnd_class;
	HINSTANCE instance;
	
	instance = NULL;

	// Register window class
	memset(&wnd_class, 0, sizeof(WNDCLASSW));
	wnd_class.style = CS_HREDRAW | CS_VREDRAW;
	wnd_class.lpfnWndProc = mglGfxMainWindowProc;
	wnd_class.hInstance = instance;
	wnd_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wnd_class.lpszClassName = L"MGLWindowClass";

	mgl_gfx.wnd_class_atom = RegisterClassW(&wnd_class);
	if (mgl_gfx.wnd_class_atom == 0)
		return false;

	// Create window
	if (mgl_gfx.win_width <= 0)
		mgl_gfx.win_width = 640;

	if (mgl_gfx.win_height <= 0)
		mgl_gfx.win_height = 480;

	mgl_gfx.wnd_handle = CreateWindowExW(
		WS_EX_OVERLAPPEDWINDOW,
		(LPCWSTR)(mgl_gfx.wnd_class_atom),
		L"MGL",
		WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		mgl_gfx.win_width, mgl_gfx.win_height,
		NULL,
		NULL,
		instance,
		NULL
	);

	if (mgl_gfx.wnd_handle == NULL) {
		UnregisterClassW(L"MGLWindowClass", instance);

		return false;
	}

	ShowWindow(mgl_gfx.wnd_handle, SW_SHOW);

	return true;
}

void mglGfxUpdate(void)
{
	MSG msg;

	while(PeekMessageW(&msg, mgl_gfx.wnd_handle, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}
