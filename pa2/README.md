# PA2: HNSW using OpenMP

## Overview
HNSW (Hierarchical Navigable Small World) is a graph-based index used for approximate nearest neighbor (ANN) search in high-dimensional spaces. Parallelize HNSW build and search with OpenMP. 

## Rank
### Build: #2 / 55
Computed with 1.19299s.

---

31 people were succeeded.
  - Q3: 5.36342s
  - Q2: 143.814s
  - Q1: 503.07s

### Query: #2 / 55
Computed with 0.056654s.

---

31 people were succeeded.
  - Q3: 0.183218s
  - Q2: 0.251111s
  - Q1: 2.369180s

### Runtime Environment
- Tested with 100K queries with specified workload.
- Optimized with ANN to 100; i.e. ```-k 100```
- Optimized with recall to be equal or more than 85%.
- Intel Xeon Gold 5115
  - use 40 threads
- NVIDIA Tesla V100

## Objective
Achieve fast, scalable HNSW while maintaining recall ≥ 85% for ```k=100```. 

## Tasks
- Parallelize build (layer insertion, neighbor selection) and search (ef, beamwidth) with openMP.
- Tune key parameters (M, MMax, MMax0, efConstruction, ml) for 40 threads.

## Workload
Sample workload is given in ```pa2/hnsw_dataset```. Tested with similar dataset.

## How to Run & Compile
In ```pa2``` directory, run following command:
```bash
make
```

This will compile code and create an executable file named ```hnsw``` in ```pa2``` directory.

---

To run HNSW, execute the following command:
```bash
./hnsw -k {num of nearest neighbors} -t {num of threads} -w {workload num (1-6)}
```
example:
```bash
./skiplist -k 100 -t 40 -w 4
```
### Caution
This project is compiled using:
```bash
g++ -O3 -fopenmp -mavx512f -mavx2 -pthread
```
These flags indicate that the implementation utilizes:
- Pthreads for concurrency
- OpenMP (enabled through -fopenmp)
- Advanced CPU instruction sets, including
  - AVX2 (-mavx2)
  - AVX-512F (-mavx512f)
---

To measure the time, execute as following command:
```bash
time ./hnsw -k {num of nearest neighbors} -t {num of threads} -w {workload num (1-6)} > /dev/null
```