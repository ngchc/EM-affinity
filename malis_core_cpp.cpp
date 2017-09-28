#include <iostream>
#include <cstdlib>
#include <cmath>
#include <boost/pending/disjoint_sets.hpp>
#include <vector>
#include <queue>
#include <map>
#include <cstdio>
#include <cstddef>
#include <cstdint>
using namespace std;

template <class T>
class AffinityGraphCompare{
    private:
        const T * mEdgeWeightArray;
    public:
        AffinityGraphCompare(const T * EdgeWeightArray){
            mEdgeWeightArray = EdgeWeightArray;
        }
        bool operator() (const int& ind1, const int& ind2) const {
            return (mEdgeWeightArray[ind1] > mEdgeWeightArray[ind2]);
        }
};

/*
 * Compute the MALIS loss function and its derivative wrt the affinity graph
 * MAXIMUM spanning tree
 * Author: Srini Turaga (sturaga@mit.edu)
 * All rights reserved
 */
void malis_loss_weights_cpp(const int nVert, const uint64_t* seg,
               const uint64_t* conn_dims, const uint64_t* nhood_data, const uint64_t* nhood_dims,
               const float* edgeWeight,
               const int pos,
               uint64_t* nPairPerEdge){


    /* Disjoint sets and sparse overlap vectors */
    vector<map<uint64_t,uint64_t> > overlap(nVert);
    vector<uint64_t> rank(nVert);
    vector<uint64_t> parent(nVert);
    boost::disjoint_sets<uint64_t*, uint64_t*> dsets(&rank[0],&parent[0]);
    for (int i=0; i<nVert; ++i){
        dsets.make_set(i);
        if (0!=seg[i]) {
            overlap[i].insert(pair<uint64_t,uint64_t>(seg[i],1));
        }
    }

    /* get number of edges */
  int nEdge = 0;
  // Loop over #edges
  for (int d = 0, i = 0; d < conn_dims[0]; ++d) {
    // Loop over Z
    for (int z = 0; z < conn_dims[1]; ++z) {
      // Loop over Y
      for (int y = 0; y < conn_dims[2]; ++y) {
        // Loop over X
        for (int x = 0; x < conn_dims[3]; ++x, ++i) {
          // Out-of-bounds check:
          if (!((z + nhood_data[d * nhood_dims[1] + 0] < 0)
              ||(z + nhood_data[d * nhood_dims[1] + 0] >= conn_dims[1])
              ||(y + nhood_data[d * nhood_dims[1] + 1] < 0)
              ||(y + nhood_data[d * nhood_dims[1] + 1] >= conn_dims[2])
              ||(x + nhood_data[d * nhood_dims[1] + 2] < 0)
              ||(x + nhood_data[d * nhood_dims[1] + 2] >= conn_dims[3]))) {
              ++nEdge;
          }
        }
      }
    }
  }
    /* Sort all the edges in increasing order of weight */
    std::vector< int > pqueue( nEdge );
    int j = 0;
      // Loop over #edges
      for (int d = 0, i = 0; d < conn_dims[0]; ++d) {
        // Loop over Z
        for (int z = 0; z < conn_dims[1]; ++z) {
          // Loop over Y
          for (int y = 0; y < conn_dims[2]; ++y) {
            // Loop over X
            for (int x = 0; x < conn_dims[3]; ++x, ++i) {
              // Out-of-bounds check:
              if (!((z + nhood_data[d * nhood_dims[1] + 0] < 0)
                  ||(z + nhood_data[d * nhood_dims[1] + 0] >= conn_dims[1])
                  ||(y + nhood_data[d * nhood_dims[1] + 1] < 0)
                  ||(y + nhood_data[d * nhood_dims[1] + 1] >= conn_dims[2])
                  ||(x + nhood_data[d * nhood_dims[1] + 2] < 0)
                  ||(x + nhood_data[d * nhood_dims[1] + 2] >= conn_dims[3]))) {
                    pqueue[j++] = i;
              }
            }
          }
        }
      }
    unsigned long nValidEdge = j;
    pqueue.resize(nValidEdge);
    sort( pqueue.begin(), pqueue.end(), AffinityGraphCompare<float>( edgeWeight ) );

    /* Start MST */
    int minEdge, e, v1, v2;
    uint64_t set1, set2;
    uint64_t nPair = 0;
    map<uint64_t,uint64_t>::iterator it1, it2;

    /* Start Kruskal's */
    for (unsigned int i = 0; i < pqueue.size(); ++i ) {
        minEdge = pqueue[i];
        e =  minEdge / nVert;
        // v1: node at edge beginning
        v1 = minEdge % nVert;
        // v2: neighborhood node at edge e
        v2 = v1 + nhood_data[e];

        set1 = dsets.find_set(v1);
        set2 = dsets.find_set(v2);

        if (set1!=set2){
            dsets.link(set1, set2);

            /* compute the number of pairs merged by this MST edge */
            for (it1 = overlap[set1].begin();
                    it1 != overlap[set1].end(); ++it1) {
                for (it2 = overlap[set2].begin();
                        it2 != overlap[set2].end(); ++it2) {

                    nPair = it1->second * it2->second;

                    if (pos && (it1->first == it2->first)) {
                        nPairPerEdge[minEdge] += nPair;
                    } else if ((!pos) && (it1->first != it2->first)) {
                        nPairPerEdge[minEdge] += nPair;
                    }
                }
            }

            /* move the pixel bags of the non-representative to the representative */
            if (dsets.find_set(set1) == set2) // make set1 the rep to keep and set2 the rep to empty
                swap(set1,set2);

            it2 = overlap[set2].begin();
            while (it2 != overlap[set2].end()) {
                it1 = overlap[set1].find(it2->first);
                if (it1 == overlap[set1].end()) {
                    overlap[set1].insert(pair<uint64_t,uint64_t>(it2->first,it2->second));
                } else {
                    it1->second += it2->second;
                }
                overlap[set2].erase(it2++);
            }
        } // end link

    } // end while
}


void connected_components_cpp(const int nVert,
               const int nEdge, const uint64_t* node1, const uint64_t* node2, const int* edgeWeight,
               uint64_t* seg){

    /* Make disjoint sets */
    vector<uint64_t> rank(nVert);
    vector<uint64_t> parent(nVert);
    boost::disjoint_sets<uint64_t*, uint64_t*> dsets(&rank[0],&parent[0]);
    for (int i=0; i<nVert; ++i)
        dsets.make_set(i);

    /* union */
    for (int i = 0; i < nEdge; ++i )
         // check bounds to make sure the nodes are valid
        if ((edgeWeight[i]!=0) && (node1[i]>=0) && (node1[i]<nVert) && (node2[i]>=0) && (node2[i]<nVert))
            dsets.union_set(node1[i],node2[i]);

    /* find */
    for (int i = 0; i < nVert; ++i)
        seg[i] = dsets.find_set(i);
}


void marker_watershed_cpp(const int nVert, const uint64_t* marker,
               const int nEdge, const uint64_t* node1, const uint64_t* node2, const float* edgeWeight,
               uint64_t* seg){

    /* Make disjoint sets */
    vector<uint64_t> rank(nVert);
    vector<uint64_t> parent(nVert);
    boost::disjoint_sets<uint64_t*, uint64_t*> dsets(&rank[0],&parent[0]);
    for (uint64_t i=0; i<nVert; ++i)
        dsets.make_set(i);

    /* initialize output array and find representatives of each class */
    std::map<uint64_t,uint64_t> components;
    for (uint64_t i=0; i<nVert; ++i){
        seg[i] = marker[i];
        if (seg[i] > 0)
            components[seg[i]] = i;
    }

    // merge vertices labeled with the same marker
    for (uint64_t i=0; i<nVert; ++i)
        if (seg[i] > 0)
            dsets.union_set(components[seg[i]],i);

    /* Sort all the edges in decreasing order of weight */
    std::vector<int> pqueue( nEdge );
    int j = 0;
    for (int i = 0; i < nEdge; ++i)
        if ((edgeWeight[i]!=0) &&
            (node1[i]>=0) && (node1[i]<nVert) &&
            (node2[i]>=0) && (node2[i]<nVert) &&
            (marker[node1[i]]>=0) && (marker[node2[i]]>=0))
                pqueue[ j++ ] = i;
    unsigned long nValidEdge = j;
    pqueue.resize(nValidEdge);
    sort( pqueue.begin(), pqueue.end(), AffinityGraphCompare<float>( edgeWeight ) );

    /* Start MST */
    int e;
    int set1, set2, label_of_set1, label_of_set2;
    for (unsigned int i = 0; i < pqueue.size(); ++i ) {
        e = pqueue[i];
        set1=dsets.find_set(node1[e]);
        set2=dsets.find_set(node2[e]);
        label_of_set1 = seg[set1];
        label_of_set2 = seg[set2];

        if ((set1!=set2) &&
            ( ((label_of_set1==0) && (marker[set1]==0)) ||
             ((label_of_set2==0) && (marker[set1]==0))) ){

            dsets.link(set1, set2);
            // either label_of_set1 is 0 or label_of_set2 is 0.
            seg[dsets.find_set(set1)] = std::max(label_of_set1,label_of_set2);
            
        }

    }

    // write out the final coloring
    for (int i=0; i<nVert; i++)
        seg[i] = seg[dsets.find_set(i)];

}
