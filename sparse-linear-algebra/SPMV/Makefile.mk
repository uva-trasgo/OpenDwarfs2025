#
# Copyright 2010 by Virginia Polytechnic Institute and State
# University. All rights reserved. Virginia Polytechnic Institute and
# State University (Virginia Tech) owns the software and its
# associated documentation.
#

bin_PROGRAMS += csr
bin_PROGRAMS += createcsr

csr_LDFLAGS = -lOpenCL -lm
csr_SOURCES = sparse-linear-algebra/SPMV/src/csr.c sparse-linear-algebra/SPMV/src-common/sparse_formats.c sparse-linear-algebra/SPMV/src-common/ziggurat.c sparse-linear-algebra/SPMV/src-common/common.c

##createcsr does not need to be linked with any of the opencl common files
createcsr_SOURCES = sparse-linear-algebra/SPMV/src-test/createcsr.c sparse-linear-algebra/SPMV/src-common/sparse_formats.c sparse-linear-algebra/SPMV/src-common/ziggurat.c sparse-linear-algebra/SPMV/src-common/common.c

createcsr_LDADD = include/common_args.o include/rdtsc.o opts/opts.o
#createcsr_LINK = $(CCLD) -lm -o $@
createcsr_CFLAGS = -lm


all_local += csr-all-local
exec_local += csr-exec-local

csr-all-local:
	cp $(top_srcdir)/sparse-linear-algebra/SPMV/src/spmv_kernel.cl .
	cp $(top_srcdir)/sparse-linear-algebra/SPMV/src/spmv_kernel_fpga_optimized.cl .

csr-exec-local:
	cp $(top_srcdir)/sparse-linear-algebra/SPMV/src/spmv_kernel.cl ${DESTDIR}${bindir}
	cp $(top_srcdir)/sparse-linear-algebra/SPMV/src/spmv_kernel_fpga_optimized.cl ${DESTDIR}${bindir}
	
# Compilation targets for FPGA AOT compilation (Emulation or Board Synthesis)
if BUILD_AOT
all_local += spmv_kernel.aocx spmv_kernel_opt_fpga.aocx
exec_local += dwarf-csr-fpga-exec-local

spmv_kernel.aocx: $(top_srcdir)/sparse-linear-algebra/SPMV/src/spmv_kernel.cl
	$(AOC_COMPILER) $(AOC_FLAGS) $< -o $@

spmv_kernel_opt_fpga.aocx: $(top_srcdir)/sparse-linear-algebra/SPMV/src/spmv_kernel_fpga_optimized.cl
	$(AOC_COMPILER) $(AOC_FLAGS) $< -o $@

dwarf-csr-fpga-exec-local:
	cp spmv_kernel.aocx ${DESTDIR}${bindir}
	cp spmv_kernel_opt_fpga.aocx ${DESTDIR}${bindir}
endif