#ifndef _BOOPSIINLINES_H_
#define _BOOPSIINLINES_H_

#ifdef __cplusplus
extern "C" {
#endif

//#include <proto/intuition.h>
#include <proto/alib.h>
#include <intuition/classes.h>

//struct Window;

#ifdef __SASC
#define AINLINE static __inline
#else
#define AINLINE static inline
#endif

AINLINE struct Window *boopsi_OpenWindow(Object *owin) {
    return  (struct Window *)DoMethod(owin, WM_OPEN, NULL);
}


#ifdef __cplusplus
}
#endif

#endif
