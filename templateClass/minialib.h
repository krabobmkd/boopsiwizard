#ifndef _MINIALIB_H_
#define _MINIALIB_H_
/** short inline replacement for proto/alib.h (amiga tool static lib)
 * BOOPSI needs alib for DoMethod()/SetSuperAttrs()/DoSuperMethod() / ...
 * To be used when linking .class and .gadget files, but without C startup.
 * some linker will not make alib functions work without a real c startup,
 * and libraries .class and .gadgets doesn't have a real c startup.
 * in all cases this is shorter and equivalent to those alib calls, so not a bad idea I guess.
 * -krb, license is LGPL.
 */

#include <proto/utility.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <utility/hooks.h>

// original alib methods used by boopsi are...
// ULONG  __stdargs CallHookA( struct Hook *hookPtr, Object *obj, APTR message );
// ULONG  __stdargs CallHook( struct Hook *hookPtr, Object *obj, ... );
// ULONG  __stdargs DoMethodA( Object *obj, Msg message );
// ULONG  __stdargs DoMethod( Object *obj, ULONG methodID, ... );
// ULONG  __stdargs DoSuperMethodA( struct IClass *cl, Object *obj, Msg message );
// ULONG  __stdargs DoSuperMethod( struct IClass *cl, Object *obj, ULONG methodID, ... );
// ULONG  __stdargs CoerceMethodA( struct IClass *cl, Object *obj, Msg message );
// ULONG  __stdargs CoerceMethod( struct IClass *cl, Object *obj, ULONG methodID, ... );
// ULONG  __stdargs HookEntry( struct Hook *hookPtr, Object *obj, APTR message );
// ULONG  __stdargs SetSuperAttrs( struct IClass *cl, Object *obj, ULONG tag1, ... );


#ifdef __SASC
#define AINLINE static __inline
#else
#define AINLINE static inline
#endif

// CallHookPkt is in utility...
extern struct Library *UtilityBase;

AINLINE ULONG DoMethodA( Object *obj, Msg message ) {
    //   if (!obj || !message) return 0L;
   return CallHookPkt((struct Hook *) OCLASS(obj), obj, message);
}

AINLINE ULONG DoMethod( Object *obj, ULONG methodID, ... ) {
    //   if (!obj) return 0L;
   return CallHookPkt((struct Hook *) OCLASS(obj), obj, (APTR)&methodID);
}
AINLINE ULONG DoSuperMethodA( struct IClass *cl, Object *obj, Msg message ) {
   return CallHookPkt((struct Hook *)cl->cl_Super, obj, message);
}
AINLINE ULONG DoSuperMethod( struct IClass *cl, Object *obj, ULONG methodID, ... ) {
   return CallHookPkt((struct Hook *)cl->cl_Super, obj, (APTR)&methodID);
}
// assume cl is class of obj
AINLINE ULONG SetSuperAttrs( struct IClass *cl, Object *obj, ULONG tag1, ... ) {
    struct opSet ops, *msg = &ops;

    ops.MethodID     = OM_SET;
    ops.ops_AttrList = ( struct TagItem	*)&tag1;
    ops.ops_GInfo    = NULL;

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/*Boopsi support function that invokes the supplied message on the specified object,
 *  as though it were the specified class.
 */
 AINLINE ULONG CoerceMethodA( struct IClass *cl, Object *obj, Msg message )
 {
    if(!cl || !obj) return NULL;
    return CallHookPkt((struct Hook *) cl, obj, message);
 }
 AINLINE ULONG CoerceMethod( struct IClass *cl, Object *obj, ULONG methodID, ... )
 {
    if(!cl || !obj) return NULL;
    return CallHookPkt((struct Hook *)cl, obj, (APTR)&methodID);
 }

#endif
