Baum-Welch Algorithm
===============

Status: Beta

The Baum-Welch algorithm falls into the graphical models dwarf and has
many applications, including bioinformatics, cryptanalysis and speech
recognition. It makes use of the forward-backward algorithm and finds
the unknown parameters of a hidden Markov model (HMM).

Running
-------

Usage: `bwa_hmm [[-p <platform> -d <device> | -t <type>] [-c <compute-units>] --] (see examples below for available run parameters, while keeping some parameters fixed)`

    -n <number>     : value for state (n), must precede -v n
    -t <number>     : value for observation (t), must precede -v t
    -s <number>     : value for symbol (s), must precede -v s
    -v <variable>   : specify the variable to vary (n, s, t)

    <platform>	    : integer ID of platform to use
    <device>        : integer ID of device in <platform> to use
    <type>		    : device type to use (0:CPU, 1:GPU, 2:MIC, 3:FPGA)	
    <compute-units> : Optional flag to specify number of compute-units to use on CPU 	
    
Example:

* Vary state (n) with fixed S = 2, T = 1000
	`$ ./bwa_hmm -n <number> -v n`
* Vary symbols (s) with fixed N = 60, T = 1000
	`$ ./bwa_hmm -s <number> -v s`
* Vary observations (t) with fixed N = 60, S = 2
	`$ ./bwa_hmm -t <number> -v t`

