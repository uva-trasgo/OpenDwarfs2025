#
# Copyright 2010 by Virginia Polytechnic Institute and State
# University. All rights reserved. Virginia Polytechnic Institute and
# State University (Virginia Tech) owns the software and its
# associated documentation.
#

bin_PROGRAMS += bfs
bin_PROGRAMS += createbfs
bin_PROGRAMS += createbfs_rodinia

bfs_SOURCES = graph-traversal/bfs/bfs.cpp
createbfs_SOURCES = graph-traversal/bfs/createbfs.c
createbfs_rodinia_SOURCES = graph-traversal/bfs/graphgen.cpp

#LIBS += -O0 -g

all_local += bfs-all-local
exec_local += bfs-exec-local

bfs-all-local:
	cp $(top_srcdir)/graph-traversal/bfs/bfs_kernel.cl .
	cp $(top_srcdir)/graph-traversal/bfs/bfs_fpga.cl .

bfs-exec-local:
	cp $(top_srcdir)/graph-traversal/bfs/bfs_kernel.cl ${DESTDIR}${bindir}
	cp $(top_srcdir)/graph-traversal/bfs/bfs_fpga.cl ${DESTDIR}${bindir}

# New: Compilation targets for FPGA Emulation if --enable-aot-emulation is passed
if BUILD_AOT_EMULATION
all_local += bfs_kernel_opt_fpga.aocx bfs_kernel.aocx
exec_local += dwarf-bfs-fpga-exec-local

bfs_kernel.aocx: $(top_srcdir)/graph-traversal/bfs/bfs_kernel.cl
	$(AOC_COMPILER) -march=emulator $< -o $@

bfs_kernel_opt_fpga.aocx: $(top_srcdir)/graph-traversal/bfs/bfs_fpga.cl
	$(AOC_COMPILER) -march=emulator $< -o $@

dwarf-bfs-fpga-exec-local:
	cp bfs_kernel.aocx ${DESTDIR}${bindir}
	cp bfs_kernel_opt_fpga.aocx ${DESTDIR}${bindir}
endif