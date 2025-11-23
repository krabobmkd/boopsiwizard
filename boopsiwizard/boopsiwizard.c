
#include <stdio.h>
#include <string.h>
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

#include <proto/string.h>
#include <gadgets/string.h>

#include <proto/texteditor.h>
#include <gadgets/texteditor.h>

#include <proto/requester.h>
#include <classes/requester.h>

#include <proto/asl.h>
#include <libraries/asl.h>

#include "compilers.h"

#include "templates.h"

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
struct Library *AslBase=NULL;

// boopsi classes bases:
struct Library *WindowBase=NULL;
struct Library *LayoutBase=NULL;
struct Library *BitMapBase=NULL;
struct Library *ButtonBase=NULL;
struct Library *LabelBase=NULL;
struct Library *CheckBoxBase=NULL;
struct Library *StringBase=NULL;
struct Library *TextFieldBase=NULL;
struct Library *RequesterBase=NULL;

void cleanexit(const char *pmessage)
{
    if(pmessage) printf("%s\n",pmessage);
    // will execute functions registered with atexit().
    // this way if C startup manages it, Ctrl-C will also close nicely.
    exit(0);
}
void exitclose(void);

void guiNotifier(int loglevel, const char *log);
// synchronize greying buttons...
void updateUIToStates();
void selectTemplate(int i);
void generate();
void openAboutReq();
// usefull union for dispatchers. Each structs also starts with MethodID.
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

/* Gadget action IDs, just to demonstrate some interactions
 */
#define GAD_BUTTON_GENERATE 1
#define GAD_BUTTON_ABOUT 2
#define GAD_CB_SASC 3
#define GAD_CB_MAKEFILE 4
#define GAD_CB_CMAKELIST 5

#define GAD_START_SELECT_TEMPLATE 16



// all app related variables are here:
struct App
{
    Object *window_obj; // window as boopsi object
    struct Window *win; // current re-opened windows, as a classic intuition Window.

    struct MsgPort *app_port;

    struct Screen *lockedscreen;
    struct DrawInfo *drawInfo; // informations on how to draw on the screen, passed to gagdets.

    ULONG   fontHeight; // some stat to size according to current font.

    Object *mainvlayout;
        Object *horizontallayoutA;
         //   Object *titlelabel;
            Object* btAbout;
        Object *horizontallayoutB;
        Object *layoutBList;
            // form constants
            Object *projectNameString;
            Object *projectDescription;
        Object *layoutBForm;

        Object* btGenerate;

            // status bar
        Object *horizontallayoutC;
        Object *bottombarlayout;
            Object* statusbarlabel;

    Object **TemplateButtonsList;

     Object *reportReq;

};
// - - - note having a private "boopsi object class and instance"
// - - - makes it fancy to connect values and receive events.
// Boopsi class pointer to manage our private modelclass.
Class *AppModelClass = NULL;
// App Model instance as a Boopsi object.
Object *AppInstance = NULL;
// App Modelinstance as our private struct.
struct App *app=NULL;

ULONG ASM SAVEDS AppModelDispatch(
                    REG(a0,struct IClass *C),
                    REG(a2,Object *obj),
                    REG(a1,union MsgUnion *M))
{
    // Warning: this is executed on "intuition's context", can't use dos nor print, like for interupts.
  ULONG retval=0;
  switch(M->MethodID)
  {
    case OM_NEW:
        if(obj=(Object *)DoSuperMethodA(C,(Object *)obj,(Msg)M))
        {
            app=(struct App *)INST_DATA(C, obj);
            memset(app,0,sizeof(struct App)); // absolutely *NOT* sure about this being cleaned, more secure.
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
            // note any button action is either managed here or in more generic main loop

            // if( sender_ID >= GAD_START_SELECT_TEMPLATE)
            // {
            // } else
            // if( sender_ID == GAD_BUTTON_GENERATE )
            // {
            //     retval = 1;
            // } else
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
    // -First param: no name needed for itself.
    // - "modelclass" is super class name, which is the base for all boopsi class.
    // a super class name or pointer must always be provided.
    AppModelClass = MakeClass(NULL,"modelclass",NULL,sizeof(struct App),0);
    if(!AppModelClass) return 0;

    AppModelClass->cl_Dispatcher.h_Entry = (REHOOKFUNC) &AppModelDispatch;

    AppInstance = (Object *)NewObject( AppModelClass, NULL, TAG_DONE);
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

static Object *populateTemplateList()
{

    int nbtemplates = getNbTemplates();
    sWizTemplate *pt;

    static const ULONG const listtagsstart[]={
                LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                LAYOUT_EvenSize, TRUE,
                LAYOUT_HorizAlignment, LALIGN_RIGHT
                };

    int nbstartpairs=3;
    ULONG *plisttags = AllocVec(sizeof(listtagsstart)+(sizeof(ULONG)*(8*nbtemplates+3)),0);
    memcpy(plisttags,listtagsstart,sizeof(listtagsstart));
    ULONG *plisttagsr =  plisttags + (sizeof(listtagsstart)/sizeof(ULONG)) ;

    pt = getTemplates();

    int nbtemplate = getNbTemplates();
    if(nbtemplate>0)
    {
        app->TemplateButtonsList = AllocVec(sizeof(Object *)*nbtemplate,MEMF_CLEAR );
    }
    Object **pTmpleBtList = app->TemplateButtonsList;
    int itemplate=0;
    while(pt) {
        const char *pdisplayname = (pt->_displayName)?pt->_displayName:"?";
        Object* label1 = NewObject( BUTTON_GetClass(),NULL,
                            GA_Text,(ULONG)pdisplayname,
                            GA_RelVerify, TRUE,
                             BUTTON_Justification, BCJ_LEFT,
                            BUTTON_PushButton, TRUE,
                        GA_ID,(GAD_START_SELECT_TEMPLATE+itemplate),
                        TAG_END);
        *pTmpleBtList++ =  label1;


        *plisttagsr++ = LAYOUT_AddChild;
        *plisttagsr++ = (ULONG)label1;
        *plisttagsr++ = CHILD_WeightedHeight;
        *plisttagsr++ = 0;
        if(pt->_versionstring)
        {
            Object* oversion= (Object *)NewObject( BUTTON_GetClass(),NULL,
                GA_DrawInfo,(ULONG) app->drawInfo,
                BUTTON_BevelStyle,BVS_NONE,
                BUTTON_Transparent, TRUE,
                GA_ReadOnly, TRUE,
                BUTTON_Justification, BCJ_LEFT,
                GA_Text,(ULONG)pt->_versionstring,

            TAG_END);
            *plisttagsr++ = LAYOUT_AddChild;
            *plisttagsr++ = (ULONG)oversion;
            *plisttagsr++ = CHILD_WeightedHeight;
            *plisttagsr++ = 0;
        }
        pt = pt->_pNext;
        itemplate++;
    }

    Object* ospacer = NewObject( BUTTON_GetClass(),NULL,
                //GA_DrawInfo,(ULONG) app->drawInfo,
                BUTTON_BevelStyle,BVS_NONE,
                BUTTON_Transparent, TRUE,
                GA_ReadOnly, TRUE,
                BUTTON_Justification, BCJ_CENTER,
                GA_Text,(ULONG)" ",
                TAG_END);


    *plisttagsr++ = LAYOUT_AddChild;
    *plisttagsr++ = (ULONG)ospacer;
    *plisttagsr++ = TAG_END;

    Object *pTemplateLayoutList =
        (Object *)NewObjectA( LAYOUT_GetClass(), NULL,plisttags);
   FreeVec(plisttags);

    return pTemplateLayoutList;
}


//  - - - -- - - - -  end of App modelclass management.

int main(int argc, char **argv)
{
    atexit(&exitclose);
    initTemplates(&guiNotifier);

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

    if ( ! (AslBase = OpenLibrary("asl.library",39)))
        cleanexit("Can't open asl.library");
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

    if ( ! (StringBase = OpenLibrary("gadgets/string.gadget",44)))
        cleanexit("Can't open string.gadget");

    if ( ! (TextFieldBase = OpenLibrary("gadgets/texteditor.gadget",44)))
        cleanexit("Can't open texteditor.gadget");

    if ( ! (RequesterBase = OpenLibrary("requester.class",44)))
        cleanexit("Can't open requester.class");

    if(!initAppModel())  cleanexit("Can't create app");


    // = = = = = now that needed classes are loaded
    // = = = = = creates the instances...

    app->lockedscreen = LockPubScreen(NULL);
    if (!app->lockedscreen) cleanexit("Can't lock screen");

    app->drawInfo = GetScreenDrawInfo(app->lockedscreen);
    // let's size according to font height.
    app->fontHeight = 8+4; // default;
    if(app->drawInfo && app->drawInfo->dri_Font) app->fontHeight =app->drawInfo->dri_Font->tf_YSize + 4;

    {
        Object* label1 = (Object *)NewObject( LABEL_GetClass(), NULL,
                        LABEL_DrawInfo, app->drawInfo,
                        //IA_Font, &helvetica15bu,
                        //LABEL_SoftStyle, FSF_BOLD | FSF_ITALIC,
                        LABEL_Justification, LABEL_CENTRE,
                        LABEL_Text,(ULONG)"Boopsi Wizard 0.2 beta",
                    TAG_END);

        app->btAbout = NewObject( BUTTON_GetClass(),NULL,
                                    GA_Text, "About...",
                                    GA_ID,GAD_BUTTON_ABOUT,
                                    GA_RelVerify, TRUE,
                         //           GA_Disabled,TRUE,
                        // BUTTON_BevelStyle,BVS_NONE,
                        // BUTTON_Transparent, TRUE,
                                TAG_END);

        app->horizontallayoutA =
             (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_EvenSize, TRUE,
                    LAYOUT_HorizAlignment, LALIGN_CENTER,
                    LAYOUT_BevelStyle, BVS_GROUP,
                    LAYOUT_AddImage, label1,
                     CHILD_WeightedWidth,1,
                    LAYOUT_AddChild, app->btAbout,
                     CHILD_WeightedWidth,0,
                    TAG_DONE);
    }

    {
        app->layoutBList = populateTemplateList();
    }
    {
        // Object* label1 = (Object *)NewObject( LABEL_GetClass(), NULL,
        //                 LABEL_DrawInfo, app->drawInfo,
        //                 //IA_Font, &helvetica15bu,
        //                 //LABEL_SoftStyle, FSF_BOLD | FSF_ITALIC,
        //                 LABEL_Justification, LABEL_CENTRE,
        //                 LABEL_Text,(ULONG)"Form",
        //             TAG_END);

                    // LAYOUT_AddChild
        // in this layout, all should have a CHILD_Label
        Object *subform;
        {

        app->projectNameString = NewObject( STRING_GetClass(), NULL,
                        GA_RelVerify, TRUE,
                        STRINGA_MaxChars, 26,
                        STRINGA_TextVal, "ProjectName",
                    TAG_END);
                    // CHILD_Label
          Object *label_ProjectNameString = NewObject( LABEL_GetClass(), NULL,
                        LABEL_Text, "Set Project Name",
                            TAG_END);

            subform = (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                    LAYOUT_EvenSize, FALSE,
                    LAYOUT_HorizAlignment, LALIGN_RIGHT,
                    LAYOUT_BevelStyle, BVS_NONE,

                    LAYOUT_AddChild, app->projectNameString,
                    CHILD_Label, label_ProjectNameString,
                    CHILD_WeightedHeight,0,

                    // LAYOUT_AddChild,formspacer,
                    // CHILD_WeightedHeight,1,

                    TAG_DONE);

        }

        app->projectDescription = NewObject( TEXTEDITOR_GetClass(), NULL,
                        GA_TEXTEDITOR_Contents,(ULONG)"...",
                        GA_TEXTEDITOR_ReadOnly, TRUE,
                        //        LAYOUT_BevelStyle, BVS_NONE,
                    TAG_END);

    // Object* formspacer = NewObject( BUTTON_GetClass(),NULL,
    //             //GA_DrawInfo,(ULONG) app->drawInfo,
    //             BUTTON_BevelStyle,BVS_NONE,
    //             BUTTON_Transparent, TRUE,
    //             GA_ReadOnly, TRUE,
    //             BUTTON_Justification, BCJ_CENTER,
    //             GA_Text,(ULONG)" ",
    //             TAG_END);

        app->layoutBForm =
             (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                    LAYOUT_EvenSize, FALSE,
                    LAYOUT_HorizAlignment, LALIGN_RIGHT,
                    LAYOUT_BevelStyle, BVS_GROUP,

                    LAYOUT_TopSpacing,2,
                    LAYOUT_LeftSpacing,2,
                    LAYOUT_RightSpacing,2,
                    LAYOUT_BottomSpacing,2,

                  //  CHILD_ScaleHeight,1, //%
                   // CHILD_MaxHeight,app->fontHeight,
                   // LAYOUT_SpaceInner, FALSE,
     //               LAYOUT_AddImage, label1,
                   // CHILD_WeightedHeight,0,

                    LAYOUT_AddChild, subform,
                    CHILD_WeightedHeight,0,

                    LAYOUT_AddChild,app->projectDescription,
                    CHILD_WeightedHeight,1,

                    // LAYOUT_AddChild,formspacer,
                    // CHILD_WeightedHeight,1,

                    TAG_DONE);

        app->horizontallayoutB =
             (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_EvenSize, TRUE,
                    LAYOUT_HorizAlignment, LALIGN_RIGHT,
                  //  CHILD_ScaleHeight,1, //%
                   // CHILD_MaxHeight,app->fontHeight,
                   // LAYOUT_SpaceInner, FALSE,
                   // LAYOUT_AddImage, label1,
                   LAYOUT_AddChild,  app->layoutBList,
                CHILD_WeightedWidth,0,
                    LAYOUT_AddChild, app->layoutBForm,
                CHILD_WeightedWidth,1,
                  //  GA_Height,app->fontHeight,
                    TAG_DONE);
    }

    {

                    Object* cbsasc =  (Object *)NewObject( CHECKBOX_GetClass(), NULL,
                        GA_DrawInfo,(ULONG) app->drawInfo,
                        GA_Text,(ULONG)"SASC6.5 smakefile (1996,C90)",
                        CHECKBOX_Checked,TRUE,
                     GA_ID,GAD_CB_SASC,
                     ICA_TARGET, (ULONG)AppInstance,     // app model will receive notifications.
                    TAG_END);


                    Object* cbgcc =  (Object *)NewObject( CHECKBOX_GetClass(), NULL,
                        GA_DrawInfo,(ULONG) app->drawInfo,
                        GA_Text,(ULONG)"GCC2.9x makefile (1999,C98)",
                        CHECKBOX_Checked,TRUE,
                     GA_ID,GAD_CB_MAKEFILE,
                     ICA_TARGET, (ULONG)AppInstance,     // app model will receive notifications.
                    TAG_END);

                    Object* cbcmake =  (Object *)NewObject( CHECKBOX_GetClass(), NULL,
                        GA_DrawInfo,(ULONG) app->drawInfo,
                        GA_Text,(ULONG)"GCC6.5 CMakeList.txt (2011,C11)",
                        CHECKBOX_Checked,TRUE,
                     GA_ID,GAD_CB_CMAKELIST,
                     ICA_TARGET, (ULONG)AppInstance,     // app model will receive notifications.
                    TAG_END);


        Object *targetcblayout =     (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                    LAYOUT_EvenSize, TRUE,
                    LAYOUT_HorizAlignment, LALIGN_RIGHT,
                   LAYOUT_AddChild, cbsasc,
                   LAYOUT_AddChild, cbgcc,
                   LAYOUT_AddChild, cbcmake,
                    TAG_DONE);

        app->btGenerate = NewObject( BUTTON_GetClass(),NULL,
                                    GA_Text, "Generate Project",
                                    GA_ID,GAD_BUTTON_GENERATE,
                                    GA_RelVerify, TRUE,
                                    GA_Disabled,TRUE,
                        // BUTTON_BevelStyle,BVS_NONE,
                        // BUTTON_Transparent, TRUE,
                                TAG_END);

        app->horizontallayoutC =
             (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_EvenSize, TRUE,
                    LAYOUT_HorizAlignment, LALIGN_RIGHT,
                    LAYOUT_BevelStyle, BVS_GROUP,
                  //  CHILD_ScaleHeight,1, //%
                   // CHILD_MaxHeight,app->fontHeight,
                   // LAYOUT_SpaceInner, FALSE,
//                   LAYOUT_AddChild, ospacer,
//                CHILD_WeightedWidth,1,

                     LAYOUT_AddChild, targetcblayout,
                 CHILD_WeightedWidth,3,

                    LAYOUT_AddChild, app->btGenerate,
                 CHILD_WeightedWidth,0,
                  //  LAYOUT_AddChild, app->labelValues,
                   // LAYOUT_AddChild, app->disablecheckbox,
                  //  GA_Height,app->fontHeight,
                    TAG_DONE);
    }



    {
        app->statusbarlabel = (Object *)NewObject( BUTTON_GetClass(),NULL,
                        GA_DrawInfo,(ULONG) app->drawInfo,
                        BUTTON_BevelStyle,BVS_NONE,
                        BUTTON_Transparent, TRUE,
						GA_ReadOnly, TRUE,
                        BUTTON_Justification, BCJ_CENTER,
                        GA_Text,(ULONG)"...",
                    TAG_END);

//         (Object *)NewObject( LABEL_GetClass(), NULL,
//                        LABEL_DrawInfo, app->drawInfo,
//                        //IA_Font, &helvetica15bu,
//                        //LABEL_SoftStyle, FSF_BOLD | FSF_ITALIC,
//                        LABEL_Justification, LABEL_CENTRE,
//                        LABEL_Text,(ULONG)"Values:",
//                    TAG_END);


        app->bottombarlayout =
             (Object *)NewObject( LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_EvenSize, TRUE,
                    LAYOUT_HorizAlignment, LALIGN_RIGHT,
                  //  CHILD_ScaleHeight,1, //%
                   // CHILD_MaxHeight,app->fontHeight,
                   // LAYOUT_SpaceInner, FALSE,
                    LAYOUT_AddChild, app->statusbarlabel,
                  //  LAYOUT_AddChild, app->labelValues,
                   // LAYOUT_AddChild, app->disablecheckbox,
                  //  GA_Height,app->fontHeight,
                    TAG_DONE);
        if(!app->bottombarlayout) cleanexit("Can't layout 2");
    }



    {
     //   struct DrawInfo *drinfo = GetScreenDrawInfo(screen);
        app->mainvlayout = (Object *)NewObject( LAYOUT_GetClass(), NULL,
            GA_DrawInfo, app->drawInfo,
            LAYOUT_DeferLayout, TRUE, // Layout refreshes done on task's context (by thewindow class)
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_BottomSpacing, 2,
            LAYOUT_TopSpacing,4,
            LAYOUT_LeftSpacing,2,
            LAYOUT_RightSpacing,2,
         //   LAYOUT_HorizAlignment, LALIGN_RIGHT,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_AddChild, app->horizontallayoutA,
                CHILD_WeightedHeight,0,
            LAYOUT_AddChild, app->horizontallayoutB,
                CHILD_WeightedHeight,4,
            LAYOUT_AddChild, app->horizontallayoutC,
                CHILD_WeightedHeight,0,
            LAYOUT_AddChild, app->bottombarlayout,
                CHILD_WeightedHeight,0,
            TAG_END);
        if (!app->mainvlayout) cleanexit("layout error 3");

        app->reportReq = NewObject(REQUESTER_GetClass(), NULL,
			// REQ_TitleText, "Project Generated",
			REQ_Image,REQIMAGE_INFO,
			REQ_BodyText,"....",
			REQ_GadgetText,(ULONG)"_Ok", //
            TAG_END);

    } //end if screen





    app->app_port = CreateMsgPort();

    /* Create the window object. */
    app->window_obj = (Object *)NewObject( WINDOW_GetClass(), NULL,
        WA_Left, 0,
        WA_Top, (ULONG)(app->lockedscreen->Font->ta_YSize) + 3,
        WA_CustomScreen, (ULONG) app->lockedscreen,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_RAWKEY ,
        WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_SIZEGADGET | WFLG_ACTIVATE | WFLG_SMART_REFRESH,
        WA_Title,(ULONG) "Boopsi Wizard",
        WINDOW_ParentGroup,(ULONG) app->mainvlayout,
        WINDOW_IconifyGadget, TRUE,
  //re      WINDOW_Icon,(ULONG) GetDiskObject("PROGDIR:ReAction"),
        WINDOW_IconTitle,(ULONG)  "Boopsi Wizard",
        WINDOW_AppPort, (ULONG)app->app_port,
    TAG_END);
    if(!app->window_obj) cleanexit("can't create window");

    /*  Open the window. */
    app->win = boopsi_OpenWindow(app->window_obj);
    if(!app->win) cleanexit("can't open window");


    updateUIToStates();
    // gui inited here.
    {
        char temp[64];
        snprintf(temp,63,"Found %d templates", getNbTemplates());
        guiNotifier(0,temp);
    }


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

            /* CA_HandleInput() returns the gadget ID of a clicked
             * gadget, or one of several pre-defined values.  For
             * this demo, we're only actually interested in a
             * close window and a couple of gadget clicks.
             */
            while ((result = DoMethod(app->window_obj, WM_HANDLEINPUT, /*code*/NULL)) != WMHI_LASTMSG)
            {
            // printf("result:%08x\n",(int)result);
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
                    {
                        ULONG gid = result &0xffff;
                       // printf("up gid:%d\n",gid);
                        if(gid>=GAD_START_SELECT_TEMPLATE)
                        {   // toggle button: which state ?

                            gid -= GAD_START_SELECT_TEMPLATE;
                            if(app->TemplateButtonsList[gid])
                            {
                                 int selected = 0;
                                GetAttr(GA_Selected, app->TemplateButtonsList[gid], &selected);
                                if(selected)  selectTemplate(gid);
                            }


                        } else if(gid == GAD_BUTTON_GENERATE)
                        {
                            generate();
                        } else if(gid == GAD_BUTTON_ABOUT)
                        {
                            openAboutReq();
                        }
                        break;
                    }
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

    // all close done in exitclose().
    return 0;
}

void guiNotifier(int loglevel, const char *log)
{
    if(!app || !app->statusbarlabel) return;

// todo errors in red/ warning in orange
//re    int textpen = -1; // default text pen
    SetGadgetAttrs((struct Gadget *)app->statusbarlabel,app->win,NULL,
    //    BUTTON_TextPen,(ULONG)textpen,
        GA_Text,(ULONG)log,
        TAG_END);



}

void exitclose(void)
{
    if(app)
    {
        if(app->TemplateButtonsList) FreeVec(app->TemplateButtonsList);
        /* Disposing of the window object will also close the
         * window if it is already opened and it will dispose of
         * all objects attached to it.
         */
        if(app->reportReq) DisposeObject( app->reportReq );
        if(app->window_obj) DisposeObject(app->window_obj);
        else {
//            // but if not attached because mid-init fail, has to be manual.
//            if(app->mainvlayout)  DisposeObject(app->mainvlayout);
//            else {
//                if(app->horizontallayout) DisposeObject(app->horizontallayout);
//                else {
//                    if(app->testbt) DisposeObject(app->testbt);
//                    if(app->kbdview) DisposeObject(app->kbdview);
//                }
//                if(app->bottombarlayout) DisposeObject(app->bottombarlayout);
//                else {
//                    if(app->label1) DisposeObject(app->label1);
//                    if(app->labelValues) DisposeObject(app->labelValues);
//                    if(app->disablecheckbox) DisposeObject(app->disablecheckbox);
//                }
//            }
        }

        if(app->drawInfo) FreeScreenDrawInfo(app->lockedscreen, app->drawInfo);
        if(app->lockedscreen) UnlockPubScreen(0, app->lockedscreen);

    }

    closeAppModel(); // thi is meant to close app implicitely, If i'm correct...
    if(RequesterBase) CloseLibrary(RequesterBase);
    if(TextFieldBase) CloseLibrary(TextFieldBase);
    if(StringBase) CloseLibrary(StringBase);
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
    if(AslBase) CloseLibrary(AslBase);

}
// synchronize greying buttons...
int CurrentTemplate = -1;
void updateUIToStates()
{
    if(!app) return;
// GA_Disabled
    ULONG generateBtDisabled = (CurrentTemplate<0);

   SetGadgetAttrs((struct Gadget *) app->btGenerate,app->win,NULL,
       GA_DISABLED,generateBtDisabled,
       TAG_END);
}

sWizTemplate *getTemplate(int i)
{
    sWizTemplate *ptmpl = getTemplates();
    while(ptmpl && i>0) { ptmpl = ptmpl->_pNext; i--; }
    return ptmpl;
}
void selectTemplate(int i)
{
    const char *pDescription;
    const char *pDefaultName;
    if(!app) return;

    // printf("selectTemplate:%d\n",i);
    if(CurrentTemplate == i) return;
    CurrentTemplate = i;

    sWizTemplate *ptmpl = getTemplate(i);

    pDescription = "...";
    pDefaultName = "ProjectName";
    if(ptmpl)
    {
        if(ptmpl->_comment) pDescription = ptmpl->_comment;
        if(ptmpl->_defaultname) pDefaultName = ptmpl->_defaultname;
    }

   SetGadgetAttrs((struct Gadget *)app->projectNameString,app->win,NULL,
       STRINGA_TextVal,(ULONG)pDefaultName,
       TAG_END);

   SetGadgetAttrs((struct Gadget *)app->projectDescription,app->win,NULL,
       GA_TEXTEDITOR_Contents,(ULONG)pDescription,
       TAG_END);

    updateUIToStates(); // greying

    // switch of other buttons,; yes it behaves like radio buttons, but that's my choice.
    for(int j=0;j<getNbTemplates();j++)
    {
        if(i == j) continue;
        Object *o = app->TemplateButtonsList[j];
        if(!o) continue;
        SetGadgetAttrs((struct Gadget *)o,app->win,NULL,
            GA_Selected,FALSE,TAG_END);
    }


}



void generate()
{
// printf("generate\n");
    if(!app) return;
    if(CurrentTemplate<0) return;
    sWizTemplate *ptmpl = getTemplate(CurrentTemplate);
    if(!ptmpl) return;
// printf("go\n");
    // asl requester
    struct FileRequester *request = AllocFileRequest();
    if(!request) return;

    int result =  AslRequestTags(request,
                        ASLFR_DrawersOnly,TRUE,
                        ASLFR_DoMultiSelect,FALSE,
                        ASLFR_TitleText,(ULONG)"Choose a directory where to extract project",
                        TAG_END);
    if(!result || request->fr_Drawer == NULL)
    {
       FreeAslRequest(request);
       guiNotifier(0,"Project Generation canceled.");
        return;
    }

    // if(request->fr_Drawer)
    // {
    //     printf("dr:%s\n",request->fr_Drawer);
    // }

    guiNotifier(0,"Generate project ...");

    {
        sGeneration sgen;
        sGenerationReport report={0};
        GetAttr(STRINGA_TextVal, app->projectNameString,(ULONG) &sgen.baseName);

        int res = extractTemplate(
                        ptmpl,
                        ptmpl->_archivename,
                        request->fr_Drawer,
                        &sgen,&report);
        // return  text in all cases, error or ok
        {
            const char *errt = extractTextError();
            if(errt) guiNotifier(((res !=0)?2:0),errt);            
        }
        if(res ==0 &&  report.destinationDir && app->reportReq)
        {
            SetAttrs(app->reportReq,REQ_TitleText,(ULONG)"Project Generated",TAG_END);

            // if ok, show a report requester
            char temp[512];
            snprintf(temp,511,"Project was generated in directory:\n%s\n"
                        "Everything was named accordingly.\n"
                        "You may open a shell there and type:\n"
                        " smake for sas-c, or make for gcc.\n"
                        "There is also a CMakeLists.txt for cross-compilation.\n"
                        "binaries will then be generated in build-xxx dirs.",report.destinationDir);
            SetAttrs(app->reportReq,REQ_BodyText,(ULONG)&temp[0],TAG_END);

            // SetGadgetAttrs((struct Gadget *)app->reportReq,app->win,NULL,
            //     REQ_BodyText,(ULONG)&temp[0],TAG_END);

            OpenRequester(app->reportReq,app->win);
        }
        CloseGenerationReport(&report);
    }

    FreeAslRequest(request);

    return ;
}

void openAboutReq()
{

    SetAttrs(app->reportReq,REQ_TitleText,(ULONG)"About...",TAG_END);
    // if ok, show a report requester
 static const char *p=
    "***Be warned***:\nThis wizard is not an official AmigaOS NDK project\nand will most likely stick to Beta stage forever.\n\n"
    "What it is, is: an OpenSource effort of individual developpers, open to participation\n"
    " at: https://github.com/krabobmkd/boopsiwizard\n"
    " The fact is, a lot of aspect of Amiga OS development are difficult to set up,\n"
    "  and setting a simple project for a library, class, gadget, datatype, commodity\n"
    "  project, for a given C compiler, is cryptic, and takes days if not more.\n"
    "  So you *may* gain some times with this.\n\n"
    "The templates code proposed here will try to be the more compliant possible with\n"
    " official Amiga guidelines, but may not be 100% compliant. You are loudly welcome\n"
    " to make any suggestion on the code at:\n"
    " https://github.com/krabobmkd/boopsiwizard/issues\n"
    " Your resources for coding Amiga OS3 should be:\n"
    " The Amiga Developer CD v2.1, forum https://developer.amigaos3.net/forum\n"
    " https://developer.amigaos3.net/article/13-recommended-reading-amiga-developer\n\n"
    "How does it work and How can I do a template ?\n"
    " Templates are just a json file with a corresponding zip file in templates dir.\n"
    " Each json describes what should be renamed. At generation, if your project name\n"
    " is \"MyProject\",In target files, BaseName will be MyProject,BASENAME MYPROJECT\n"
    " and basename myproject. File names and text contents are replaced.\n"
    " Adress file names case-wise, we allow linux cross-compilation.\n\n"
    "License of wizard itself is LGPL, which means you can fork it or embedd it in\n"
    " commercial projects. It uses cJson and zlib.\n"
    " Some templates have code parts from official Amiga examples, some not.\n"
    " If your compiler is GCC, from now on you should also install phxass, needed to\n"
    " assemble the C startups."
    "\n\n - krb, Nov.2025."
    ;
    SetAttrs(app->reportReq,REQ_BodyText,(ULONG)p,TAG_END);

    OpenRequester(app->reportReq,app->win);

}


