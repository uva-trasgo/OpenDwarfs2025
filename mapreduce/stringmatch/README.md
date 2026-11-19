stringmatch (from StreamMR)
================

Status: Beta

Stringmatch searches an input text for occurrences of a set of target words
and reports how many times each target occurs. It parallelizes by splitting
the input text into line-sized records: each thread checks its own line
against the full list of targets (kept small and shared across all threads)
and accumulates matches into per-target buckets via atomics; a reduce phase
then sums each bucket into a final count. Available parallelism scales with
the size of the input text rather than with the number of targets being
searched for.

Note: This benchmark is a MapReduce port based on StreamMR https://github.com/vtsynergy/StreamMR/tree/master. 

The full license associated is in ./LICENSE.md

Running
-------


Usage: `stringmatch [[-p <platform> -d <device> | -t <type>] [-c <compute-units>] --] [options]`

    -i input-file     : file with targets (comma-separated, first line) and text to search (remaining lines)
    -h                : print this help message

    <platform>	      : integer ID of platform to use
    <device>          : integer ID of device in <platform> to use
    <type>		      : device type to use (0:CPU, 1:GPU, 2:MIC, 3:FPGA)
    <compute-units>   : Optional flag to specify number of compute-units to use on CPU

Example (from a file):   stringmatch -i test/mapreduce/stringmatch/test_file

Test File Generator
------------------

The `stringmatch_gen` utility generates random input datasets (keys/text) for the Stringmatch benchmark.

**Usage:** `./stringmatch_gen [OPTIONS]`

* `-n <num-targets>`: Number of target words to generate [default=5].
* `-s <text-size>`  : Bytes of text to generate. Might overshoot on last word [default=30000000].
* `-t`              : Generate the test, train and reference files.

**Example:** Generate 10 keys and a text of at least 4096 bytes along with the three test files:
`./stringmatch_gen -n 10 -s 4096 -t`