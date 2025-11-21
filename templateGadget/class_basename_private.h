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

/** for dispatcher, very wise use of union.
 *  each  struct also starts with MethodID.
 * and they are the very parameters for each methods.
 */
typedef union MsgUnion
{
  ULONG  MethodID;
  // from classusr.h or gadgetclass.h, all starts with MethodID.
  struct opSet        opSet;
  struct opUpdate     opUpdate;
  struct opGet        opGet;
  struct gpHitTest    gpHitTest;
  struct gpRender     gpRender;
  struct gpInput      gpInput;
  struct gpGoInactive gpGoInactive;
  struct gpLayout     gpLayout;
} *Msgs;

/**
* This is to publish ou data when they change.
* It may be better to just notify what change and have many notify functions per theme.
* Some examples use only one Notify which send all attribs.
*/
ULONG BaseName_NotifyCoords(Class *C, struct Gadget *Gad, struct GadgetInfo	*GInfo);

/** this is the struct that is the extended struct Library
 * That is created with OpenLibrary().
 * But as it just manages a BOOPSI class there are just the open/close functions.
 * which themselves only manages registering the class with MakeClass()/AddClass()
 * versioning, and closing itself. This is *not* the boopsi class definition which is up there.
 * So it doesnt have to evolve, and can keep same name for each projects.
 * That said, layout.gadget has tool methods like any library.
 * must be mirrored to equivalent in classinit.s
 */
struct ExtClassLib
{
    struct ClassLibrary cb_ClassLibrary;

    APTR  cb_SysBase; // this is passed as LibInit
    APTR  cb_SegList; // this is passed at OpenLib and needed at expunge.
    // note: old libraries examples adds bases for graphics/intuition/utility after this
    // but C compiler will only search then in globals...
};

#ifdef __cplusplus
}
#endif

#endif
