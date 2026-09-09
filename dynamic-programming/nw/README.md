Needleman-Wunsch
================

Status: Beta

Needleman-Wunsch is an algorithm for calculating optimal global alignment of two
DNA sequences. All-pairs matching is represented by an NxM 2D matrix. Matrix
cells are scored from northwestern-most to southeastern-most, depending solely
on northern, northeastern, and eastern neighboring scores. Finally, the
algorithm backtracks through the array to return an optimal alignment.

Note: This application was ported from the Rodinia Suite
      (https://www.cs.virginia.edu/~skadron/wiki/rodinia/).

Running
-------

Usage: `needle [[-p <platform> -d <device> | -t <type>] [-c <compute-units>] --] <length of sequences> <penalty value>` 

    -d dimensions   : x and y dimensions of the problem
    -p penalty      : penalty value
    -b blocksize    : blocksize|local work size

    <platform>      : integer ID of platform to use
    <device>        : integer ID of device in <platform> to use
    <type>          : device type to use (0:CPU, 1:GPU, 2:MIC, 3:FPGA)
    <compute-units> : Optional flag to specify number of compute-units to use on CPU 	
    
Example: `needle -d 256 -p 1 -b 16`

Note: This program generate two sequences randomly. Please specify your own
      sequences for different uses. At the current stage, the program only
      supports two sequences with the same lengh, which can be divided by 16.
