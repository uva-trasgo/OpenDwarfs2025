#
# Copyright 2010 by Virginia Polytechnic Institute and State
# University. All rights reserved. Virginia Polytechnic Institute and
# State University (Virginia Tech) owns the software and its
# associated documentation.
#

bin_PROGRAMS += nqueens

nqueens_SOURCES = branch-and-bound/nqueens/main.cpp branch-and-bound/nqueens/nqueen_cl.cpp branch-and-bound/nqueens/nqueen_cpu.cpp

all_local += nqueens-all-local
exec_local += nqueens-exec-local

nqueens-all-local:
	cp $(top_srcdir)/branch-and-bound/nqueens/kernels_nqueens.cl .

nqueens-exec-local:
	cp $(top_srcdir)/branch-and-bound/nqueens/kernels_nqueens.cl ${DESTDIR}${bindir}

# New: Compilation targets for FPGA Emulation if --enable-aot-emulation is passed
if BUILD_AOT_EMULATION
all_local += kernels_nqueens.aocx
exec_local += dwarf-nqueens-fpga-exec-local

kernels_nqueens.aocx: $(top_srcdir)/branch-and-bound/nqueens/kernels_nqueens.cl
	$(AOC_COMPILER) -g0 -march=emulator $< -o $@

dwarf-nqueens-fpga-exec-local:
	cp kernels_nqueens.aocx ${DESTDIR}${bindir}
endif
