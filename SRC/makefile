#
# Makefile for Scheme version 1.1a
#	written by Jason Coughlin
#
# Make system mods in machine.h
#
# Note the order of the next line.  Placing microcode and eval before the
#	higher levels saves compiling time when you're developing the
#	lower layers.
OBJS =	scheme.obj mc_io.obj micro.obj eval.obj mc_math.obj globals.obj \
	scanner.obj memory.obj ops.obj symstr.obj forms.obj preds.obj \
	compile.obj

ERROR = debug.h error.h

# make rules for Turbo C's stupid make
.c.obj:
     $(CC) $(CFLAGS) -c $<

# this is where the actual makefile begins
CC = tcc
LINKER = tlink
CFLAGS = -v
LDFLAGS = /d

all: scheme.exe

scheme.exe: $(OBJS)
	$(LINKER) $(LDFLAGS) @scheme.lnk

scheme.obj: scheme.c glo.h scanner.h memory.h micro.h symstr.h eval.h \
	compile.h $(ERROR)

globals.obj: globals.c glo.h memory.h micro.h symstr.h eval.h predefs.h \
	ops.h forms.h preds.h compile.h

micro.obj: micro.c micro.h glo.h predefs.h memory.h scanner.h symstr.h $(ERROR)

mc_math.obj: mc_math.c micro.h glo.h memory.h symstr.h predefs.h $(ERROR)

mc_io.obj: mc_io.c micro.h glo.h memory.h scanner.h symstr.h predefs.h \
	$(ERROR)

scanner.obj: scanner.c glo.h scanner.h $(ERROR)

memory.obj: memory.c glo.h memory.h micro.h $(ERROR)

ops.obj: ops.c ops.h glo.h micro.h compile.h predefs.h eval.h $(ERROR)

compile.obj: compile.c compile.h glo.h symstr.h memory.h micro.h \
	predefs.h $(ERROR)

eval.obj: eval.c eval.h ops.h glo.h micro.h predefs.h forms.h preds.h $(ERROR)

preds.obj: preds.c preds.h glo.h micro.h eval.h $(ERROR)

forms.obj: forms.c forms.h glo.h micro.h predefs.h eval.h $(ERROR)

symstr.obj: symstr.c glo.h symstr.h micro.h $(ERROR)

testscan: testscan.exe

testscan.exe: testscan.obj scanner.obj
	$(CC) testscan.obj scanner.obj

test: scheme.exe
	test

clean:
	rm /f $(OBJS)
