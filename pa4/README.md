# PA4: rwho using RPC

## Overview
Create a simple distributed rwho: define an RPC interface, generate stubs with rpcgen, implement server to fetch logged-in users (from the who utility) and a client to display the result.

## Grade
100/100

- No time was measured; only correctness.

- 47people were succeded.

### Runtime Environment
- Tested with 1M queries with specified workload.
- Intel Xeon Gold 5115
- NVIDIA Tesla V100

## Objective
Build a working RPC client/server pair that mirrors who functionality across machines.

## Tasks
- Write who.x interface; run rpcgen to create stubs. 
- Implement server: call get_logged_in_users() and return results. 
- Implement client: invoke RPC and print the string.

## How to Run & Compile
In ```pa4``` directory, run following command:
```bash
make
```

This will compile code and create rpc stubs and executable files named ```client``` and ```server``` in ```pa4``` directory.

---

In server machine, execute the following command:
```bash
./server
```
And in client machine, execute the following command:
```bash
./client {hostname}
```