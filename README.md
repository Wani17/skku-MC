# Multicore Computing Project Assignments (2024-2)
**Instructor:** Prof. Beomseok Nam

This repository contains implements of multicore computing and and distributed computing projects, each are organized as folders. Each folder includes its implementation files and descriptions.

## PA1: Concurrent SkipList with PThreads
- **Objective:** Implement a scalable concurrent key-value store using a master/worker task-queue model with POSIX threads; practice fine-grained synchronization (mutexes, condition variables).
- **Rank:** #1 / 55
- **Key Features:**
  - Parse a text file of tasks (```i n```, ```q n```, ```p```) and dispatch to worker threads; output must match the single-threaded reference.
  - Implement as per-node fine-grained locks for the SkipList.

## PA2: HNSW using OpenMP
- **Objective:** Parallelize HNSW (Hierarchical Navigable Small World) index construction and search with OpenMP while keeping recall ≥ 85% for ```k=100```.
- **Rank:** #2 / 55
- **Key Features:**
  - Parallelize build (layer insertion, neighbor selection) and search (ef, beamwidth) with openMP.
  - Tune key parameters (M, MMax, MMax0, efConstruction, ml) for 40 threads

## PA3: Parallel Sorting using CUDA
- **Objective:** Implement a parallel radix sort for variable-length strings in CUDA.
- **Rank:** #6 / 55
- **Key Features:**
  - Implement digit-wise passes and stable grouping on GPU; handle variable-length strings.
  - It should run as faster with various optimizations.

## PA4: rwho using RPC
- **Objective:** Build an RPC-based version of ```who```/```rwho``` to print users logged into remote machines.
- **Grade:** 100/100
- **Key Features:**
  - Define IDL (```who.x```), run rpcgen, implement server (```server.c```) and client (```client.c```).