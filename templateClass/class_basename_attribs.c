
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

#include "class_basename.h"
#include "class_basename_private.h"

#include <utility/tagitem.h>



// ULONG BaseName_DoNotify(struct IClass *C, struct Gadget *Gad, Msg M, ULONG Flags, Tag Tags, ...);
ULONG BaseName_NotifyValueChange(Class *C, Object *Obj)
{
    struct opUpdate notifymsg;
    BaseName *cdata=INST_DATA(C, Obj);
    ULONG tags[]={
        BASENAME_Value,0, // will be set just after
       //  BASENAME_AnotherAttribToNotifyInTheSameTime,0,
        TAG_DONE
    };

    tags[1] = (LONG)cdata->_value;
   // tags[3] = (LONG)cdata->_somethingelse;
    notifymsg.MethodID = OM_NOTIFY;
    notifymsg.opu_AttrList = (struct TagItem *)&tags[0];
    notifymsg.opu_GInfo = NULL; // "always there for gadget, in all messages, else NULL (we are a class)"
    notifymsg.opu_Flags = 0;
    return DoSuperMethodA(C,(APTR)Obj,(Msg)&notifymsg );

}


ULONG BaseName_GetAttr(Class *C, Object *Obj, struct opGet *Get)
{
  ULONG retval=1;
  int   DoSuperCall=0;
  BaseName *cdata;
  ULONG *data;

  cdata=INST_DATA(C, Obj);

  data=Get->opg_Storage;

  switch(Get->opg_AttrID)
  {
    case BASENAME_Value:
        *data = (LONG)cdata->_value;
    break;
    //DEVTODO
    // case BASENAME_NextAtrib:
    //     *data = (LONG)cdata->_...;
    // break;
    // super class gadget things. would manage attribs selected/hightlighted, ...
    default:
        DoSuperCall = 1;
      // everything we don't manage directly is managed by supercall.
  }
  if(DoSuperCall)  retval=DoSuperMethodA(C, (APTR)Obj, (APTR)Get);

  return(retval);
}


ULONG BaseName_SetAttrs(Class *C, Object *Obj, struct opSet *Set)
{
  struct TagItem *tag;
  ULONG data; // for SetAttribs, retval means if anything needed redraw.
  BaseName *cdata;
  ULONG useful=0, notifValueChange=0;
// Printf("BaseName_SetAttrs\n");
  cdata=INST_DATA(C, Obj);

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
            case BASENAME_Value:
            if((int)data != cdata->_value )
            {
                cdata->_value = (int)data ;
                //Printf("set value:%ld\n",(int)data);
                notifValueChange = 1;
            }
            useful = 1;
            break;
            //DEVTODO... manage other attribs

        } // end switch
    } // end for


    if(notifValueChange)
    {
        BaseName_NotifyValueChange(C,Obj);
    }

  return(useful);
}


