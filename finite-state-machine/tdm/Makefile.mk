#
# Copyright 2010 by Virginia Polytechnic Institute and State
# University. All rights reserved. Virginia Polytechnic Institute and
# State University (Virginia Tech) owns the software and its
# associated documentation.
#

bin_PROGRAMS += tdm

tdm_SOURCES = finite-state-machine/tdm/tdm_ocl.cpp finite-state-machine/tdm/dataio.cpp

all_local += tdm-all-local
exec_local += tdm-exec-local

tdm-all-local:
	cp $(top_srcdir)/finite-state-machine/tdm/tdm_ocl_kernel.cl .
	cp $(top_srcdir)/finite-state-machine/tdm/types.h .

tdm-exec-local:
	cp $(top_srcdir)/finite-state-machine/tdm/tdm_ocl_kernel.cl ${DESTDIR}${bindir}
	cp $(top_srcdir)/finite-state-machine/tdm/types.h ${DESTDIR}${bindir}

# New: Compilation targets for FPGA Emulation if --enable-aot-emulation is passed
if BUILD_AOT_EMULATION
all_local += tdm_ocl_kernel.aocx
exec_local += dwarf-tdm-fpga-exec-local

tdm_ocl_kernel.aocx: $(top_srcdir)/finite-state-machine/tdm/tdm_ocl_kernel.cl
	$(AOC_COMPILER) -march=emulator $< -o $@

dwarf-tdm-fpga-exec-local:
	cp tdm_ocl_kernel.aocx ${DESTDIR}${bindir}
endif