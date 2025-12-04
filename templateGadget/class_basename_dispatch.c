
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
                    REG(a2,struct Gadget *Gad),
                    REG(a1,union MsgUnion *M))
{
  BaseName *gdata;
  ULONG retval=0;
  gdata=INST_DATA(C, Gad);

  switch(M->MethodID)
  {
    case OM_NEW:
      if(Gad=(struct Gadget *)DoSuperMethodA(C,(Object *)Gad,(Msg)M))
      {
        gdata=INST_DATA(C, Gad);
        // DEVTODO: here you write the default values for your objects.
        gdata->_circleCenterX = 32767;
        gdata->_circleCenterY = 32767;

        gdata->_minimalWidth = 64;
        gdata->_minimalHeight = 64;

        // set gadget (super class) attributes for this instance like this:
        // (BOOL) Indicate whether gadget is part of TAB/SHIFT-TAB cycle.
        // default to false
        SetSuperAttrs(C,(Object *)Gad, GA_TabCycle,TRUE,TAG_DONE);

#ifdef USE_REGION_CLIPPING
    gdata->_clipRegion = NewRegion();
#endif
#ifdef USE_BEVEL_FRAME
          gdata->Bevel= NewObject(BEVEL_GetClass(),NULL,
            BEVEL_Style, BVS_BUTTON,
            BEVEL_FillPen, -1,
            TAG_END);
#endif

 //   Printf("instance:%lx\n",(int)gdata);
//        SetSuperAttrs(C,Gad, GA_TabCycle,1,TAG_DONE);

        // DEVTODO:
        // you could create instances of other objects as members...
        // to be deleted in OM_DISPOSE of course...

//        gdata->Pattern=NewObject(0,(UBYTE *)"mlr_ordered.pattern", TAG_DONE);
//        {
//          if(gdata->Bevel=BevelObject, BEVEL_Style, BVS_BUTTON, BEVEL_FillPen, -1, End)
//          {
//            gdata->Precision=8;
//            gdata->ShowSelected=1;

//            gad_SetAttrs(C,Gad,(struct opSet *)M);
//            retval=(ULONG)Gad;
//          }
//        }
        // means new object OK so far:
        retval=(ULONG)Gad;
      }
      break;

    case OM_UPDATE:
    case OM_SET:
       // Printf("OM_SET: GadgetID:%ld gad:%lx\n",(int)Gad->GadgetID,(int)Gad);
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      BaseName_SetAttrs(C,Gad,(struct opSet *)M);
     break;

    case OM_GET:
      BaseName_GetAttr(C,Gad,(struct opGet *)M);
     break;

    case OM_DISPOSE:
    #ifdef USE_BEVEL_FRAME
        if(gdata->Bevel) DisposeObject(gdata->Bevel);
    #endif
    #ifdef USE_REGION_CLIPPING
        if(gdata->_clipRegion) DisposeRegion(gdata->_clipRegion);
    #endif
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;

    case GM_HITTEST:
      retval = GMR_GADGETHIT;
      break;

    case GM_GOACTIVE:
      Gad->Flags |= GFLG_SELECTED;
      retval=BaseName_HandleInput(C,Gad,(struct gpInput *)M);
//      gad_Render(C,Gad,(APTR)M,GREDRAW_UPDATE);
//      retval=GMR_MEACTIVE;
      break;

    case GM_GOINACTIVE:
      Gad->Flags &= ~GFLG_SELECTED;
      BaseName_Render(C,Gad,(APTR)M,GREDRAW_UPDATE);
      break;

    case GM_LAYOUT:
      retval= BaseName_Layout(C,Gad,(struct gpLayout *)M);
      break;

    case GM_RENDER:
      retval=BaseName_Render(C,Gad,(struct gpRender *)M,0);
      break;

    case GM_HANDLEINPUT:
      retval=BaseName_HandleInput(C,Gad,(struct gpInput *)M);
      break;

    case GM_DOMAIN:
      BaseName_Domain(C, Gad, (APTR)M);
      retval=1;

    break;

    default:
  //  Printf(" - default unmanaged method - %lx\n",);
      retval=DoSuperMethodA(C,(Object *)Gad,(Msg)M);
      break;
  }
  return(retval);
}

