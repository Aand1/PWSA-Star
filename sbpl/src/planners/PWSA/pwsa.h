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
#ifndef __PWSA_PLANNER_H_
#define __PWSA_PLANNER_H_

#include "../../sbpl/headers.h"

#include "../../../pasl/sequtil/container.hpp"
#include "../../../pasl/sched/native.hpp"

#include <boost/thread.hpp>
#include <cstring>


class pwsaState {
  public:
    int id;
    unsigned int v;
    unsigned int g;
    int h;
    pwsaState* best_parent;
    boost::mutex mutex;
};

class pwsaPlanner : public SBPLPlanner{

public:
  pwsaPlanner(DiscreteSpaceInformation* environment, int eps_);
  ~pwsaPlanner();

  virtual int replan(int start, int goal, vector<int>* solution_stateIDs_V, ReplanParams params, int* solcost);
  virtual int set_goal(int goal_stateID);
  virtual int set_start(int start_stateID);

  template <class FUNC>
  void for_each_neighbor_of(int v, const FUNC& f);

  void run_pwsa();

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
  virtual int replan(std::vector<int>* solution_stateIDs_V, ReplanParams params, int* solcost) {
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

  pwsaPlanner& self() { return *this; }

  //params
  ReplanParams params;
	pwsaState* goal_state;
	pwsaState* start_state;
  int goal_state_id;
  int start_state_id;

  bool iteration_done;

  //search member variables
	double eps;
  int search_expands;
	uint64_t TimeStarted;
	short unsigned int search_iteration;
  bool use_repair_time;

  std::atomic<int>* initialized;
  vector<pwsaState*> states;

  //stats
  vector<PlannerStats> stats;
  unsigned int totalExpands;
  double totalTime;
  double totalPlanTime;
  double reconstructTime;

	virtual pwsaState* GetState(int id);

	virtual vector<int> GetSearchPath(int& solcost);

  virtual void initializeSearch();
	virtual bool Search(vector<int>& pathIds, int & PathCost);
};

#endif
