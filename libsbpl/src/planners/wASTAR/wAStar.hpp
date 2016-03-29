/*
 * Copyright (c) 2013, Mike Phillips and Maxim Likhachev
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of Pennsylvania nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __ASTAR_PLANNER_H_
#define __ASTAR_PLANNER_H_

#include "../../sbpl/headers.h"

#include "../PWSA/bin_heap.hpp"

//#include "../../../pasl/sequtil/container.hpp"
//#include "../../../pasl/sched/native.hpp"

#include <cstring>
#include <climits>
#include <sys/time.h>

#define MAX_N 100000
#define MIN_F 16383
#define NUM_INNER_VECS 32
#define MIN_VEC_SIZE 16384
#define LOG_MIN_VEC_SIZE 14



class astarState {
  public:
    int id; // id of node
    unsigned int g; // g-value
    bool expanded;
    bool initialized;
    int h; // heuristic
    astarState* best_parent; // pointer to node that achieves best val

    astarState() {
      expanded = false;
      initialized = false;
      g = INT_MAX;
    }
};

template <class GRAPH, class HEAP, class HEURISTIC>
void wAStar(GRAPH& graph, const HEURISTIC& heuristic,
                            const int& source, const int& destination, 
                            ubvec<astarState>& states) {
  astarState* src = states.getState(source);
  src->g = 0;
  HEAP frontier = HEAP();
  frontier.insert(heuristic(source), source);

  astarState* dest = states.getState(destination);

  while (dest->g == INT_MAX) {
    int v = frontier.delete_min();
    astarState* vState = states.getState(v);
    if (!vState->expanded) {
      vState->expanded = true;

      graph.for_each_neighbor_of(v, [&] (int ngh, int weight) {
        int nghdist = vState->g + weight;
        astarState* nghState = states.getState(ngh);
        if (nghdist < nghState->g) {
          frontier.insert(heuristic(ngh) + nghdist, ngh);
          nghState->g = nghdist;
        }

      });
    }
  }
}

class wAStarPlanner : public SBPLPlanner {

public:
  wAStarPlanner(DiscreteSpaceInformation* environment, bool bforwardsearch);
  ~wAStarPlanner();

  virtual int replan(int start, int goal, vector<int>* solution_stateIDs_V, ReplanParams params, int* solcost);
  virtual int replan(std::vector<int>* solution_stateIDs_V, ReplanParams params, int* solcost);

  virtual int set_goal(int goal_stateID);
  virtual int set_start(int start_stateID);

  template <class FUNC>
  void for_each_neighbor_of(int v, const FUNC& f);

  void run_wAStar();

  // unimplemented
	virtual int replan(double allocated_time_secs, vector<int>* solution_stateIDs_V){
    printf("Not supported. Use ReplanParams");
    return -1;
  };
	virtual int replan(double allocated_time_sec, vector<int>* solution_stateIDs_V, int* solcost){
    printf("Not supported. Use ReplanParams");
    return -1;
  };
  virtual int replan(std::vector<int>* solution_stateIDs_V, ReplanParams params) {
    printf("Not supported. Use ReplanParams");
    return -1;
  };
  virtual void costs_changed(StateChangeQuery const & stateChange){return;};
  virtual void costs_changed(){return;};
  virtual int force_planning_from_scratch(){return 1;};
  virtual int force_planning_from_scratch_and_free_memory(){return 1;};
	virtual int set_search_mode(bool bSearchUntilFirstSolution){
    printf("Not supported. Use ReplanParams");
    return -1;
  };
	virtual void set_initialsolution_eps(double initialsolution_eps){
    printf("Not supported. Use ReplanParams");
  };


protected:
  FILE* fout;

  wAStarPlanner& self() { return *this; }

  //params
  ReplanParams params;
	astarState* goal_state;
	astarState* start_state;
  int goal_state_id;
  int start_state_id;

  bool iteration_done;

  //search member variables
	double eps;
  int num_threads;
  int search_expands;
	uint64_t TimeStarted;
	short unsigned int search_iteration;
  bool use_repair_time;

  ubvec<astarState> states;

  //stats
  vector<PlannerStats> stats;
  unsigned int totalExpands;
  double totalTime;
  double totalPlanTime;
  double reconstructTime;

	virtual astarState* GetState(int id);

	virtual vector<int> GetSearchPath(int& solcost);

  virtual void initializeSearch();
	virtual bool Search(vector<int>& pathIds, int & PathCost);
};

#endif
