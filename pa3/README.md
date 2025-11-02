# PA3: Parallel Sorting using CUDA

## Overview
Implement a parallel radix sort for variable-length strings on the GPU; use the provided sequential bubble-sort tester for correctness and baseline.

## Rank
### #6 / 55
Computed with 1.898s.

---

35 people were succeeded.
  - Q3: 4.572s
  - Q2: 8.075s
  - Q1: 26.918s

### Runtime Environment
- Tested with 1M queries with specified workload.
- Intel Xeon Gold 5115
- NVIDIA Tesla V100

## Objective
Implement a correct, scalable GPU radix sort that outperforms the sequential baseline.

## Tasks
- Implement digit-wise passes and stable grouping on GPU; handle variable-length strings.
- Optimize memory traffic (coalescing, shared memory), occupancy, and divergence.
- Provide correctness printouts with more speedups.

## Workload
Workload is given as some random texts separated with line. The maximum of length is 30, and could be shorter than it.

## How to Run & Compile
In ```pa3``` directory, run following command:
```bash
make
```

This will compile code and create executable files named ```textgen``` and ```radix``` in ```pa3``` directory.

---

To make input workload, execute the following command:
```bash
./textgen {file name} {number_of_strings}
```
example:
```bash
./textgen 1M.input 1000000
```

---

To run radix sort, execute the following command:
```bash
./radix {input file} {number_of_strings} {print_pos_start} {print_pos_range}
```
example:
```bash
./skiplist 1M.input 1000000 0 10
```

---

To measure the time, execute as following command:
```bash
time ./radix {input file} {number_of_strings} {print_pos_start} {print_pos_range} > /dev/null
```