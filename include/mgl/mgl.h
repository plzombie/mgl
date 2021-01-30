
#ifndef MGL_H
#define MGL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

enum {
	MGL_GFX_PARAMI_INIT,
	MGL_GFX_PARAMI_WIN_WIDTH,
	MGL_GFX_PARAMI_WIN_HEIGHT,
	MGL_GFX_PARAMI_NEED_EXIT,
	MGL_GFX_PARAMI_BKG_RED,
	MGL_GFX_PARAMI_BKG_GREEN,
	MGL_GFX_PARAMI_BKG_BLUE,
	MGL_GFX_PARAMI_MOUSE_X,
	MGL_GFX_PARAMI_MOUSE_Y,
	MGL_GFX_PARAMI_MOUSE_KEY_LEFT,
	MGL_GFX_PARAMI_MOUSE_KEY_RIGHT,
	MGL_GFX_PARAMI_MOUSE_KEY_MIDDLE,
	MGL_GFX_PARAMI_MOUSE_KEY_4,
	MGL_GFX_PARAMI_MOUSE_KEY_5,
	MGL_GFX_PARAMI_MOUSE_WHEEL
};

enum {
	MGL_GFX_KEY_UP = 0,
	MGL_GFX_KEY_JUST_PRESSED = 1, // 0b01
	MGL_GFX_KEY_PRESSED = 3, // 0b11
	MGL_GFX_KEY_RELEASED = 2 // 0b10
};

enum {
	MGL_GFX_WINDOW_MODE_WINDOWED
};

enum {
	MGL_GFX_TEX_FORMAT_R8G8B8,
	MGL_GFX_TEX_FORMAT_R8G8B8A8,
	MGL_GFX_TEX_FORMAT_GRAYSCALE
};

enum {
	MGL_GFX_TEX_FILTER_REPEAT = 0x1,
	MGL_GFX_TEX_FILTER_LINEAR_MIN = 0x2,
	MGL_GFX_TEX_FILTER_LINEAR_MAG = 0x4
};

extern bool mglGfxInit(void);

extern void mglGfxClose(void);

extern void mglGfxUpdate(void);

extern int mglGfxGetParami(int param);

extern bool mglGfxSetParami(int param, int value);

extern bool mglGfxSetScreen(int winx, int winy, int mode, int flags);

extern int mglGfxGetKey(int key);

extern size_t mglGfxCreateTextureFromMemory(unsigned int tex_width, unsigned int tex_height, int tex_format, int tex_filters, void *buffer);

extern void mglGfxDestroyTexture(size_t tex_id);

extern bool mglGfxDrawPicture(size_t tex_id, int off_x, int off_y, int toff_x, int toff_y, int size_x, int size_y, float scale_x, float scale_y, int col_r, int col_g, int col_b);

#ifdef __cplusplus
}
#endif

#endif
