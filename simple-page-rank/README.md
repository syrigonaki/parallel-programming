
## Summary

Parallel implementation of a simplified page rank algorithm in C using pthreads. Includes parsing large graph files and iterative rank computation.

## Compilation

### Using GCC directly

```bash
gcc -o page_rank page_rank.c -lpthread
```

### Using Makefile

```bash
make all
```

Remove executable and temporary files using
```bash
make clean
```

## Running the program

```bash
./page_rank <input_file>
```

input_file format: each line contains src_node dst_node.  Lines starting with # are ignored.

For example:
```bash
0 3
1 5
#Comment
1 0
...
```

output_file format: each line contains node, pagerank

For example:

```bash
node,pagerank
0,1.0
1,0.85
2,0.92
...
```

## Notes:
* Default number of threads: 4 ('NUM_OF_THRDS' in the code).
* Performs 50 iterations of PageRank (hardcoded).
---
*This was developed for the Parallel Programming Course*
