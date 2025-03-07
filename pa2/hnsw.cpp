#include "hnsw.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <random>
#include <set>
#include <unordered_set>
#include <vector>
#include <queue>
#include <algorithm>
#include <stdio.h>
#include <chrono>
using namespace std;

vector<int> HNSWGraph::searchLayer(Item& q, int ep, int ef, int lc) {
	priority_queue<pair<float, int>, vector<pair<float, int>>, greater<pair<float, int>>> candidates;
	priority_queue<pair<float, int>> nearestNeighbors;
	unordered_set<int> isVisited;

	float td = q.dist(items[ep]);

	candidates.emplace(td, ep);
	nearestNeighbors.emplace(td, ep);
	isVisited.insert(ep);

	while (!candidates.empty()) {
		auto ci = candidates.top();
		candidates.pop();

		int nid = ci.second;
		
		if (ci.first > nearestNeighbors.top().first) break;

		pthread_rwlock_rdlock(&nodeLocks[lc][nid]);
		for (int ed: layerEdgeLists[lc][nid]) {
			if (isVisited.find(ed) != isVisited.end()) continue;
			isVisited.insert(ed);

			td = q.dist(items[ed]);
		
			if(nearestNeighbors.size() < ef) {
				candidates.emplace(td, ed);
				nearestNeighbors.emplace(td, ed);
			}
			else if(td < nearestNeighbors.top().first) {
				nearestNeighbors.pop();
				candidates.emplace(td, ed);
				nearestNeighbors.emplace(td, ed);
			}
		}
		pthread_rwlock_unlock(&nodeLocks[lc][nid]);
	}

	vector<int> results;
	while(!nearestNeighbors.empty()) {
		results.push_back(nearestNeighbors.top().second);
		nearestNeighbors.pop();
	}
	reverse(results.begin(), results.end());
	return results;
}

vector<int> HNSWGraph::searchLayer_noLock(Item& q, int ep, int ef, int lc) {
	priority_queue<pair<float, int>, vector<pair<float, int>>, greater<pair<float, int>>> candidates;
	priority_queue<pair<float, int>> nearestNeighbors;
	unordered_set<int> isVisited;

	float td = q.dist(items[ep]);

	candidates.emplace(td, ep);
	nearestNeighbors.emplace(td, ep);
	isVisited.insert(ep);

	while (!candidates.empty()) {
		auto ci = candidates.top();
		candidates.pop();

		int nid = ci.second;
		
		if (ci.first > nearestNeighbors.top().first) break;

		for (int ed: layerEdgeLists[lc][nid]) {
			if (isVisited.find(ed) != isVisited.end()) continue;
			isVisited.insert(ed);

			td = q.dist(items[ed]);
		
			if(nearestNeighbors.size() < ef) {
				candidates.emplace(td, ed);
				nearestNeighbors.emplace(td, ed);
			}
			else if(td < nearestNeighbors.top().first) {
				nearestNeighbors.pop();
				candidates.emplace(td, ed);
				nearestNeighbors.emplace(td, ed);
			}
		}
	}

	vector<int> results;
	while(!nearestNeighbors.empty()) {
		results.push_back(nearestNeighbors.top().second);
		nearestNeighbors.pop();
	}
	reverse(results.begin(), results.end());
	return results;
}

vector<int> HNSWGraph::KNNSearch(Item& q, int K) {
	int maxLyer = layerEdgeLists.size() - 1;
	int ep = enterNode;
	for (int l = maxLyer; l >= 1; l--) {
		ep = searchLayer_noLock(q, ep, 1, l)[0];
	}

	return searchLayer_noLock(q, ep, K, 0);
}

void HNSWGraph::addEdge(int st, int ed, int lc) {
	pthread_rwlock_rdlock(&nodeLocks[lc][st]);
	if(layerEdgeLists[lc][st].size() < layerEdgeLists[lc][st].capacity()) {
		layerEdgeLists[lc][st].push_back(ed);
	}
	else {
		pthread_rwlock_unlock(&nodeLocks[lc][st]);
		pthread_rwlock_wrlock(&nodeLocks[lc][st]);
		layerEdgeLists[lc][st].push_back(ed);
	}
	pthread_rwlock_unlock(&nodeLocks[lc][st]);

	pthread_rwlock_rdlock(&nodeLocks[lc][ed]);
	if(layerEdgeLists[lc][ed].size() < layerEdgeLists[lc][ed].capacity()) {
		layerEdgeLists[lc][ed].push_back(st);
	}
	else {
		pthread_rwlock_unlock(&nodeLocks[lc][ed]);
		pthread_rwlock_wrlock(&nodeLocks[lc][ed]);
		layerEdgeLists[lc][ed].push_back(st);
	}
	pthread_rwlock_unlock(&nodeLocks[lc][ed]);

}

void HNSWGraph::Insert(Item& q, int idx) {
	int nid = idx;
	items[nid] = q;

	unsigned int seed = idx;
	// omp threadidx로 해야 정확하지만, 시간 단축을 위해 rand_r로 수행.
	int l = min(ml, __builtin_ctz(rand_r(&seed)));

	int ep = enterNode;
	for (int i = ml; i > l; i--) {
		ep = searchLayer(q, ep, 1, i)[0];
	}

	for (int i = l; i >= 0; i--) {
		vector<int> neighbors = searchLayer(q, ep, efConstruction, i);
		int lmt = min(M, int(neighbors.size()));
		
		for(int j = 0; j < lmt; ++j) {
			addEdge(neighbors[j], nid, i);
		}

		ep = neighbors[0];
		Pruning(ep, i);
	}
}

void HNSWGraph::Pruning(int n, int l) {
	int lmt = 60;
	int chnge = l == 0 ? 40 : 30;
	pthread_rwlock_rdlock(&nodeLocks[l][n]);
	if (layerEdgeLists[l][n].size() > lmt) {
		pthread_rwlock_unlock(&nodeLocks[l][n]);
		vector<pair<float, int>> distPairs;
		pthread_rwlock_wrlock(&nodeLocks[l][n]);
		for (int nn: layerEdgeLists[l][n]) distPairs.emplace_back(items[n].dist(items[nn]), nn);
		sort(distPairs.begin(), distPairs.end());
		layerEdgeLists[l][n].clear();
		for (int d = 0; d < chnge; d++) layerEdgeLists[l][n].push_back(distPairs[d].second);
	}
	pthread_rwlock_unlock(&nodeLocks[l][n]);
}


void HNSWGraph::Pruning_noLock(int n) {
	int MM = 40;
	if (layerEdgeLists[0][n].size() > MM) {

		vector<pair<float, int>> distPairs;
		for (int nn: layerEdgeLists[0][n]) distPairs.emplace_back(items[n].dist(items[nn]), nn);
		sort(distPairs.begin(), distPairs.end());
		layerEdgeLists[0][n].clear();
		for (int d = 0; d < MM; d++) layerEdgeLists[0][n].push_back(distPairs[d].second);
	}
}