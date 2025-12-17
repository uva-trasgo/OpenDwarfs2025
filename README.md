# OpenDwarfs 2025

OpenDwarfs 2025 is a re-engineering of the [OpenDwarfs benchmark suite](https://github.com/vtsynergy/OpenDwarfs), originally published in 2012, which consists of the instantiation in OpenCL of various computation and communication idioms, known as the "13 Berkeley Dwarfs". It was designed to evaluate the performance of different high-performance computing architectures, such as multicore CPUs, GPUs, MICs, and FPGAs.

This updated benchmark suite addresses some issues in the latest available version of the original one, which has not seen significant updates in over six years. Key enhancements in OpenDwarfs 2025 include:

- **Error Correction**: Compilation and execution errors have been corrected, ensuring compatibility with modern platforms.
- **Improved Usability**: The user experience has been streamlined through standardized processes for compilation, execution, and result collection throughout all the benchmarks, making it easier for users to configure and run them.
- **Sample Experimental Inputs**: Input sets for evaluating the benchmarks have been added, for ease of use and to enable comparative performance analysis. For each benchmark, three kinds of input sets are available: _test_, _train_ and _reference_ (from least to most computationally intensive).

OpenDwarfs 2025 aims to restore and enhance the suite's functionality, making it a viable tool for evaluating the performance of diverse computing systems in today's rapidly evolving technological landscape. By building on the foundation of the original OpenDwarfs, this new version ensures that users can effectively measure and compare the performance of their systems.

The current version of OpenDwarfs 2025 has been extensively tested to work on modern CPUs (Intel and AMD), and GPUs (NVIDIA and AMD).

## Citing

If you use OpenDwarfs 2025 in your research, we would appreciate it if you could cite the following paper:

```
@inproceedings{OpenDwarfs2025,
    author = {Ropero, Juan and de Castro, Manuel and Llanos, Diego},
    year = {2025},
    month = {08},
    title = {OpenDwarfs 2025: Modernizing the OpenDwarfs Benchmark Suite for Heterogeneous Computing},
    doi = {10.5281/zenodo.17044367}
}
```

Below you can find the original README of the OpenDwarfs benchmark suite (adapted from the one found in the latest version of its repository), which provides more information about the project and its benchmarks.

OpenDwarfs (original README)
==========

  The OpenDwarfs project provides a benchmark suite consisting of different computation/communication idioms, i.e., dwarfs, for state-of-art multicore CPUs, GPUs, Intel MICs and Altera FPGAs. 
  
  The first instantiation of the OpenDwarfs has been realized in OpenCL, as briefly described in "OpenCL and the 13 Dwarfs: A Work in Progress" by Feng, Lin, Scogland, and Zhang in the 3rd ACM/SPEC International Conference on Performance Engineering, April 2012. 
  The current version, which contains an in-depth performance evaluation on a subset of OpenDwarfs, is described in "On the Characterization of OpenCL Dwarfs on Fixed and Reconfigurable Platforms" by Krommydas, Feng, Owaida, Antonopoulos, and Bellas in the 25th IEEE International Conference on Application-specific Systems, Architectures and Processors (ASAP), June 2014.
  A more thorough description of the latest version, including further in-depth performance evaluation for a larger number of OpenDwarfs, is described in "OpenDwarfs: Characterization of Dwarf-based Benchmarks on Fixed and Reconfigurable Architectures" by Krommydas, Feng, Antonopoulos, and Bellas in Journal of Signal Processing Systems (JSPS), Springer, October 2015.
  
The computation/communication idioms are based on the [13 Berkeley Dwarfs](http://view.eecs.berkeley.edu/wiki/Dwarf_Mine).

Benchmark status
----------------

Stable:
gem

Beta:
bfs
cfd
crc
fft
kmeans
lud
nw
spmv
srad
swat
bwa\_hmm
nqueens

Alpha:
tdm

Requirements
------------

Packages and libraries needed to build and run the applications.

To build:

    opencl >= 1.0 (some apps require 1.1, but we do not yet guarantee support for 1.2 in all applications.)
    autoconf >= 2.63
    autoheader
    automake
    libtool
    gcc
    maker

To run:

    opencl libs

Building
--------

To build all of the included applications:

    $ ./autogen.sh
    $ mkdir build
    $ cd build
    $ ../configure
    $ make

To build only the applications you select, call configure with the --with-apps
option:

    $ ../configure --with-apps=srad,gem,cfd

To see a full list of options and applications:

    $ ../configure --help

Running
-------

See the application-specific README file in each application's directory.
All the dwarf applications support a common list of options for optionally specifying the OpenCL platform ID (-p)
and OpenCL device ID (-d), or alternatively, the device type (-t). Optionally you can provide -o option to use 
optimized kernels. It picks up the optimized kernel for the given device type. For an example, if the device in use 
is GPU and -o option is provided, it will use <kenel\_name>\_opt\_gpu.cl file present in the application directory. 
These options, if supplied, must follow the executable name and be delimited from the application-specific options by double dashes (--).

General format: ./<executable> [-p <platform> -d <device> | -t <type> -o --] [app-specific options]

    <platform>	:integer ID of platform to use
    <device>    :integer ID of device in <platform> to use
    <type>	    : device type to use (0:CPU, 1:GPU, 2:MIC, 3:FPGA)
    -o          :Optional flag to use the optimzed flag for the device in use

Example1: ./astar -p 0 -d 0 -- (selects device with device ID 0 on platform with platform ID 0)
Example2: ./astar -t 0 -- (selects CPU device type on default platform with platform ID 0, if available)
Example2: ./nw -p 0 -d 0 -o -- (Run the optimized dwarf of device ID 0 on platfrom with platform ID 0)

Notes:	If no parameters are supplied, default platform ID is 0 and default device type is CPU.
	If -t parameter is given, default platform ID 0 is searched for supplied device type <type>. If not available, CPU device type selection will be attempted.
	If device ID is unknown, a combination of -p and -t is available to search for device of selected <type> on platform ID <platform>.
    If the optimized kernel does not exist, application wil throw and error and exit. 

Notes: SWAT DOES NOT compile for OpenCl and FFT kernel DID NOT fit on Stratix V in this release. 

Acknowledgements
----------------

This project has been supported in part by Air Force Research Lab, Altera, AMD, Department of Defense, Harris, Los Alamos National Laboratory, and Xilinx via the NSF Center for High-Performance Reconfigurable Computing (CHREC) under NSF grant IIP-0804155 and indirectly by AFOSR grant FA9550-12-1-0442 and NSF grants CNS-0916719 and MRI-0960081.

Integration for Altera FPGA support for crc and csr, as well as extensions for these
benchmarks, have been contributed by Tyler Kenney at IBM.

Part of the OpenDwarfs benchmark suite (as acknowledged in the respective benchmarks' READMEs) was ported to OpenCL from the corresponding CUDA implementations in earlier implementations of the [Rodinia benchmark suite](http://www.cs.virginia.edu/~skadron/wiki/rodinia/index.php/Main_Page). 
