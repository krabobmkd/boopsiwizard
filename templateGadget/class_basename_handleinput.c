
#include <proto/exec.h>
#include <proto/intuition.h>

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


// example of how to send attribute change update message
ULONG BaseName_NotifyCoords(Class *C, struct Gadget *Gad, struct GadgetInfo	*GInfo)
{
    struct opUpdate notifymsg;
    BaseName *gdata=INST_DATA(C, Gad);
    ULONG tags[]={
        GA_ID,0,
        BASENAME_CenterX,0,
        BASENAME_CenterY,0,
        TAG_DONE
    };

    tags[1] = Gad->GadgetID;
    tags[3] = (LONG)gdata->_circleCenterX;
    tags[5] = (LONG)gdata->_circleCenterY;
    notifymsg.MethodID = OM_NOTIFY;
    notifymsg.opu_AttrList = (struct TagItem *)&tags[0];
    notifymsg.opu_GInfo = GInfo; // "always there for gadget, in all messages"
    notifymsg.opu_Flags = 0;
    return DoSuperMethodA(C,(APTR)Gad,(Msg)&notifymsg );

}

ULONG BaseName_HandleInput(Class *C, struct Gadget *Gad, struct gpInput *Input)
{
  ULONG retval=GMR_MEACTIVE; //default

  BaseName *gdata;
  struct InputEvent *ie;

  gdata=INST_DATA(C, Gad);
  retval = GMR_MEACTIVE;
  ie = Input->gpi_IEvent;

//  if(gdata->Disabled)
//    return(GMR_NOREUSE);

  switch(ie->ie_Class)
  {    case IECLASS_RAWKEY:
//      KP("RAW KEY CODE - %lx %8lx\n",ie->ie_Code,ie->ie_Qualifier);
//        gdata->gd_MouseMode=0;

      /*switch(ie->ie_Code)
      {

        case 0x4c: // UP
          {
            LONG t;

            t=gdata->ActivePen-gdata->Cols;
            if(t<0) t+=gdata->Pens;
            if(t<0) t=0; // double check!
            gdata->ActivePen=t;

            gad_Render(C,Gad,(APTR)Input,GREDRAW_UPDATE);
            BaseName_Notify(C,Gad,(APTR)Input, 0);
          }
          break;

        case 0x4f: // LEFT
          {
            LONG t;

            t=gdata->ActivePen-1;
            if(t<0) t=gdata->Pens-1;
            gdata->ActivePen=t;

            gad_Render(C,Gad,(APTR)Input,GREDRAW_UPDATE);
            BaseName_Notify(C,Gad,(APTR)Input, 0);
          }
          break;

        case 0x4e: // RIGHT
          {
            LONG t;

            t=gdata->ActivePen+1;
            if(t>=gdata->Pens) t=0;
            gdata->ActivePen=t;

            gad_Render(C,Gad,(APTR)Input,GREDRAW_UPDATE);
            BaseName_Notify(C,Gad,(APTR)Input, 0);
          }
          break;

        case 0x4d: // DOWN
          {
            LONG t;

            t=gdata->ActivePen+gdata->Cols;
            if(t>=gdata->Pens) t-=gdata->Pens;
            if(t>=gdata->Pens) t=0; // double check!
            gdata->ActivePen=t;

            gad_Render(C,Gad,(APTR)Input,GREDRAW_UPDATE);
            BaseName_Notify(C,Gad,(APTR)Input, 0);
          }
          break;

        default:
          if(MapRawKey(ie,buffer,MRK_BUFFER_SIZE,0))
          {
       //     KP("%ld %lc\n",buffer[0],buffer[0]);

            switch(buffer[0])
            {
              case 27: // Esc
                retval=GMR_NOREUSE;
                break;
              case 155: //  Shift + TAB
                retval=GMR_NOREUSE | GMR_PREVACTIVE;
                break;
              case  9:  // TAB
                retval=GMR_NOREUSE | GMR_NEXTACTIVE;
                break;
              case 0x20:
//                  step=(shifted?-1:1);
                break;
            }
          }
          break;

      }  // end ie_code switch
       */
      break;
    case IECLASS_RAWMOUSE:
      {
//        LONG x,y;
//        LONG r,c;

//        retval = GMR_MEACTIVE;

        // x=(Input->gpi_Mouse).X+Gad->LeftEdge;
        // y=(Input->gpi_Mouse).Y+Gad->TopEdge;

    // still in IECLASS_RAWMOUSE
        switch(ie->ie_Code)
         {

          case SELECTUP:
             gdata->_MouseMode=0;

           retval = GMR_MEACTIVE;
            break;

          case SELECTDOWN:
            // actually receive all clics on the whole WB !!
             if ( (((Input->gpi_Mouse).X < 0) ||
                 ((Input->gpi_Mouse).X >= Gad->Width) ||
                 ((Input->gpi_Mouse).Y < 0) ||
                 ((Input->gpi_Mouse).Y >= Gad->Height))
                  )
            {// outside gadget or disabled.

              if(gdata->_EditMode)
              {
                gdata->_EditMode=0;
                BaseName_Render(C,Gad,(APTR)Input,GREDRAW_UPDATE);
              }
//              retval = GMR_NOREUSE | GMR_VERIFY;
              retval = GMR_REUSE;
            }
            else if((Gad->Flags & GFLG_DISABLED)==0) // don't manage clicks if disabled.
            {
            LONG cx = gdata->_circleCenterX;
            LONG cy = gdata->_circleCenterY;

            // mouse click inside gadget !
            // recenter circle proportionaly.
            if(Gad->Width>0)
                cx = ((Input->gpi_Mouse).X <<16)/Gad->Width;
            if(Gad->Height>0)
                cy = ((Input->gpi_Mouse).Y <<16)/Gad->Height;

              gdata->_MouseMode=1;
              SetGadgetAttrs(Gad,Input->gpi_GInfo->gi_Window,NULL,
                    BASENAME_CenterX,cx,
                    BASENAME_CenterY,cy,
                    TAG_END
                );
              //BaseName_Render(C,Gad,(APTR)Input,GREDRAW_UPDATE);

              retval = GMR_MEACTIVE;
            }
            break;

 /* The user hit the menu button. Go inactive and let      */
                                     /* Intuition reuse the menu button event so Intuition can */
                                     /* pop up the menu bar.                                   */

       /*   case MENUDOWN:
          if(gdata->EditMode)//                                                                      (44.3.1) (09/01/00)
            {//                                                                                        (44.3.1) (09/01/00)
              gdata->EditMode=0;//                                                                     (44.3.1) (09/01/00)
              gad_Render(C,Gad,(APTR)Input,GREDRAW_UPDATE);//                                          (44.3.1) (09/01/00)
              BaseName_Notify(C,Gad,(APTR)Input, 0);//                                                        (44.3.1) (09/01/00)
            }//                                                                                        (44.3.1) (09/01/00)
            retval = GMR_REUSE;*/
                                          /* Since the gadget is going inactive, send a final   */
                                         /* notification to the ICA_TARGET.                    */
/*
            break;
            */
          default:
            retval = GMR_MEACTIVE;
        } // end of
      } // end of IECLASS_RAWMOUSE
      break;
  } // end of ieclass switch

    // if(notifCoords)
    // {
    //     BaseName_NotifyCoords(C,Gad,Input->gpi_GInfo);
    // }

  if(retval!=GMR_MEACTIVE)
  {
    //BaseName_Notify(C,Gad,(APTR)Input, 0);
  }

  return(retval);
}
