
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>

#ifdef __SASC
//    #include "minialib.h"
    #include <clib/alib_protos.h>
#else
    // GCC
    #include "minialib.h"
#endif

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include "class_basename.h"
#include "class_basename_private.h"

#include <utility/tagitem.h>

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

ULONG BaseName_GetAttr(Class *C, struct Gadget *Gad, struct opGet *Get)
{
  ULONG retval=1;
  int   DoSuperCall=0;
  BaseName *gdata;
  ULONG *data;

  gdata=INST_DATA(C, Gad);

  data=Get->opg_Storage;

  switch(Get->opg_AttrID)
  {
    case BASENAME_CenterX:
        *data = (LONG)gdata->_circleCenterX;
    break;
    case BASENAME_CenterY:
        *data = (LONG)gdata->_circleCenterY;
    break;
    // super class gadget things. would manage attribs selected/hightlighted, ...
    default:
        DoSuperCall = 1;
      // everything we don't manage directly is managed by supercall.
  }
  if(DoSuperCall)  retval=DoSuperMethodA(C, (APTR)Gad, (APTR)Get);

  return(retval);
}



ULONG Redraw[]={0, GREDRAW_UPDATE, GREDRAW_REDRAW};

ULONG BaseName_SetAttrs(Class *C, struct Gadget *Gad, struct opSet *Set)
{
  struct TagItem *tag;
  ULONG data; // for SetAttribs, retval means if anything needed redraw.
  BaseName *gdata;
  ULONG redraw=0, update=0, notifCoords=0;

  gdata=INST_DATA(C, Gad);

 // set can use a list of attribs to change, so we manage this with a loop.
 // this also allows to have just one draw refresh for a set of change.
  for( tag = Set->ops_AttrList ;
        tag->ti_Tag != TAG_END ;
        tag++
   )
  {
    data=tag->ti_Data;

    switch(tag->ti_Tag)
    {
     case BASENAME_CenterX:
        if((UWORD)data != gdata->_circleCenterX )
        {
            gdata->_circleCenterX = (UWORD)data ;
            redraw=1;
            notifCoords = 1;
        }
        break;
     case BASENAME_CenterY:
        if((UWORD)data != gdata->_circleCenterY )
        {
            gdata->_circleCenterY = (UWORD)data ;

            redraw=1;
            notifCoords = 1;
        }
        break;
     // - - - actually we have to manage super class attribs:
     // with GA_XXX and struct Gadget members...
     // is there  a way to super call this ? DoSuperMethodA() deosn't seems to manage these attribs.
      case GA_Disabled:
        {
            if(data) Gad->Flags |= GFLG_DISABLED; // set bit
            else Gad->Flags &= ~GFLG_DISABLED; // remove bit.
            redraw=1;
        }
        break;
      case GA_Highlight:
        {
            if(data) Gad->Flags |= GFLG_GADGHBOX; // set bit
            else Gad->Flags &= ~GFLG_GADGHBOX; // remove bit.
            redraw=1;
        }
        break;
      case GA_Selected:
        {
            if(data) Gad->Flags |= GFLG_SELECTED; // set bit
            else Gad->Flags &= ~GFLG_SELECTED; // remove bit.
            redraw=1;
        }
        break;
    default:
        //does not seems to do anything for gadgets.... DoSuperMethodA(C,(APTR)Gad,(Msg)Set);
        //note: apparently super call is not to be managed here (not sure !!!)
        break;

    } // end switch
  } // end for

  if(redraw | update)
  {
    struct RastPort *rp;

    if(rp=ObtainGIRPort(Set->ops_GInfo))
    {
        // freeze with "..." call with GCC6.5, use local struct works 100%.
        struct gpRender gpr;
        gpr.MethodID = GM_RENDER;
        gpr.gpr_GInfo = Set->ops_GInfo;
        gpr.gpr_RPort = rp;
        gpr.gpr_Redraw = (redraw?GREDRAW_REDRAW:GREDRAW_UPDATE);

        DoMethodA((Object *)Gad,(Msg)&gpr.MethodID);
        ReleaseGIRPort(rp);
    }

    if(notifCoords)
    {
        BaseName_NotifyCoords(C,Gad,Set->ops_GInfo);
    }
  }

  return(redraw| update);
}


