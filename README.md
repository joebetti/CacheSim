CacheSim

This program inputs trace files and simulates a CPU cache. Output includes memory reads, memory writes, cache hits, and cache misses.

To run, first compile and then enter "./first (cache size) (associativity) (replacement policy) (block size) (trace file)

(cache size): the total cache size in bytes (must be a power of 2).

(associativity): direct - directly mapped cache.
		 assoc - a fully associative cache.
		 assoc:n - a n-way associative cache (n must be a power of 2).

(replace policy): lru - least recently used
		  fifo - first in first out

(block size): the size of each cache block in bytes (must be a power of 2)

(trace file): path to the trace file

EXAMPLES: ./first 64 assoc lru 4 "..testcases/first/input/trace1.txt"
	  ./first 32 assoc:4 fifo 8 "..testcases/first/input/trace2.txt"



(Trace files provided by Rutgers University)