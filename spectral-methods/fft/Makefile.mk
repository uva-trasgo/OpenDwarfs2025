#
# Copyright 2010 by Virginia Polytechnic Institute and State
# University. All rights reserved. Virginia Polytechnic Institute and
# State University (Virginia Tech) owns the software and its
# associated documentation.
#

bin_PROGRAMS += clfft

clfft_SOURCES = spectral-methods/fft/src/opencl/fft/fft.cpp spectral-methods/fft/src/opencl/fft/fftlib.cpp spectral-methods/fft/src/opencl/common/main.cpp spectral-methods/fft/src/opencl/common/Event.cpp spectral-methods/fft/src/opencl/common/OpenCLDeviceInfo.cpp spectral-methods/fft/src/opencl/common/OpenCLNodePlatformContainer.cpp spectral-methods/fft/src/opencl/common/OpenCLPlatform.cpp spectral-methods/fft/src/common/OptionParser.cpp spectral-methods/fft/src/common/ResultDatabase.cpp spectral-methods/fft/src/common/Timer.cpp spectral-methods/fft/src/common/Option.cpp spectral-methods/fft/src/common/InvalidArgValue.cpp 

clfft_CPPFLAGS = -I$(top_srcdir)/spectral-methods/fft/src/common -I$(top_srcdir)/spectral-methods/fft/src/opencl/common

all_local += clfft-all-local
exec_local += clfft-exec-local

clfft-all-local:
	cp $(top_srcdir)/spectral-methods/fft/src/opencl/fft/fft.cl .

clfft-exec-local:
	cp $(top_srcdir)/spectral-methods/fft/src/opencl/fft/fft.cl ${DESTDIR}${bindir}

# New: Compilation targets for FPGA Emulation if --enable-aot-emulation is passed
if BUILD_AOT_EMULATION
all_local += fft.aocx
exec_local += dwarf-fft-fpga-exec-local

fft.aocx: $(top_srcdir)/spectral-methods/fft/src/opencl/fft/fft.cl
	# Static defines needed for compilation since it cant be defined during execution
	$(AOC_COMPILER) -march=emulator -DFFT_128 -DFFT_256 -DFFT_512 -DFFT_1024 -DFFT_2048 -DFFT_4096 -DFFT_8192 -Dfftn1=1024 -Dpow1=64 $< -o $@

dwarf-fft-fpga-exec-local:
	cp fft.aocx ${DESTDIR}${bindir}
endif
