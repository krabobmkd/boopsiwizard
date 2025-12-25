#ifndef _CLASS_BASENAMEPRIVATE_H_
#define _CLASS_BASENAMEPRIVATE_H_

#include "compilers.h"
#include "class_basename.h"

// not much sense because c++ static runtime are hard to link.
#ifdef __cplusplus
extern "C" {
#endif

#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <graphics/gfx.h>
#include <graphics/regions.h>

// enable or not some parts of code...
#define USE_REGION_CLIPPING 1
#define USE_BEVEL_FRAME 1

/**
*  this is the internal private gadget struct that own the data of the object instances.
* an important principle of boopsi is that structure for the class is hidden to the consumers.
* the consumers will only see the public header, and will do setAtribs()/GetAttribs()/Domethod().
* Also: for the same Gadget, superclass members are in struct Gadget * passed to functions.
* (These are just concatenated structs in a system private way.)
* DEVTODO: make this class evolve to retain the data needed to draw and interact with your gadget.
*/
typedef struct IBaseName {
    // let's say we have coordinates of the center of the circle
    UWORD _circleCenterX,_circleCenterY;

    // DEVTODO: we could manage the mouse interaction current state....
    ULONG _MouseMode;
    ULONG _EditMode;

    // would have minimal size here.
    UWORD _minimalWidth,_minimalHeight;


    struct Rectangle _framerec;
#ifdef USE_REGION_CLIPPING
    struct Region *_clipRegion;
#endif
#ifdef USE_BEVEL_FRAME
    struct Image *Bevel;
#endif

} BaseName;

ULONG BaseName_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set);
ULONG BaseName_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get);
ULONG BaseName_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout);
ULONG BaseName_Render(Class *C, struct Gadget *Gad, struct gpRender *Render, ULONG update);
ULONG BaseName_HandleInput(Class *C, struct Gadget *Gad, struct gpInput *Input);
ULONG BaseName_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D);

// - - - - -- -


/**
* This is to publish ou data when they change.
* It may be better to just notify what change and have many notify functions per theme.
* Some examples use only one Notify which send all attribs.
*/
ULONG BaseName_NotifyCoords(Class *C, struct Gadget *Gad, struct GadgetInfo	*GInfo);

#ifdef __cplusplus
}
#endif

#endif
