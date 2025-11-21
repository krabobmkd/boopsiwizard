#ifndef COMPILERS_H_
#define COMPILERS_H_
/** control assembler parameters for functions signatures.
 * compilers specify this differenly.
 * Tested so far with SASC 6.5, GCC 2.95 Amiga, GCC 6.5 "Bebbo"
 */
#if defined(__VBCC__)
    #define ASM __asm
    #define INLINE static __inline
    #define REG(reg,arg) __reg(#reg) arg
    #define SAVEDS __saveds
#elif defined(__SASC)
    #define ASM __asm
    #define SAVEDS __saveds
    #define REG(reg,arg) register __##reg arg
    #define INLINE static __inline
#elif defined(__GNUC__)
    // works for amiga gcc 2.95 (1999) and bebbo gcc6.5 (2024).
    //#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 0)
    #define ASM
    #define SAVEDS
    #define INLINE static inline
    #define REG(reg,arg) arg __asm(#reg)
#else
    #define ASM
    #define SAVEDS
    #define REG(reg,arg) arg
    #define INLINE static inline
#endif


#endif // COMPILERS_H_
