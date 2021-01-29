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

#include <gl/GLU.h>

typedef struct {
	int win_width, win_height;
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

	mglGfxSetOGLScreen();
	mglGfxClearOGLScreen();

	mgl_gfx.mgl_init = true;

	return true;
}

void mglGfxClose(void)
{
	HINSTANCE instance;
	
	if(mgl_gfx.mgl_init == false)
		return;

	instance = GetModuleHandle(NULL);

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

	SwapBuffers(mgl_gfx.wnd_dc);

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
		default:
			return false;
	}
}

bool mglGfxSetScreen(int winx, int winy, int mode, int flags)
{
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

int mglGetKey(int key)
{
	if(key >= 0 && key < 256)
		return mgl_gfx.win_keys[key];

	return MGL_GFX_KEY_UP;
}

bool mglGfxDrawPicture(size_t tex_id, int off_x, int off_y, int toff_x, int toff_y, int size_x, int size_y, float scale_x, float scale_y, int col_r , int col_g, int col_b)
{
	if(!mgl_gfx.mgl_init)
		return false;

	if(tex_id)
		return false;

	if(size_x <= 0 || size_y <= 0 || scale_x <= 0 || scale_y <= 0)
		return false;

	if(col_r < 0 || col_r > 255 || col_g < 0 || col_g > 255 || col_b < 0 || col_b > 255)
		return false;

	glBegin(GL_TRIANGLES);
	glColor4f(col_r / 255.0f, col_g / 255.0f, col_b / 255.0f, 1.0f);
	glVertex2f((float)off_x, (float)off_y);
	glColor4f(col_r / 255.0f, col_g / 255.0f, col_b / 255.0f, 1.0f);
	glVertex2f((float)off_x, (float)off_y + size_y * scale_y);
	glColor4f(col_r / 255.0f, col_g / 255.0f, col_b / 255.0f, 1.0f);
	glVertex2f((float)off_x + size_x * scale_x, (float)off_y + size_y * scale_y);

	glColor4f(col_r / 255.0f, col_g / 255.0f, col_b / 255.0f, 1.0f);
	glVertex2f((float)off_x, (float)off_y);
	glColor4f(col_r / 255.0f, col_g / 255.0f, col_b / 255.0f, 1.0f);
	glVertex2f((float)off_x + size_x * scale_x, (float)off_y + size_y * scale_y);
	glColor4f(col_r / 255.0f, col_g / 255.0f, col_b / 255.0f, 1.0f);
	glVertex2f((float)off_x + size_x * scale_x, (float)off_y);
	glEnd();

	return true;
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
