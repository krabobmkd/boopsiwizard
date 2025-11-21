/*
 This is a C version of a binary header for .class and .gadget,
 this must be linked as the startup code.
 There is also an equivalent assembler version named classinit.s,
 but this would need an assembler (phxass, devpac genam, sasc asm, gcc as, ...)
*/
#include "compilers.h"
#include "class_basename_private.h"

// called by shared Lib init to create class.
extern int ASM CreateClass(REG(a6,struct ExtClassLib *LibBase));

// called by shared Lib expunge to dispose class.
extern void ASM DestroyClass(REG(a6,struct ExtClassLib *LibBase));

// boopsi class are like libs with first function being this:
extern Class *BASENAME_GetClass();

extern const char *Class_ID;
extern const char *VersionString;


// First executable location, must return an error to the caller
static int ASM _start() { return -1; }
