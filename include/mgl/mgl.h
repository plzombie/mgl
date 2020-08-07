
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
	MGL_GFX_PARAM_NEED_EXIT
};

extern bool mglGfxInit(void);

extern void mglGfxClose(void);

extern void mglGfxUpdate(void);

extern int mglGfxGetParam(int param);

extern bool mglGfxSetParam(int param, int value);

#ifdef __cplusplus
}
#endif

#endif