
#ifndef MGL_H
#define MGL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

enum {
	MGL_GFX_PARAMI_INIT,
	MGL_GFX_PARAMI_WIN_WIDTH,
	MGL_GFX_PARAMI_WIN_HEIGHT,
	MGL_GFX_PARAMI_NEED_EXIT,
	MGL_GFX_PARAMI_BKG_RED,
	MGL_GFX_PARAMI_BKG_GREEN,
	MGL_GFX_PARAMI_BKG_BLUE
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

extern bool mglGfxInit(void);

extern void mglGfxClose(void);

extern void mglGfxUpdate(void);

extern int mglGfxGetParami(int param);

extern bool mglGfxSetParami(int param, int value);

extern bool mglGfxSetScreen(int winx, int winy, int mode, int flags);

extern int mglGetKey(int key);

extern bool mglGfxDrawPicture(size_t tex_id, int off_x, int off_y, int toff_x, int toff_y, int size_x, int size_y, float scale_x, float scale_y, int col_r, int col_g, int col_b);

#ifdef __cplusplus
}
#endif

#endif