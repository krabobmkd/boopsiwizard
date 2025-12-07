
    #include <proto/exec.h>
    #include <proto/intuition.h>
    #include <proto/graphics.h>
    #include <proto/utility.h>
#ifdef __SASC
    #include <clib/alib_protos.h>
//    #include "minialib.h"
#else
    // GCC
    #include "minialib.h"
#endif
#include <proto/dos.h>
//#include <proto/utility.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_basename.h"
#include "class_basename_private.h"

#ifdef USE_BEVEL_FRAME
    #include <proto/bevel.h>
    #include <images/bevel.h>
#endif


/* Most of the calls to boopsi methods are not done from the App's context,
 * but from a specific intuition context, and because of that we can't use DOS calls
 * like dos/Printf() , and also stdlib printf().
 * So we may print debug informations with a special buffer,and function bdbprintf(),
 * hen flushbdbprint() in main process will print for real to standard output.
 * remove word USE_DEBUG_BDBPRINT to desactivate all bdbprintf()/flushbdbprint() calls.
 * Template projects that links boopsi classes statically use USE_DEBUG_BDBPRINT by default.
 * Template projects that uses boopsi classes with LoadLibrary() do not.
 */
#include "bdbprintf.h"

/** WATCH OUT ! boopsi docs says:
*  "the rkmmodelclass dispatcher must be able to run on Intuition's context,
*  which puts some limitations on what the dispatcher is permitted to do:
*  it can't use dos.library, it can't wait on application signals or message ports
* and it can't call any Intuition functions which might wait on Intuition."
*/
ULONG ASM SAVEDS BaseName_Dispatcher(
                    REG(a0,struct IClass *C),
                    REG(a2,Object *Obj),
                    REG(a1,union MsgUnion *M))
{
  BaseName *cdata;
  ULONG retval=0;
  cdata=INST_DATA(C, Obj);

  switch(M->MethodID)
  {
    case OM_NEW:
      if(Obj=(Object *)DoSuperMethodA(C,(Object *)Obj,(Msg)M))
      {
        cdata=INST_DATA(C, Obj);
        // DEVTODO: here you write the default values for your objects.
        cdata->_value = 1337;

        // set gadget (super class) attributes for this instance like this:
        // (BOOL) Indicate whether gadget is part of TAB/SHIFT-TAB cycle.
        // default to false
        //SetSuperAttrs(C,(Object *)Obj, SOMEPROPS,TRUE,TAG_DONE);

        // DEVTODO:
        // you could create instances of other objects as members...
        // to be deleted in OM_DISPOSE of course...

        // means new object OK so far:
        retval=(ULONG)Obj;
        // attribs that are passed at init, managed like updates.
        BaseName_SetAttrs(C,Obj,(struct opSet *)M);
      }
      break;

    case OM_UPDATE:
    case OM_SET:
      retval=DoSuperMethodA(C,(Object *)Obj,(Msg)M);
      BaseName_SetAttrs(C,Obj,(struct opSet *)M);
     break;

    case OM_GET:
      BaseName_GetAttr(C,Obj,(struct opGet *)M);
     break;

    case OM_DISPOSE:
      retval=DoSuperMethodA(C,(Object *)Obj,(Msg)M);
      break;

    default:

      retval=DoSuperMethodA(C,(Object *)Obj,(Msg)M);
      break;
  }
  return(retval);
}

