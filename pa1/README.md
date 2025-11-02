# PA1: Concurrent Skiplist with PThreads

## Overview
Build a concurrent SkipList key-value store driven by a master/worker task queue, using POSIX threads with correct synchronization and identical output to the serial reference. 

## Rank
### #1 / 55
Computed with 0.721s.

18 people were succeeded.
  - Q3: 2.050s
  - Q2: 5.630s
  - Q1: 9.559s

### Runtime Environment
- Tested with 1M queries with specified workload.
- Intel Xeon Gold 5115
  - could use 40 threads
- NVIDIA Tesla V100

## Objective
Implement a thread-safe SkipList and a master/worker dispatcher with correctness equal to the serial baseline.

## Tasks
- Read a list of queries, and each task consists of a character
code indicating an action and an optional number.
  - ```i number```: number n should be inserted to SkipList.
  - ```q number```: search number n and print out the number and its next number in SkipList. 
  - ```p```: print first 200 numbers in SkipList in that time.
- Guarantee serial-equivalent semantics despite concurrent processing (e.g., ordering concerns around ```q``` before ```i```).

## Workload
- The first 80% of operations are inserts(```i```).
- In the remaining 20%, only 10% are query(```q```) operations.

## How to Run & Compile
In ```pa1``` directory, run following command:
```bash
make
```

This will compile code and create executable files named ```inputegen``` and ```skiplist``` in ```pa1``` directory.

---

To make input workload, execute the following command:
```bash
./inputgen -n {num of queries} -h {'q' hit rate (0-100)} > {text file name}
```
example:
```bash
./inputgen -n 1000000 -h 50 > 1M.input
```

---

To run skiplist, execute the following command:
```bash
./skiplist {input file} {num of threads}
```
example:
```bash
./skiplist 1M.input 40
```

---

To measure the time, execute as following command:
```bash
time ./skiplist {input file} {num of threads} > /dev/null
```