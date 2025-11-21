#ifndef CLASS_BASENAME_H
#define CLASS_BASENAME_H
/**
 * Definitions for Class BaseName
 * (This is the public file that can be released when publishing just the .class file)
 */
#include <exec/types.h>
#include <intuition/classes.h>

#define VERSION_BASENAME 1
#define BaseName_SUPERCLASS_ID "modelclass"

#ifdef BASENAME_STATICLINK
    extern int BaseNameStaticInit();
    extern void BaseNameStaticClose();
    extern Class *BASENAME_GetClass();
#else
    // BaseName_CLASS_ID is the identifier for this class, when shared.
    // NewObject() can use either BASENAME_GetClass() or BaseName_CLASS_ID.
    #define BaseName_CLASS_ID "basename.class"

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

/**  Attributes defined by the class,
 * if there is a super class,
 * all its attribs should also be valid.
 */
// different classes may not use same base.
//DEVTODO: have another offset...
#define BASENAME_Dummy			(TAG_USER+0x04120000)

// some attrib in the class
#define	BASENAME_Value		(BASENAME_Dummy+1)

//#define	BASENAME_NextMember		(BASENAME_Dummy+2)

/** DEVTODO: adds attributes definitions here and
 * manage them in class_basename_attribs.c
 */
#endif
