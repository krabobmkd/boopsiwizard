cmake_minimum_required (VERSION 3.1)

# VASM is an extra asm language configured in our cmake platform,
# to cross-assemble with vasm assembler installed by bebbo gcc
set(CMAKE_C_STANDARD 11)
project(BoopsiWizard LANGUAGES C VERSION 1.0)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(VERSTRING "1.0")

set(COMMONLIBS "../../amigacommonlibs")

set(ZLIBPATH "${COMMONLIBS}/zlib")
set(MINIZIPLIBPATH "${COMMONLIBS}/zlib/contrib/minizip")
add_subdirectory(${ZLIBPATH} zlib)
# let's have 68000 and soft float using libs/ieeemath libs !
# let's go "no float at all" with modified json lib....
# set( AMIGACOMPOPTS -Os -save-temps -m68000 -msoft-float  )
set( AMIGACOMPOPTS -Os -save-temps -m68000 )

add_library(cjson STATIC
				cjson/cJSON.c
				cjson/cJSON.h
			)
target_compile_options(cjson PRIVATE ${AMIGACOMPOPTS} )

# exe that consume the .class
add_executable(BoopsiWizard
		compilers.h
		boopsiwizard.c
		templates.h
		templates.c
		template_extract.c

		datatypebm.c
		datatypebm.h
		bpwizard_png.c

		${ZLIBPATH}/contrib/minizip/unzip.c
		${ZLIBPATH}/contrib/minizip/unzip.h
		${ZLIBPATH}/contrib/minizip/ioapi.c
		${ZLIBPATH}/contrib/minizip/ioapi.h
		)

# minizip
target_compile_options(BoopsiWizard PRIVATE ${AMIGACOMPOPTS} )
target_link_options(BoopsiWizard PRIVATE -s )
target_include_directories(BoopsiWizard PUBLIC ${ZLIBPATH} ${MINIZIPLIBPATH} cjson )
target_link_libraries(BoopsiWizard PUBLIC z cjson  )
# IOAPI_NO_64 is for minizip/ioapi.c
# sources that doesn't support -O2 are moved to MameXAmigaO0 lib
#target_compile_options(BoopsiWizard PRIVATE
#		$<$<CONFIG:RELEASE>:-Os -Wno-narrowing -DNDEBUG>
#		 ${AMIGA_COMPILER_OPTIONS} )


install(TARGETS BoopsiWizard DESTINATION "${CMAKE_SOURCE_DIR}/..")

add_executable(convertbin convertbin.c )
