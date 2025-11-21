
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/layers.h>

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
#include <utility/tagitem.h>

#include "class_basename.h"
#include "class_basename_private.h"

#ifdef USE_BEVEL_FRAME
    #include <proto/bevel.h>
    #include <images/bevel.h>
#endif



/* The GM_DOMAIN method is used to obtain the sizing requirements of an
 * object for a class before ever creating an object. */

/* GM_DOMAIN */
//struct gpDomain
//{
//    ULONG		 MethodID;
//    struct GadgetInfo	*gpd_GInfo;
//    struct RastPort	*gpd_RPort;	/* RastPort to layout for */
//    LONG		 gpd_Which;
//    struct IBox		 gpd_Domain;	/* Resulting domain */
//    struct TagItem	*gpd_Attrs;	/* Additional attributes */
//};

ULONG BaseName_Domain(Class *C, struct Gadget *Gad, struct gpDomain *D)
{
  BaseName *gdata=0;

  if(Gad) gdata=INST_DATA(C, Gad);
// Printf("BaseName_Domain data:%lx\n",(int)gdata);

  D->gpd_Domain.Left=0;
  D->gpd_Domain.Top=0;

  switch(D->gpd_Which)
  {
    case GDOMAIN_NOMINAL:
     // if(gdata)
     // {
     //   D->gpd_Domain.Width =gdata->_minimalWidth;
     //   D->gpd_Domain.Height=gdata->_minimalHeight;
     // }
     // else
      {
        D->gpd_Domain.Width=100;
        D->gpd_Domain.Height=50;
      }
      break;

    case GDOMAIN_MAXIMUM:
      D->gpd_Domain.Width=16000;
      D->gpd_Domain.Height=16000;
      break;

    case GDOMAIN_MINIMUM:
    default:
     if(gdata)
     {
       D->gpd_Domain.Width =gdata->_minimalWidth; // sqrt(gdata->Pens) * 8 + 8;
       D->gpd_Domain.Height=gdata->_minimalHeight; // sqrt(gdata->Pens) * 8 + 8;
     }
     else
      {
        D->gpd_Domain.Width=  50;
        D->gpd_Domain.Height= 50;
      }
      break;

  }
  return(1);
}

/**
 * method GM_LAYOUT
 * The gadget knows its final coordinates,
 * So we may have to resize what's inside our gadget.
 */
ULONG BaseName_Layout(Class *C, struct Gadget *Gad, struct gpLayout *layout)
{
  BaseName *gdata;
  LONG topedge,leftedge,width,height;

    gdata=INST_DATA(C, Gad);

    topedge = Gad->TopEdge;
    leftedge = Gad->LeftEdge;
    width = Gad->Width;
    height = Gad->Height;

#ifdef USE_BEVEL_FRAME
    if(gdata->Bevel)
    {   // all other attribs that doesnt change are set at NewObject()
        SetAttrs((Object *)gdata->Bevel,
            IA_Left, leftedge,
            IA_Top,        topedge,
            IA_Width,      width,
            IA_Height,     height,
            BEVEL_ColorMap,(ULONG)layout->gpl_GInfo->gi_Screen->ViewPort.ColorMap,
            BEVEL_Transparent,TRUE, // we will draw iside the frame ourselve.
            BEVEL_Style,BVS_BUTTON,
            TAG_DONE);
        // consider the effective rectangle is inside the frame.
        GetAttr(BEVEL_InnerTop,     gdata->Bevel,(ULONG *) &topedge);
        GetAttr(BEVEL_InnerLeft,    gdata->Bevel,(ULONG *) &leftedge);
        GetAttr(BEVEL_InnerWidth,   gdata->Bevel,(ULONG *) &width);
        GetAttr(BEVEL_InnerHeight,  gdata->Bevel,(ULONG *) &height);
    }
#endif
    gdata->_framerec.MinX = leftedge;
    gdata->_framerec.MinY = topedge;
    gdata->_framerec.MaxX = leftedge + width  -1;
    gdata->_framerec.MaxY = topedge  + height -1;

#ifdef USE_REGION_CLIPPING

        ClearRegion(gdata->_clipRegion);
        OrRectRegion(gdata->_clipRegion, &gdata->_framerec);

#endif

  return(1);
}


/* draw yourself, in the appropriate state */
ULONG BaseName_Render(Class *C, struct Gadget *Gad, struct gpRender *Render, ULONG update)
{
  BaseName *gdata;
  struct RastPort *rp; 
  ULONG retval=1;

  gdata=INST_DATA(C, Gad);

  if(Render->MethodID==GM_RENDER)
  {
    rp=Render->gpr_RPort;
    update=Render->gpr_Redraw;
  }
  else
  {
    rp = ObtainGIRPort(Render->gpr_GInfo);
  }

  if(rp)
  {
    int penbg=1,penb=2,penc=3;
    struct Region *oldClipRegion;

    if(Gad->Flags & GFLG_DISABLED) // if disabled, draw background with another color.
    {
        penbg = 0;
    }
    #ifdef USE_BEVEL_FRAME
        if(gdata->Bevel) DrawImage(rp,gdata->Bevel,0,0);
    #endif

    #ifdef USE_REGION_CLIPPING
        oldClipRegion = InstallClipRegion( rp->Layer, gdata->_clipRegion);
    #endif

      SetDrMd(rp,JAM1);
      SetAPen(rp,penbg);
      RectFill(rp,gdata->_framerec.MinX,
                  gdata->_framerec.MinY,
                  gdata->_framerec.MaxX,
                  gdata->_framerec.MaxY) ;
        {
            UWORD width = gdata->_framerec.MaxX - gdata->_framerec.MinX;
            UWORD height = gdata->_framerec.MaxY - gdata->_framerec.MinY;

            UWORD xc = gdata->_framerec.MinX + ((width*gdata->_circleCenterX)>>16);
            UWORD yc = gdata->_framerec.MinY + ((height*gdata->_circleCenterY)>>16);
            SetAPen(rp,penb);
            DrawEllipse(rp,xc,yc,width>>1,height>>1);
            SetAPen(rp,penc);
            DrawEllipse(rp,xc,yc,width>>2,height>>2);
        }
    #ifdef USE_REGION_CLIPPING
        InstallClipRegion( rp->Layer,oldClipRegion); // important to pass NULL if oldClipRegion is NULL.
    #endif

    if (Render->MethodID != GM_RENDER)
      ReleaseGIRPort(rp);
  }
  return(retval);
}

//void i_RenderColorBox(Class *C, struct Gadget *Gad, struct GadgetInfo *gi, struct RastPort *rp, ULONG Pen)
//{
//  struct DrawInfo *di;
//  struct GadData *gdata;
//  ULONG row,col,
//          left,top,
//          width,height,
//          bottom,right;

//  gdata=INST_DATA(C, Gad);

//  di=gi->gi_DrInfo;

//  col=Pen % gdata->Cols;
//  row=Pen / gdata->Cols;

//  left    =gdata->Col[col];
//  right   =gdata->Col[col+1]-1;
//  width   =right-left;
//  top     =gdata->Row[row];
//  bottom  =gdata->Row[row+1]-1;
//  height  =bottom-top;

//#define SIZE (0)


//  if(Pen==gdata->ActivePen && (((Gad->Flags & GFLG_SELECTED) && gdata->MouseMode) || gdata->ShowSelected))
//  {
//    if(gdata->EditMode)
//    {
////      SetDrPt(rp,0x0f0f);

///*      SetAPen(rp, di->dri_Pens[BACKGROUNDPEN]);

//      Move(rp,left,   bottom);
//      Draw(rp,left,   top);
//      Draw(rp,right,  top);
//      Draw(rp,right,  bottom);
//      Draw(rp,left,   bottom);

//      Move(rp,left+1,   bottom-1);
//      Draw(rp,left+1,   top+1);
//      Draw(rp,right-1,  top+1);
//      Draw(rp,right-1,  bottom-1);
//      Draw(rp,left+1,   bottom-1);
//*/
//      SetDrPt(rp,0xF0F0);
//    }

//    SetDrMd(rp,JAM2);

//    SetBPen(rp, di->dri_Pens[BACKGROUNDPEN]);

//    SetAPen(rp, di->dri_Pens[SHADOWPEN]);
//    Move(rp,left,bottom);
//    Draw(rp,left,top);
//    Draw(rp,right,top);

//    SetAPen(rp, di->dri_Pens[SHINEPEN]);
//    Draw(rp,right,bottom);
//    Draw(rp,left,bottom);

//    SetAPen(rp, di->dri_Pens[SHADOWPEN]);
//    Move(rp,left+1,bottom-1);
//    Draw(rp,left+1,top+1);
//    Draw(rp,right-1,top+1);

//    SetAPen(rp, di->dri_Pens[SHINEPEN]);
//    Draw(rp,right-1,  bottom-1);
//    Draw(rp,left+1,   bottom-1);

//    SetDrPt(rp,0xFfff);

//    SetAPen(rp, di->dri_Pens[BACKGROUNDPEN]);
//    Move(rp,left+2,bottom-2);
//    Draw(rp,left+2,top+2);
//    Draw(rp,right-2,top+2);
//    Draw(rp,right-2,  bottom-2);
//    Draw(rp,left+2,   bottom-2);



//    top+=3;
//    left+=3;
//    right-=3;
//    bottom-=3;
//  }
//  else
//  {
//    SetAPen(rp, di->dri_Pens[BACKGROUNDPEN]);
//    Move(rp,left,bottom);
//    Draw(rp,left,top);
//    Draw(rp,right,top);
//    Draw(rp,right,bottom);
//    Draw(rp,left,bottom);
///*

//    Move(rp,left+1,bottom-1);
//    Draw(rp,left+1,top+1);
//    Draw(rp,right-1,top+1);
//    Draw(rp,right-1,  bottom-1);
//    Draw(rp,left+1,   bottom-1);*/

//    top+=1;
//    left+=1;
//    right-=1;
//    bottom-=1;
//  }

//  width   =right-left+1;
//  height  =bottom-top+1;

//  if(CyberGfxBase && GetBitMapAttr(gi->gi_Screen->RastPort.BitMap, BMA_DEPTH )>8)
//  {
//    ULONG argb;

//    argb= ((gdata->Palette[Pen].R & 0xff000000) >> 8) |
//          ((gdata->Palette[Pen].G & 0xff000000) >> 16) |
//          ((gdata->Palette[Pen].B & 0xff000000) >> 24);


//    FillPixelArray(rp, left, top, width, height, argb);
//  }
//  else
//  {
//    ULONG p;
//    p=FindColor(gi->gi_Screen->ViewPort.ColorMap,
//                        gdata->Palette[Pen].R,
//                        gdata->Palette[Pen].G,
//                        gdata->Palette[Pen].B,
//                        -1);

//    SetAPen(rp, p);
//    RectFill(rp,left, top, right, bottom);
//  }

//  if(gdata->Disabled)
//  {
//    gui_GhostRect(rp, gi->gi_DrInfo->dri_Pens[TEXTPEN], left, top, right, bottom);
//  }

//  /*
//  SetAttrs(gdata->Pattern,PAT_RastPort,    rp,
//                          PAT_DitherAmt,   gdata->ActivePen * 256,
//                          TAG_DONE);
//*/

//}


