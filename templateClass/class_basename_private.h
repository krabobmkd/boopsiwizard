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

/**
*  this is the internal private BOOPSI Class struct that own the data of the object instances.
* an important principle of boopsi is that structure for the class is hidden to the consumers.
* the consumers will only see the public header, and will do setAtribs()/GetAttribs()/Domethod().
* Also: for the same BOOPSI Class, superclass members are in struct BOOPSI Class * passed to functions.
* (These are just concatenated structs in a system private way.)
* DEVTODO: add member to class, manage them in all dispatcher functions.
*/
typedef struct IBaseName {
    // this class manage this member:
    int _value;

} BaseName;

ULONG BaseName_SetAttrs(Class *C, Object *Gad, struct opSet *Set);
ULONG BaseName_GetAttr(Class *C, Object *Gad, struct opGet *Get);

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
} *Msgs;

/**
* This is to publish our data when they change.
* It may be better to just notify what change and have many notify functions per theme.
* Some examples use only one Notify which send all attribs.
*/
ULONG BaseName_NotifyValueChange(Class *C, Object *Obj);

/** this is the struct that is the extended struct Library
 * That is created with OpenLibrary().
 * But as it just manages a BOOPSI class there are just the open/close functions.
 * which themselves only manages registering the class with MakeClass()/AddClass()
 * versioning, and closing itself. This is *not* the boopsi class definition which is up there.
 * So it doesnt have to evolve, and can keep same name for each projects.
 * That said, some class and gadgets like layout.gadget has tool methods like any library.
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
