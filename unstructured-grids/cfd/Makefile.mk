#
# Copyright 2010 by Virginia Polytechnic Institute and State
# University. All rights reserved. Virginia Polytechnic Institute and
# State University (Virginia Tech) owns the software and its
# associated documentation.
#

bin_PROGRAMS += cfd 

cfd_SOURCES = unstructured-grids/cfd/cfd.cpp

all_local += dwarf-cfd-all-local
exec_local += dwarf-cfd-exec-local

dwarf-cfd-all-local:
	cp $(top_srcdir)/unstructured-grids/cfd/cfd_kernel.cl .

dwarf-cfd-exec-local:
	cp $(top_srcdir)/unstructured-grids/cfd/cfd_kernel.cl ${DESTDIR}${bindir}

# New: Compilation targets for FPGA Emulation if --enable-aot-emulation is passed
if BUILD_AOT_EMULATION
all_local += cfd_kernel.aocx
exec_local += dwarf-cfd-fpga-exec-local

cfd_kernel.aocx: $(top_srcdir)/unstructured-grids/cfd/cfd_kernel.cl
	$(AOC_COMPILER) -march=emulator $< -o $@

dwarf-cfd-fpga-exec-local:
	cp cfd_kernel.aocx ${DESTDIR}${bindir}
endif