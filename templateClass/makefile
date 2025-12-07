# this is the makefile for all Amiga GCC2.x
# like ADE on aminet.
# Update your compiler os-include and os-lib directories with recent NDK3.2 include and lib,
# doing an overriding copy.
# use with command "make"

CC = gcc
BUILDDIR = build-gcc
BUILDLIBDIR = build-gcc/libobjs
BUILDEXEDIR = build-gcc/exeobjs

ASMFLAGS = I=include: 
CFLAGS = -m68000 -Os -noixemul

BINARIES = \
 $(BUILDDIR)/basename.class\
 $(BUILDDIR)/basenameTest\
 $(BUILDDIR)Static/basenameTest_static

HEADERS = class_basename.h class_basename_private.h  minialib.h bdbprintf.h

MODELCLASSOBJS = \
 $(BUILDLIBDIR)/classinit.s.o \
 $(BUILDLIBDIR)/class_basename_lib.c.o \
 $(BUILDLIBDIR)/class_basename_dispatch.c.o \
 $(BUILDLIBDIR)/class_basename_attribs.c.o

MODELCLASSOBJSSTATIC = \
 $(BUILDDIR)Static/class_basename_lib.c.o \
 $(BUILDDIR)Static/class_basename_dispatch.c.o \
 $(BUILDDIR)Static/class_basename_attribs.c.o \
 $(BUILDDIR)Static/bdbprintf.c.o


all: $(BINARIES)

# - - - - - - - - -

$(BUILDDIR):
	-makedir $(BUILDDIR)

$(BUILDLIBDIR): $(BUILDDIR)
	-makedir $(BUILDLIBDIR)

$(BUILDEXEDIR): $(BUILDDIR)
	-makedir $(BUILDEXEDIR)

$(BUILDDIR)Static:
	-makedir $(BUILDDIR)Static

# - - - - - - - - 

$(BUILDLIBDIR)/%.c.o: %.c $(BUILDLIBDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) -ffreestanding -nostartfiles

$(BUILDEXEDIR)/%.c.o: %.c $(BUILDEXEDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)Static/%.c.o: %.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) -DBASENAME_STATICLINK=1 -DUSE_DEBUG_BDBPRINT=1


$(BUILDLIBDIR)/%.s.o: %.s $(BUILDLIBDIR) $(HEADERS)
	# use phxass as assembler
	phxass $< TO $@ $(ASMFLAGS)
	#gcc assembler will not make it.
	#as $< -o $@ -m68000
# - - - - - - - - - for .class

$(BUILDDIR)/basename.class: $(MODELCLASSOBJS)
	$(CC) -o $@ $(MODELCLASSOBJS) $(CFLAGS) -ffreestanding -nostartfiles  -Wl,-e,__start

# - - - - - - - - - executable to test model class

$(BUILDDIR)/basenameTest: $(BUILDEXEDIR)/basenametest.c.o
	$(CC) -o $@ $(CFLAGS) $(BUILDEXEDIR)/basenametest.c.o  -s


# - - - - - - - - -  static test case executable

$(BUILDDIR)Static/basenameTest_static: $(BUILDDIR)Static $(BUILDDIR)Static/basenametest.c.o $(MODELCLASSOBJSSTATIC)
	$(CC) -o $@ $(CFLAGS) $(BUILDDIR)Static/basenametest.c.o $(MODELCLASSOBJSSTATIC)


# - - - - - - -
clean:
	-delete ALL FORCE $(BUILDDIR)
	-delete ALL FORCE $(BUILDDIR)Static
