#
# Copyright 2010 by Virginia Polytechnic Institute and State
# University. All rights reserved. Virginia Polytechnic Institute and
# State University (Virginia Tech) owns the software and its
# associated documentation.
#

bin_PROGRAMS += crc 
bin_PROGRAMS += createcrc

crc_LDFLAGS = -lm 
#@SEARCHFLAGS@ @LIBFLAGS@ @RPATHFLAGS@
crc_SOURCES = combinational-logic/crc/src/crc_algo.c combinational-logic/crc/src-common/crc_formats.c

createcrc_SOURCES = combinational-logic/crc/src-test/createcrc.c combinational-logic/crc/src-common/crc_formats.c
createcrc_LDADD = include/rdtsc.o include/common_args.o opts/opts.o
createcrc_LINK = $(CCLD) -lm -o $@

all_local += dwarf-crc-all-local
exec_local += dwarf-crc-exec-local

# Standard build targets (copies source .cl files to build folder)
dwarf-crc-all-local:
	cp $(top_srcdir)/combinational-logic/crc/src/crc_kernel.cl .
	cp $(top_srcdir)/combinational-logic/crc/src/crc_kernel_fpga_optimized.cl .

dwarf-crc-exec-local:
	cp $(top_srcdir)/combinational-logic/crc/src/crc_kernel.cl ${DESTDIR}${bindir}
	cp $(top_srcdir)/combinational-logic/crc/src/crc_kernel_fpga_optimized.cl ${DESTDIR}${bindir}

# Compilation targets for FPGA AOT compilation (Emulation or Board Synthesis)
if BUILD_AOT
all_local += crc_kernel_opt_fpga.aocx crc_kernel.aocx
exec_local += dwarf-crc-fpga-exec-local

crc_kernel.aocx: $(top_srcdir)/combinational-logic/crc/src/crc_kernel.cl
	$(AOC_COMPILER) $(AOC_FLAGS) $< -o $@

crc_kernel_opt_fpga.aocx: $(top_srcdir)/combinational-logic/crc/src/crc_kernel_fpga_optimized.cl
	$(AOC_COMPILER) $(AOC_FLAGS) $< -o $@

dwarf-crc-fpga-exec-local:
	cp crc_kernel.aocx ${DESTDIR}${bindir}
	cp crc_kernel_opt_fpga.aocx ${DESTDIR}${bindir}
endif

