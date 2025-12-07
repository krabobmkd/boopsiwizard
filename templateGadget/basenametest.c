/** little boopsi/reaction example
 * that uses the gadget to test,
 * in a layout, with a few buttons interaction.
 * This will use a static link of the class definition
 * when XXXXX_STATICLINK is defined,
 * (which is usefull for debugging)
 * and will search the shared .class file
 * when this is not defined.
 * Having different _STATICLINK names per
 * classes allow to have multiple class linked
 * as static or not per project.
 */
/**
 Useful reminder when coding Boopsi/Reaction:
    - Boopsi classes can extend modelclass, gadget, image, ...
    - class initialized with LoadLibrary() uses MakeClass() and AddClass(),
      have a registered name and are visible for all the system.
    - class initialized with just MakeClass() are visible only to your program.
    - In a layout gadget, you put gadgets with LAYOUT_AddChild.
    - In a layout gadget, you put images with LAYOUT_AddImage.
    - label.image can display text and/or images,
    - label.image extends "image" and the text can't change.
    - for a dynamic text label, use a flat button.
    - you change one or more object attributes with SetAttrs()
    - ...except for gadgets, in this case it's SetGadgetAttrs()
        (certainly because of the antique gadtools API if you ask.)
    - It's GetAttr() for everyone to get attributes values.
    - Definitions for attribs are in their class include down there.
    - All this "Datatype" story is the very same thing.

 https://wiki.amigaos.net/wiki/BOOPSI_-_Object_Oriented_Intuition
*/
#include "compilers.h"

// this is the public definition of the class we test:
#include "class_basename.h"

//#include <stdio.h>
//#include <string.h>
#include <stdlib.h>

#include <clib/alib_protos.h>
#include <clib/reaction_lib_protos.h>

#include <intuition/screens.h>
#include <intuition/icclass.h>

#include <proto/diskfont.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <exec/alerts.h>

#include <proto/window.h>
#include <classes/window.h>

#include <proto/layout.h>
#include <gadgets/layout.h>

#include <proto/button.h>
#include <gadgets/button.h>

#include <proto/checkbox.h>
#include <gadgets/checkbox.h>

#include <proto/label.h>
#include <images/label.h>

#include "bdbprintf.h"

INLINE struct Window *boopsi_OpenWindow(Object *owin) {
    return  (struct Window *)DoMethod(owin, WM_OPEN, NULL);
}

typedef ULONG (*REHOOKFUNC)();

// DOSBase is already opened by C startup...
// struct DosLibrary *DOSBase=NULL;
struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase *GfxBase=NULL;
struct Library *UtilityBase=NULL; // inlined DoMethod() may use CallHooksKpt().
struct Library *LayersBase=NULL; // only used by gadgets drawing in static link mode.

// used for appicon.
struct Library *IconBase=NULL;

// boopsi classes bases:
struct Library *WindowBase=NULL;
struct Library *LayoutBase=NULL;
struct Library *BitMapBase=NULL;
struct Library *ButtonBase=NULL;
struct Library *LabelBase=NULL;
struct Library *CheckBoxBase=NULL;

#ifndef BASENAME_STATICLINK
struct Library *BaseNameBase=NULL;
#endif


void cleanexit(const char *pmessage)
{
    if(pmessage) Printf("%s\n",pmessage);
    // will execute functions registered with atexit().
    // this way if C startup manages it, Ctrl-C will also close nicely.
    exit(0);
}
void exitclose(void);

// usefull union for dispatchers. Each structs also starts with MethodID.
typedef union ModelMsgUnion
{
  ULONG  MethodID;
  // from classusr.h or gadgetclass.h, all starts with MethodID.
  struct opSet        opSet;
  struct opUpdate     opUpdate;
  struct opGet        opGet;
} *Msgs;

/* Gadget action IDs, just to demonstrate some interactions
 */
#define GAD_BUTTON_RECENTER 1
#define GAD_BASENAME_TOTEST 2
#define GAD_DISABLECHECKBOX 3
// all app related variables are here:
// also we register that struct as a BOOPSI model,
// which is useful to receive notifications
// from gadgets that are created with ICA_TARGET, (ULONG)AppInstance
// let's just use it to store anything including gadgets...
// DEVTODO: you can extend this example app struct.
struct App
{
    Object *window_obj; // window as boopsi object
    struct Window *win; // current re-opened windows, as a classic intuition Window.

    struct MsgPort *app_port;

    struct Screen *lockedscreen;
    struct DrawInfo *drawInfo; // informations on how to draw on the screen, passed to gagdets.

    ULONG   fontHeight; // some stat to size according to current font.

    Object *mainlayout;
        Object *horizontallayout;
            Object *testbt;
            Object *kbdview;
        Object *bottombarlayout;
            Object *label1;
            Object *labelValues;
            Object *disablecheckbox;
};
// Boopsi class pointer to manage our private modelclass.
Class *AppModelClass = NULL;
// App Model instance as a Boopsi object.
struct Object *AppInstance = NULL;
// App Modelinstance as our private struct.
struct App *app=NULL;

ULONG ASM SAVEDS AppModelDispatch(
                    REG(a0,struct IClass *C),
                    REG(a2,struct Object *obj),
                    REG(a1,union ModelMsgUnion *M))
{
    // Warning: this is executed on "intuition's context", can't use dos nor print, like for interupts.
  ULONG retval=0;
  switch(M->MethodID)
  {
    case OM_NEW:
        if(obj=(Object *)DoSuperMethodA(C,(Object *)obj,(Msg)M))
        {
            app=INST_DATA(C, obj);
            retval = (ULONG)obj;
        }
    break;
    case OM_DISPOSE:
        retval=DoSuperMethodA(C,(Object *)obj,(Msg)M);
      break;
    case OM_UPDATE:
        {
            struct TagItem *ptag;
            // here receive events from gadgets as target.
            ULONG sender_ID=0;
            if((ptag = FindTagItem( GA_ID,M->opUpdate.opu_AttrList ))!=NULL) sender_ID = ptag->ti_Data;

            // our gadget is notifying new clicked coordinates!
            if( sender_ID == GAD_BASENAME_TOTEST )
            {   // table used as parameter for the button internal sprintf() formating
                LONG centerXY[2];
                if((ptag = FindTagItem( BASENAME_CenterX,M->opUpdate.opu_AttrList ))!=NULL)
                    centerXY[0] = ((UWORD)ptag->ti_Data * 100)>>16; // get percent
                if((ptag = FindTagItem( BASENAME_CenterY,M->opUpdate.opu_AttrList ))!=NULL)
                    centerXY[1] = ((UWORD)ptag->ti_Data * 100)>>16;

                if(app->labelValues)
                {
                    // display coords in button label with formatting:
                    SetGadgetAttrs((struct Gadget *)app->labelValues,app->win,NULL,
                        GA_Text,(ULONG)"X: %ld %% Y: %ld %%", // in amiga API %d is for short and %ld for longs.
                        BUTTON_VarArgs,(ULONG) &centerXY[0],
                        TAG_END);
                }
                retval = 1;
            } else if(sender_ID == GAD_DISABLECHECKBOX)
            {   // also works, but would be activated for all attribs sent:
                //ULONG v;
                //GetAttr(GA_SELECTED, app->disablecheckbox, &v);
                if((ptag = FindTagItem( GA_SELECTED,M->opUpdate.opu_AttrList ))!=NULL)
                {   // checkbox sent new Disable value.
                    SetGadgetAttrs((struct Gadget *)app->kbdview,app->win,NULL, GA_DISABLED,ptag->ti_Data,TAG_END);
                }
            }
            else // if ...other receive mamangement... else
            {
                retval=DoSuperMethodA(C,(Object *)obj,(Msg)M);
            }
        }
        break;
    default:
        retval=DoSuperMethodA(C,(Object *)obj,(Msg)M);
    break;
  }
  return retval;
}

int initAppModel(void)
{
    // this is how you create a private transient class:
    AppModelClass = MakeClass(NULL,"modelclass",NULL,sizeof(struct App),0);
    if(!AppModelClass) return 0;

    AppModelClass->cl_Dispatcher.h_Entry = (REHOOKFUNC) &AppModelDispatch;

    AppInstance = NewObject( AppModelClass, NULL, TAG_DONE);
    if(!AppInstance) return 0;

    return 1;
}
void closeAppModel(void)
{
    if(AppInstance) DisposeObject(AppInstance);
    AppInstance = NULL;
    app=NULL;
    if(AppModelClass) FreeClass(AppModelClass);
    AppModelClass = NULL;
}
//  - - - -- - - - -  end of App modelclass management.

int main(int argc, char **argv)
{
    atexit(&exitclose);

    // - - - - open libraries...

    if ( ! (IntuitionBase = (struct IntuitionBase*)OpenLibrary("intuition.library",33)))
        cleanexit("Can't open intuition.library");

    if ( ! (GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",39)))
        cleanexit("Can't open graphics.library");

    if ( ! (UtilityBase = OpenLibrary("utility.library",39)))
        cleanexit("Can't open utility.library");

    if ( ! (LayersBase = OpenLibrary("layers.library",39)))
        cleanexit("Can't open layers.library");

    if ( ! (IconBase = OpenLibrary("icon.library",39)))
        cleanexit("Can't open icon.library");

    // note: DOSBase is opened by C startup.

    // - - - - open boopsi classes...

    if ( ! (WindowBase = OpenLibrary("window.class",44)))
        cleanexit("Can't open window.class");

    if ( ! (LayoutBase = OpenLibrary("gadgets/layout.gadget",44)))
        cleanexit("Can't open layout.gadget");

//    if ( ! (BitMapBase = OpenLibrary("images/bitmap.image",44)))
//        cleanexit("Can't open bitmap.image");

    if ( ! (ButtonBase = OpenLibrary("gadgets/button.gadget",44)))
        cleanexit("Can't open button.gadget");

    if ( ! (LabelBase = OpenLibrary("images/label.image",44)))
        cleanexit("Can't open label.image");

   if ( ! (CheckBoxBase = OpenLibrary("gadgets/checkbox.gadget",44)))
       cleanexit("Can't open checkbox.gadget");

#ifdef BASENAME_STATICLINK

    if(BaseNameStaticInit()) cleanexit("Can't create private class");
#else
    if ( ! (BaseNameBase = OpenLibrary("basename.gadget",VERSION_BASENAME)))
        cleanexit("Can't open basename.gadget");
#endif


    if(!initAppModel())  cleanexit("Can't create app");


    // = = = = = creates the instances...

    app->lockedscreen = LockPubScreen(NULL);
    if (!app->lockedscreen) cleanexit("Can't lock screen");

    app->drawInfo = GetScreenDrawInfo(app->lockedscreen);
    // let's size according to font height.
    app->fontHeight = 8+4; // default;
    if(app->drawInfo && app->drawInfo->dri_Font) app->fontHeight =app->drawInfo->dri_Font->tf_YSize + 4;

    app->testbt = (Object *)NewObject( NULL, "button.gadget",
                                    GA_DrawInfo, app->drawInfo,
                              //      GA_TextAttr, &garnet16,
                                    GA_ID,GAD_BUTTON_RECENTER,
                                    GA_Text, "R_ecenter",
                                    GA_RelVerify, TRUE, // needed
                                TAG_END);


    if(!app->testbt) cleanexit("Can't button");


    // with BASENAME_STATICLINK, BASENAME_GetClass() is a function,
    //  else it is a vector function from a shared class library.
    app->kbdview = NewObject(BASENAME_GetClass(), NULL,
        GA_DrawInfo, app->drawInfo,
        GA_ID,      GAD_BASENAME_TOTEST, // Gadget ID assigned by the application, needed to sort notifies.
        ICA_TARGET, (ULONG)AppInstance,     // app model will receive notifications.
            TAG_END);


    if(!app->kbdview) cleanexit("Can't create kbdview");

    app->horizontallayout = (Object *)NewObject( LAYOUT_GetClass(), NULL,
                LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                LAYOUT_EvenSize, TRUE,
                LAYOUT_HorizAlignment, LALIGN_RIGHT,
               // LAYOUT_SpaceInner, FALSE,
                LAYOUT_AddChild, app->testbt,
                LAYOUT_AddChild, app->kbdview,
                TAG_DONE);

    if(!app->horizontallayout) cleanexit("Can't layout 1");



 app->label1 = (Object *)NewObject( LABEL_GetClass(), NULL,
                        LABEL_DrawInfo, app->drawInfo,
                        //IA_Font, &helvetica15bu,
                        //LABEL_SoftStyle, FSF_BOLD | FSF_ITALIC,
                        LABEL_Justification, LABEL_CENTRE,
                        LABEL_Text,(ULONG)"Values:",
                    TAG_END);
 // label which changing text are reconfigured buttons...
 app->labelValues = (Object *)NewObject( NULL, "button.gadget",
                        GA_DrawInfo,(ULONG) app->drawInfo,
                        BUTTON_BevelStyle,BVS_NONE,
                        BUTTON_Transparent, TRUE,
						GA_ReadOnly, TRUE,
                        BUTTON_Justification, BCJ_CENTER,
						
                        GA_Text,(ULONG)"...",
                    TAG_END);

    app->disablecheckbox = (Object *)NewObject( CHECKBOX_GetClass(), NULL,
                    GA_DrawInfo,(ULONG) app->drawInfo,
                    GA_Text,(ULONG)"Disable",
                 GA_ID,GAD_DISABLECHECKBOX,
                 ICA_TARGET, (ULONG)AppInstance,     // app model will receive notifications.
                TAG_END);

    if(!app->disablecheckbox) cleanexit("Can't create checkbox");

    app->bottombarlayout =
         (Object *)NewObject( LAYOUT_GetClass(), NULL,
                LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                LAYOUT_EvenSize, TRUE,
                LAYOUT_HorizAlignment, LALIGN_RIGHT,
              //  CHILD_ScaleHeight,1, //%
               // CHILD_MaxHeight,app->fontHeight,
               // LAYOUT_SpaceInner, FALSE,
                LAYOUT_AddImage, app->label1,
                LAYOUT_AddChild, app->labelValues,
                LAYOUT_AddChild, app->disablecheckbox,
              //  GA_Height,app->fontHeight,
                TAG_DONE);

    if(!app->bottombarlayout) cleanexit("Can't layout 2");

    {
     //   struct DrawInfo *drinfo = GetScreenDrawInfo(screen);
        app->mainlayout = (Object *)NewObject( LAYOUT_GetClass(), NULL,
            GA_DrawInfo, app->drawInfo,
            LAYOUT_DeferLayout, TRUE, /* Layout refreshes done on task's context (by thewindow class) */
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_BottomSpacing, 4,
            LAYOUT_HorizAlignment, LALIGN_RIGHT,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_AddChild, app->horizontallayout,
            LAYOUT_AddChild, app->bottombarlayout,
                CHILD_WeightedHeight,0,
            TAG_END);
        if (!app->mainlayout) cleanexit("layout error 3");
    } //end if screen


    app->app_port = CreateMsgPort();

    /* Create the window object. */
    app->window_obj = (Object *)NewObject( WINDOW_GetClass(), NULL,
        WA_Left, 0,
        WA_Top, app->lockedscreen->Font->ta_YSize + 3,
        WA_CustomScreen, app->lockedscreen,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_RAWKEY /*| IDCMP_VANILLAKEY*/, // we want localized keys , not the raws.
        WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_SIZEGADGET | WFLG_ACTIVATE | WFLG_SMART_REFRESH,
        WA_Title, "BaseName Gadget Test",
        WINDOW_ParentGroup, app->mainlayout,
        WINDOW_IconifyGadget, TRUE,
        WINDOW_Icon, GetDiskObject("PROGDIR:ReAction"),
        WINDOW_IconTitle, "BaseName Test",
        WINDOW_AppPort, app->app_port,
    TAG_END);
    if(!app->window_obj) cleanexit("can't create window");

    /*  Open the window. */
    app->win = boopsi_OpenWindow(app->window_obj);
    if(!app->win) cleanexit("can't open window");

    {
        ULONG signal;
        BOOL ok = TRUE;

        /* Obtain the window wait signal mask.*/
        GetAttr(WINDOW_SigMask, app->window_obj, &signal);

        /* Input Event Loop */
        while (ok)
        {
            ULONG result;

            Wait(signal | (1L << app->app_port->mp_SigBit));

            flushbdbprint();

            /* CA_HandleInput() returns the gadget ID of a clicked
             * gadget, or one of several pre-defined values.  For
             * this demo, we're only actually interested in a
             * close window and a couple of gadget clicks.
             */
            while ((result = DoMethod(app->window_obj, WM_HANDLEINPUT, /*code*/NULL)) != WMHI_LASTMSG)
            {
                switch(result & WMHI_CLASSMASK)
                {
                   case WMHI_RAWKEY:
                        // quit on "esc down" key.
                        if((result & WMHI_KEYMASK) == 0x45) ok = FALSE;
                        break;
                    case WMHI_CLOSEWINDOW:
                        // quit on window close gadget
                        ok = FALSE;
                        break;

                    case WMHI_GADGETUP:
                        switch (result & WMHI_GADGETMASK)
                        {
                            case GAD_BUTTON_RECENTER:
                                // does the button action.
                                // change attributes of the gadget we created:
                                // watch out it's SetGadgetAttrs and not SetAttrs() for gadgets...
                                SetGadgetAttrs((struct Gadget *)app->kbdview,app->win,NULL,
                                    BASENAME_CenterX, 32768,
                                    BASENAME_CenterY, 32768,
                                    TAG_DONE);
                            break;

                            default:
                                break;
                        }
                        break;

                    case WMHI_ICONIFY:
                        //if (RA_Iconify(window_obj)) win = NULL;
                        if(DoMethod(app->window_obj, WM_ICONIFY, NULL)) app->win = NULL;
                        break;

                    case WMHI_UNICONIFY:
                        app->win = boopsi_OpenWindow(app->window_obj);
                        if (!app->win) cleanexit("can't open window");

                        break;

                    default:
                        break;
                }


            } // end while messages
        } // end while app loop
    } // loop paragraph end

    flushbdbprint();

    // all close done in exitclose().
    return 0;
}


void exitclose(void)
{
    if(app)
    {
        /* Disposing of the window object will also close the
         * window if it is already opened and it will dispose of
         * all objects attached to it.
         */
        if(app->window_obj) DisposeObject(app->window_obj);
        else {
            // but if not attached because mid-init fail, has to be manual.
            if(app->mainlayout)  DisposeObject(app->mainlayout);
            else {
                if(app->horizontallayout) DisposeObject(app->horizontallayout);
                else {
                    if(app->testbt) DisposeObject(app->testbt);
                    if(app->kbdview) DisposeObject(app->kbdview);
                }
                if(app->bottombarlayout) DisposeObject(app->bottombarlayout);
                else {
                    if(app->label1) DisposeObject(app->label1);
                    if(app->labelValues) DisposeObject(app->labelValues);
                    if(app->disablecheckbox) DisposeObject(app->disablecheckbox);
                }
            }
        }

        if(app->drawInfo) FreeScreenDrawInfo(app->lockedscreen, app->drawInfo);
        if(app->lockedscreen) UnlockPubScreen(0, app->lockedscreen);

    }
    closeAppModel();


#ifndef BASENAME_STATICLINK
    if(BaseNameBase) CloseLibrary(BaseNameBase);
#else
    BaseNameStaticClose();
#endif
    if(CheckBoxBase) CloseLibrary(CheckBoxBase);
    if(LabelBase) CloseLibrary(LabelBase);
    if(ButtonBase) CloseLibrary(ButtonBase);
    if(BitMapBase) CloseLibrary(BitMapBase);
    if(LayoutBase) CloseLibrary(LayoutBase);
    if(WindowBase) CloseLibrary(WindowBase);

    if(GfxBase) CloseLibrary((struct Library*)GfxBase);
    if(IntuitionBase) CloseLibrary((struct Library*)IntuitionBase);
    if(LayersBase) CloseLibrary(LayersBase);
    if (UtilityBase) CloseLibrary(UtilityBase);

    //if(DOSBase) CloseLibrary((struct Library*)DOSBase);
    if(IconBase) CloseLibrary(IconBase);
}

