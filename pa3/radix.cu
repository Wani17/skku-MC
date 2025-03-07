#include<iostream>
#include<fstream>
#include<string.h>
#include <vector>
#include <algorithm>

using namespace std;

#define STRING_LENMAX 30
#define MIN_CHAR 63
#define NUM_CHARACTER 64

void errorcheck(cudaError_t err) {
    if (err != cudaSuccess) {
        std::cerr << "CUDA Error: " << cudaGetErrorString(err) << std::endl;
        // 적절한 에러 처리
    }
}

__global__ void kernel_init(char* d_string, int* d_index, int N) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if(idx < N) {
        int flag = 0;
        d_index[idx] = idx;
        for (int i = 0; i < STRING_LENMAX; i++) {
            if(!d_string[idx * STRING_LENMAX + i]) flag = 1;
            if(flag)
                d_string[idx * STRING_LENMAX + i] = 64;
        }
    }
}

__global__ void kernel_block_prefix_sum(char* d_string, int* d_index, int* offset, int* blockSum, int pos, int N) {
    extern __shared__ short sharedData[][NUM_CHARACTER];

    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x + tid;

    for(int i = 0; i < NUM_CHARACTER; ++i) {
        sharedData[tid][i] = 0;
    }

    __syncthreads();
    char c = 0;

    if(idx < N) {
        c = d_string[d_index[idx] * STRING_LENMAX + pos] - MIN_CHAR;
        sharedData[tid][c] = 1;
    }

    __syncthreads();

    for(int stride = 1; stride < blockDim.x; stride <<= 1) {
        int index = (tid + 1) * stride * 2 - 1;
        if(index < blockDim.x) {
            for(int i = 0; i < NUM_CHARACTER; ++i) {
                sharedData[index][i] += sharedData[index - stride][i];
            }
        }

        __syncthreads();
    }

    if(tid == blockDim.x - 1) {
        for(int i = 0; i < NUM_CHARACTER; ++i) {
            blockSum[blockIdx.x * NUM_CHARACTER + i] = sharedData[tid][i];
            sharedData[tid][i] = 0;
        }
    }

    __syncthreads();

    for(int stride = blockDim.x >> 1; stride > 0; stride >>= 1) {
        int index = (tid + 1) * stride * 2 - 1;
        if(index < blockDim.x) {
            for(int i = 0; i < NUM_CHARACTER; ++i) {
                int temp = sharedData[index - stride][i];
                sharedData[index - stride][i] = sharedData[index][i];
                sharedData[index][i] += temp;
            }
        }

        __syncthreads();
    }

    if(idx < N) {
        offset[idx] = sharedData[tid][c];
    }
}

__global__ void kernel_addBlockSums(char* d_string, int* d_index, int* data, int* blockSums, int pos, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    char c = 0;

    __shared__ int histogram[64];

    if(threadIdx.x < 64) {
        histogram[threadIdx.x] = blockSums[(gridDim.x - 1) * NUM_CHARACTER + threadIdx.x];
    }

    __syncthreads();

    if(idx < n) {
        c = d_string[d_index[idx] * STRING_LENMAX + pos] - MIN_CHAR;
        data[idx] += histogram[c-1];
    }

    if (idx < n && blockIdx.x > 0) {
        data[idx] += blockSums[(blockIdx.x - 1)*NUM_CHARACTER + c];
    }
}

__global__ void kernel_pr(int* data, int N) {
    int tid = threadIdx.x;
    if(blockIdx.x == 0 && tid < 64) {
        for(int i = 1; i < N; ++i) {
            data[i*NUM_CHARACTER + tid] += data[(i-1)*NUM_CHARACTER + tid];
        }
    }

    __syncthreads();

    if(tid == 0 && blockIdx.x == 0) {
        for(int i = 1; i < 64; ++i) {
            data[(N-1) * NUM_CHARACTER + i] += data[(N-1) * NUM_CHARACTER + (i-1)];
        }
    }
}

__global__ void kernel_remap(int* index, int* index2, int N) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if(idx < N) {
        index2[idx] = index[idx];
    }
}

__global__ void remap2(int* index, int* index2, int* offset, int N) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if(idx < N) {
        index[offset[idx]] = index2[idx];
    }
}

int main(int argc, char* argv[])
{
    int i, N, pos, range, ret;

    if(argc<5){
	    cout << "Usage: " << argv[0] << " filename number_of_strings pos range" << endl;
	    return 0;
    }

    ifstream inputfile(argv[1]);
    if(!inputfile.is_open()){
	    cout << "Unable to open file" << endl;
	    return 0;
    }

    ret=sscanf(argv[2],"%d", &N);
    if(ret==EOF || N<=0){
	    cout << "Invalid number" << endl;
	    return 0;
    }

    ret=sscanf(argv[3],"%d", &pos);
    if(ret==EOF || pos<0 || pos>=N){
	    cout << "Invalid position" << endl;
	    return 0;
    }

    ret=sscanf(argv[4],"%d", &range);
    if(ret==EOF || range<0 || (pos+range)>N){
	    cout << "Invalid range" << endl;
	    return 0;
    }

    char* h_string = new char[N * STRING_LENMAX];
    char tmp[STRING_LENMAX];

    for(i=0; i<N; i++) {
        inputfile>>tmp;
        memmove(h_string + i*STRING_LENMAX, tmp, STRING_LENMAX);
    }

    inputfile.close();

    char* d_string;
    int* d_index;
    int* d_offset;
    int* d_histogram;
    int* d_index2;

    int threadsPerBlock = 256;
    int numBlocks = (N + threadsPerBlock - 1) / threadsPerBlock;
    int sharedMemorySize = threadsPerBlock * NUM_CHARACTER * sizeof(short);

    cudaMalloc(&d_string, N * STRING_LENMAX * sizeof(char));
    cudaMalloc(&d_index, N * sizeof(int));
    cudaMalloc(&d_index2, N * sizeof(int));
    cudaMalloc(&d_offset, N * sizeof(int));
    cudaMalloc(&d_histogram, numBlocks * NUM_CHARACTER * sizeof(int));

    cudaMemcpy(d_string, h_string, N * STRING_LENMAX * sizeof(char), cudaMemcpyHostToDevice);
    kernel_init<<<(N+1023) / 1024, 1024>>>(d_string, d_index, N);

    for(int pos = STRING_LENMAX - 1; pos >= 0; --pos) {
        kernel_block_prefix_sum<<<numBlocks, threadsPerBlock, sharedMemorySize>>>(d_string, d_index, d_offset, d_histogram, pos, N);
        // errorcheck(cudaGetLastError());
        cudaDeviceSynchronize(); 


        kernel_pr<<<1, 64>>>(d_histogram, numBlocks);
        // errorcheck(cudaGetLastError());
        cudaDeviceSynchronize();

         kernel_addBlockSums<<<numBlocks, threadsPerBlock>>>(d_string, d_index, d_offset, d_histogram, pos, N);
        // errorcheck(cudaGetLastError());
        cudaDeviceSynchronize(); 

        kernel_remap<<<(N+1023) / 1024, 1024>>>(d_index, d_index2, N);
        // errorcheck(cudaGetLastError());
        cudaDeviceSynchronize(); 

        remap2<<<(N+1023) / 1024, 1024>>>(d_index, d_index2, d_offset, N);
        // errorcheck(cudaGetLastError());
        cudaDeviceSynchronize(); 
    }

    int* h_index = new int[N];
    cudaMemcpy(h_index, d_index, N * sizeof(int), cudaMemcpyDeviceToHost);

    cout<<"\nStrings (Names) in Alphabetical order from position " << pos << ": " << endl;
    for(i=pos; i<N && i<(pos+range); i++) {
        memmove(tmp, h_string + STRING_LENMAX * h_index[i], 30);
        cout<< i << ": " << tmp<<endl;
    }
    cout<<endl;

    delete[] h_string;
    delete[] h_index;

    cudaFree(d_string);
    cudaFree(d_index);
    cudaFree(d_index2);
    cudaFree(d_offset);
    cudaFree(d_histogram);

    return 0;
}


