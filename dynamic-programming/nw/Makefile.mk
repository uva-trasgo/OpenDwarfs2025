#
# Copyright 2010 by Virginia Polytechnic Institute and State
# University. All rights reserved. Virginia Polytechnic Institute and
# State University (Virginia Tech) owns the software and its
# associated documentation.
#

bin_PROGRAMS += needle

needle_SOURCES = dynamic-programming/nw/needle.c

all_local += nw-all-local
exec_local += nw-exec-local

nw-all-local:
	cp $(top_srcdir)/dynamic-programming/nw/needle_kernel.cl .
	cp $(top_srcdir)/dynamic-programming/nw/needle_kernel_opt_gpu.cl .
	cp $(top_srcdir)/dynamic-programming/nw/needle_fpga.cl .

nw-exec-local:
	cp $(top_srcdir)/dynamic-programming/nw/needle_kernel.cl ${DESTDIR}${bindir}
	cp $(top_srcdir)/dynamic-programming/nw/needle_kernel_opt_gpu.cl ${DESTDIR}${bindir}
	cp $(top_srcdir)/dynamic-programming/nw/needle_fpga.cl ${DESTDIR}${bindir}

# New: Compilation targets for FPGA Emulation if --enable-aot-emulation is passed
if BUILD_AOT_EMULATION
all_local += needle_kernel_opt_fpga.aocx needle_kernel.aocx
exec_local += dwarf-needle-fpga-exec-local

needle_kernel.aocx: $(top_srcdir)/dynamic-programming/nw/needle_kernel.cl
	$(AOC_COMPILER) -march=emulator $< -o $@

needle_kernel_opt_fpga.aocx: $(top_srcdir)/dynamic-programming/nw/needle_fpga.cl
	$(AOC_COMPILER) -march=emulator $< -o $@

dwarf-needle-fpga-exec-local:
	cp needle_kernel.aocx ${DESTDIR}${bindir}
	cp needle_kernel_opt_fpga.aocx ${DESTDIR}${bindir}
endif