#ifndef GADGETS_BASENAME_H
#define GADGETS_BASENAME_H
/**
 * Definitions for Gadget BaseName
 * (This is the public file that can be released when publishing just the .gadget)
 */
#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>
            #include <inline/macros.h>
#define VERSION_BASENAME 1
#define BaseName_SUPERCLASS_ID "gadgetclass"

#ifdef BASENAME_STATICLINK
    extern int BaseNameStaticInit();
    extern void BaseNameStaticClose();
    extern Class *BASENAME_GetClass();
#else
    // BaseName_CLASS_ID is the identifier for this class, when shared.
    // NewObject() can use either BASENAME_GetClass() or BaseName_CLASS_ID.
    #define BaseName_CLASS_ID "basename.gadget"

    // the following is to define function with implicit library call for BASENAME_GetClass().
    // note it should be in includes generated from a fd files.
    Class * __stdargs BASENAME_GetClass( void );
    // ... could have other functions here

    #ifndef _NO_INLINE
        # if defined(__GNUC__)
            #include <inline/macros.h>
            #define BASENAME_GetClass() LP0(0x1e, Class *, BASENAME_GetClass ,, BaseNameBase)
            // ... could have other functions here
        # endif
        #if defined(LATTICE) || defined(__SASC) || defined(_DCC)
           #pragma libcall BaseNameBase BASENAME_GetClass 1e 00
            // ... could have other functions here
        # endif
        #if defined(__VBCC__)
            Class * __BASENAME_GetClass(__reg("a6") void *)="\tjsr\t-$1e(a6)";
            #define BASENAME_GetClass() __BASENAME_GetClass(BaseNameBase)
			// ... could have other functions here			
        #endif
    #endif /* _NO_INLINE */

    #ifndef __NOLIBBASE__
      extern struct Library *
        # ifdef __CONSTLIBBASEDECL__
       __CONSTLIBBASEDECL__
        # endif /* __CONSTLIBBASEDECL__ */
      BaseNameBase;
    #endif /* !__NOLIBBASE__ */

// end if dynamic link
#endif

/**  Attributes defined by the gadget class,
 * all attribs from gadgetclass.h are also valid.
 */
// different classes may not use same base.
//DEVTODO: have another offset for your new class to not collide super class ones and optimize...
#define BASENAME_Dummy			(TAG_USER+0x04110000)

// abstract coordinate from 0 to 65535 , whatever width is.
#define	BASENAME_CenterX		(BASENAME_Dummy+1)
// abstract coordinate from 0 to 65535, whatever height is.
#define	BASENAME_CenterY		(BASENAME_Dummy+2)

/** DEVTODO: adds attributes definitions here and
 * manage them in class_basename_attribs.c
 */
#endif
