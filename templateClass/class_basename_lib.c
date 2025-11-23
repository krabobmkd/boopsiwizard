/**
 * This file contains lib inits that create the class
 * and would OpenLibrary() for other dependencies.
 * word XXX_STATICLINK decides if this is used as the header for a shared .class file
 * or if it is statically linked.
 */

#include <exec/alerts.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/layers.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>

#include "class_basename_private.h"

#ifdef USE_BEVEL_FRAME
    #include <proto/bevel.h>
#endif

typedef ULONG (*REHOOKFUNC)();

#ifdef BASENAME_STATICLINK
#include <stdlib.h>
#include <string.h>
#endif

#ifndef BASENAME_STATICLINK
struct ExecBase       *SysBase=NULL;
struct GfxBase        *GfxBase=NULL;
struct IntuitionBase  *IntuitionBase=NULL;
struct Library        *LayersBase=NULL;
struct DosLibrary     *DOSBase=NULL;
struct Library        *UtilityBase=NULL;
    #if defined(__GNUC__) && (__GNUC__ < 3)
        struct Library        *__UtilityBase=NULL; // amiga gcc2.95 with noixemul and 68000, and our gadget startup needs that.
    #endif
#endif

#ifdef USE_BEVEL_FRAME
struct Library        *BevelBase=NULL;
#endif
// this is the only global writtable we should see in the whole class binary !
struct IClass   *BaseNameClassPtr=NULL;
// this 2 strings are also linked to the asm startup header ( in .gadget mode)
// note: (const char *str="") would make str be a (char **) to the linker. so char str[] is linkable to asm startup
#ifndef BASENAME_STATICLINK
const char Class_ID[]= BaseName_CLASS_ID;
const char *VersionString = "basename.gadget 1.0 "; // add date
#endif
const char BaseNameSuperClassID[]=BaseName_SUPERCLASS_ID;




// note: if other boopsi classes are dependences, they need to be opened here.
#ifndef BASENAME_STATICLINK
    BOOL BaseName_OpenLibs(void)
    {
      // if here, sysbase is already acquired from LibInit.
      //NO: if(!SysBase) SysBase = *(( struct ExecBase **)4);
       if(!DOSBase)  DOSBase = (struct DosLibrary *)OpenLibrary("dos.library",1);
       if(!IntuitionBase)  IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library",39);
       if(!GfxBase) GfxBase = (struct GfxBase *) OpenLibrary("graphics.library",39);
       if(!UtilityBase) UtilityBase = OpenLibrary("utility.library",39);
       if(!LayersBase) LayersBase = OpenLibrary("layers.library",39);
    #if defined(__GNUC__) && (__GNUC__ < 3)
        __UtilityBase = UtilityBase; // amiga gcc2.95 with noixemul and 68000, and our gadget startup needs that.
    #endif
        return TRUE;
    }

    void BaseName_CloseLibs(void)
    {
        if(LayersBase) CloseLibrary(LayersBase);
        if(DOSBase) CloseLibrary((struct Library *)DOSBase);
        if(UtilityBase) CloseLibrary(UtilityBase);
        if(GfxBase) CloseLibrary((struct Library *)GfxBase);
        if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    }

#endif
BOOL BaseName_OpenLibs_Dependencies(void)
{
    // DEVTODO: if superclass
    // if(!SuperClassBase) SuperClassBase = OpenLibrary("superclass.class",44);
    // return (SuperClassBase != NULL);
    return TRUE;
}

void BaseName_CloseLibs_Dependencies(void)
{
    //if(SuperClassBase) CloseLibrary(SuperClassBase);
}
//==========================================================================================
// does not need to be exact, we just want the function pointer:
ULONG ASM SAVEDS BaseName_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M));

#ifndef BASENAME_STATICLINK
// called by shared Lib init to create class.

int ASM CreateClass(REG(a6,struct ExtClassLib *LibBase))
{
  if(LibBase) SysBase = LibBase->cb_SysBase;
  if(BaseName_OpenLibs() && BaseName_OpenLibs_Dependencies())
  {
    if(BaseNameClassPtr=MakeClass(BaseName_CLASS_ID,BaseNameSuperClassID,0,sizeof(BaseName),0))
    {
     if(LibBase) LibBase->cb_ClassLibrary.cl_Class = BaseNameClassPtr;
      BaseNameClassPtr->cl_Dispatcher.h_Data=LibBase;
      BaseNameClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)BaseName_Dispatcher;

      AddClass(BaseNameClassPtr);
      /* Success */
      return(0);
    }
    BaseName_CloseLibs_Dependencies();
    BaseName_CloseLibs();
  }
  /* Fail */
  return(-1);
}
// called by shared Lib expunge to dispose class.
void ASM DestroyClass(REG(a6,struct ExtClassLib *LibBase))
{
    // note LibBase and BaseNameClassPtr should be the same
    if(BaseNameClassPtr)
    {
      RemoveClass(BaseNameClassPtr);
      FreeClass(BaseNameClassPtr);
      BaseNameClassPtr = NULL;
    }
  BaseName_CloseLibs_Dependencies();
  BaseName_CloseLibs();
}
// first public lib function for boopsi classes
Class * ASM GetClass(void)
{
    return BaseNameClassPtr;
}
// end if shared class
#else
// static version:
Class *BASENAME_GetClass()
{
    return BaseNameClassPtr;
}
#endif

//====================================================================================

#ifdef BASENAME_STATICLINK

// just use this one once when static link
int BaseNameStaticInit()
{ 
   if(!BaseName_OpenLibs_Dependencies()) return 1;
    if(BaseNameClassPtr=MakeClass(NULL,BaseNameSuperClassID,0,sizeof(BaseName),0))
    {
      BaseNameClassPtr->cl_Dispatcher.h_Entry=(REHOOKFUNC)BaseName_Dispatcher;
     // do not AddClass() when static, no need to publish, BaseNameClassPtr will be enough.
      /* Success */
      return(0);
    }
    return 1;
}

void BaseNameStaticClose()
{
    BaseName_CloseLibs_Dependencies();
    if(BaseNameClassPtr)
    {
      FreeClass(BaseNameClassPtr);
      BaseNameClassPtr = NULL;
    }

}

#endif


