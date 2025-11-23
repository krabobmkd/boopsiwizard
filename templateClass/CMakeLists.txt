cmake_minimum_required (VERSION 3.1)

# VASM is an extra asm language configured in our cmake platform,
# to cross-assemble with vasm assembler installed by bebbo gcc
# you may boostrap with cmake platform in amigacommonlibs with:
# mkdir build
# cd build
# cmake ..  -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../amigacommonlibs/cmake/Modules/Platform/m68k-amigaos.cmake
# compile with:
# cmake --build .

enable_language( VASM )
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)
project(BaseNameClassProject LANGUAGES C CXX VERSION 1.0)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(VERSTRING "1.0")

add_executable(BaseNameTest_static
		compilers.h
		basenametest.c
		class_basename.h
		class_basename_private.h
		class_basename_lib.c
		class_basename_dispatch.c
		class_basename_attribs.c
		)

target_compile_options(BaseNameTest_static PRIVATE -s -save-temps -Os -m68000)
target_compile_definitions(BaseNameTest_static PRIVATE BASENAME_STATICLINK=1 )
target_link_options(BaseNameTest_static PRIVATE -s )
install(TARGETS BaseNameTest_static DESTINATION "${CMAKE_SOURCE_DIR}/..")

# binary class version -- should be a "add_library( SHARED..."
add_executable(basename.class
		compilers.h
		minialib.h
		class_basename.h
		class_basename_private.h
		classinit.s
		class_basename_lib.c
		class_basename_dispatch.c
		class_basename_attribs.c
		)


# important: gcc6.5 in 68000 modes doesn't manage (.w * .w)=.l mulu.w , it wants its emulated "muls.l" lib link for this.
# (does not happen with 68020 which as native muls.l)
#  so with need libc to link this muls, also it links alib things it seems (??) which is good because
# boopsi use a lot of DoXXXMethod() , but if a real std call is made: boom.
# because the self made library startup will not support std inits and close.
# so -nostdlib is removed to have a bit of functions linked, but it's dangerous.

target_link_options(basename.class PRIVATE -ffreestanding -nostartfiles -Wl,-e,__start)
target_compile_options(basename.class PRIVATE -Os -m68000 -fomit-frame-pointer )
target_compile_definitions(basename.class PRIVATE PRIVATE VERSION_STRING="${VERSTRING}")


# exe that consume the .class
add_executable(BaseNameTest
		compilers.h
		basenametest.c
		class_basename.h
		)

target_compile_options(BaseNameTest PRIVATE -Os -save-temps -m68000 )
target_link_options(BaseNameTest PRIVATE -s )
install(TARGETS BaseNameTest DESTINATION "${CMAKE_SOURCE_DIR}/..")
