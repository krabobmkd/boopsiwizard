#ifndef _LOADDTTOBM_H_
#define _LOADDTTOBM_H_

#include <exec/types.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>

typedef struct {
    void *obj ;
    struct BitMap *bm;
    WORD   width; // height is bm->rows
    WORD   height;
    WORD    nbColors,d;
} DtBm;

/** this version works for 24bit target or 8bit target with remap, may return transparent bnitmap */
int LoadDataTypeToBm(const char *pFileNameOrMem, int ifRamRamSize,
                        DtBm *DtBm,PLANEPTR *pmaskPlane, struct Screen *pDestScreen);
void closeDataTypeBm(DtBm *DtBm);
#endif


