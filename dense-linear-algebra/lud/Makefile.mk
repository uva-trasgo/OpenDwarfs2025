#
# Copyright 2010 by Virginia Polytechnic Institute and State
# University. All rights reserved. Virginia Polytechnic Institute and
# State University (Virginia Tech) owns the software and its
# associated documentation.
#

bin_PROGRAMS += lud
bin_PROGRAMS += createlud

LIBS += -lm -fopenmp

lud_SOURCES = dense-linear-algebra/lud/lud.c dense-linear-algebra/lud/common.c
lud_CFLAGS = -g3 -O0

createlud_SOURCES = dense-linear-algebra/lud/createlud.c 

all_local += lud-all-local
exec_local += lud-exec-local

lud-all-local:
	cp $(top_srcdir)/dense-linear-algebra/lud/lud_kernel.cl .
	cp $(top_srcdir)/dense-linear-algebra/lud/lud_kernel_opt_gpu.cl .

lud-exec-local:
	cp $(top_srcdir)/dense-linear-algebra/lud/lud_kernel.cl ${DESTDIR}${bindir}
	cp $(top_srcdir)/dense-linear-algebra/lud/lud_kernel_opt_gpu.cl ${DESTDIR}${bindir}

# New: Compilation targets for FPGA Emulation if --enable-aot-emulation is passed
if BUILD_AOT_EMULATION
all_local += lud_kernel.aocx
exec_local += dwarf-lud-fpga-exec-local

lud_kernel.aocx: $(top_srcdir)/dense-linear-algebra/lud/lud_kernel.cl
	$(AOC_COMPILER) -march=emulator $< -o $@

dwarf-lud-fpga-exec-local:
	cp lud_kernel.aocx ${DESTDIR}${bindir}
endif