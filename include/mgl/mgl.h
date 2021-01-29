
#ifndef MGL_H
#define MGL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

enum {
	MGL_GFX_PARAM_INIT,
	MGL_GFX_PARAM_WIN_WIDTH,
	MGL_GFX_PARAM_WIN_HEIGHT,
	MGL_GFX_PARAM_NEED_EXIT,
	MGL_GFX_PARAM_BKG_RED,
	MGL_GFX_PARAM_BKG_GREEN,
	MGL_GFX_PARAM_BKG_BLUE
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

#ifdef __cplusplus
}
#endif

#endif