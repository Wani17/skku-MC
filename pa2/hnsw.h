#ifndef HNSW_H
#define HNSW_H

#include <vector>
#include <unordered_map>
#include <iostream>
#include <immintrin.h>
#include <omp.h>
#include <time.h>
using namespace std;

struct Item {
	Item() {}
	Item(vector<float> _values):values(_values) {}
	vector<float> values;
	// Assume L2 distance
	float dist(Item& other) { 
		int i = 0, len = values.size();
		
		// SIMD를 활용하여 계산 속도 단축
		__m512 sum_vec = _mm512_setzero_ps();
		for(; i + 15 < len; i += 16) {
			__m512 a = _mm512_loadu_ps(&values[i]);
			__m512 b = _mm512_loadu_ps(&other.values[i]);	

			__m512 diff = _mm512_sub_ps(a, b);
			__m512 squared = _mm512_mul_ps(diff, diff);
			sum_vec = _mm512_add_ps(sum_vec, squared);
		}

		float result = _mm512_reduce_add_ps(sum_vec);

		for(; i < len; ++i) {
			result += (values[i] - other.values[i]) * (values[i] - other.values[i]);
		}

		return result;
	}
};

struct HNSWGraph {
	HNSWGraph(int _M, int _MMax, int _MMax0, int _efConstruction, int _ml, int _numInserts, Item& q, size_t _num_threads):M(_M),MMax(_MMax),MMax0(_MMax0),efConstruction(_efConstruction),ml(_ml),num_threads(_num_threads){
		for(int i = 0; i <= ml; ++i) {
			layerEdgeLists.push_back(vector<vector<int>>());
			layerEdgeLists[i].resize(_numInserts);
			nodeLocks.push_back(vector<pthread_rwlock_t>());
			nodeLocks[i].resize(_numInserts);
		}

		// lock init 병렬화
		#pragma omp parallel num_threads(num_threads)
		{
			#pragma omp for
			for(int i = 0; i < _numInserts; ++i) {
				for(int j = 0; j <= ml; ++j) {
					pthread_rwlock_init(&nodeLocks[j][i], NULL);
				}
			}
		}

		items.resize(_numInserts);
		items[0] = q;
		enterNode = 0;
	}

	~HNSWGraph() {
		int len = nodeLocks[0].size();
		#pragma omp parallel for collapse(2) num_threads(num_threads)
		for (int i = 0; i <= ml; ++i) {
			for (int j = 0; j < len; ++j) {
				pthread_rwlock_destroy(&nodeLocks[i][j]);
			}
		}
	}

	// vector<unsigned int> seeds;
	
	size_t num_threads;

	// Number of neighbors
	int M;
	// Max number of neighbors in layers >= 1
	int MMax;
	// Max number of neighbors in layers 0
	int MMax0;
	// Search numbers in construction
	int efConstruction;
	// Max number of layers
	int ml;

	// number of items
	int itemNum = 0;
	// actual vector of the items
	vector<Item> items;
	// adjacent edge lists in each layer
	vector<vector<vector<int>>> layerEdgeLists;
	// enter node id
	int enterNode;

	vector<vector<pthread_rwlock_t>> nodeLocks;

	// default_random_engine generator;

	// methods
	void addEdge(int st, int ed, int lc);
	vector<int> searchLayer(Item& q, int ep, int ef, int lc);
	vector<int> searchLayer_noLock(Item& q, int ep, int ef, int lc);
	void Insert(Item& q, int idx);
	void Pruning(int n, int l);
	void Pruning_noLock(int n);
	vector<int> KNNSearch(Item& q, int K);
};

#endif
