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
#ifndef __P_ARAPLANNER_H_
#define __P_ARAPLANNER_H_

#include "../../sbpl/headers.h"

#include <boost/thread.hpp>

class pARAState{
  public:
    int id;
    unsigned int v;
    unsigned int g;
    int h;
    short unsigned int iteration_closed;
    short unsigned int replan_number;
    pARAState* best_parent;
    bool in_incons;
    multimap<int,pARAState*>::iterator iter;
};

class myHeap{
  public:
    myHeap(vector<pARAState*>* expanding, bool* done_flag, DiscreteSpaceInformation* e);
    void clear();
    void setEps(double e);
    bool empty();
    void initIter(pARAState* s);
    void insert(pARAState* s, int key);
    int min_value();
    void analyze();
    pARAState* remove(boost::unique_lock<boost::mutex>* lock, int* fval, int thread_id);

  private:
    multimap<int,pARAState*> m;
    vector<pARAState*>* being_expanded;
    bool* done;
    DiscreteSpaceInformation* env;
    double eps;
};



class pARAPlanner : public SBPLPlanner{

public:
	virtual int replan(double allocated_time_secs, vector<int>* solution_stateIDs_V){
    printf("Not supported. Use ReplanParams");
    return -1;
  };
	virtual int replan(double allocated_time_sec, vector<int>* solution_stateIDs_V, int* solcost){
    printf("Not supported. Use ReplanParams");
    return -1;
  };

  virtual int replan(int start, int goal, vector<int>* solution_stateIDs_V, ReplanParams params, int* solcost);
  virtual int replan(std::vector<int>* solution_stateIDs_V, ReplanParams params);
  virtual int replan(std::vector<int>* solution_stateIDs_V, ReplanParams params, int* solcost);

  virtual int set_goal(int goal_stateID);
  virtual int set_start(int start_stateID);

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

  pARAPlanner(DiscreteSpaceInformation* environment, bool bforwardsearch);
  ~pARAPlanner();

  virtual void get_search_stats(vector<PlannerStats>* s);

  void checkConnections();
  bool checkConnection(pARAState* s);

protected:
  FILE* fout;
  boost::mutex mutex;
  boost::condition_variable worker_cond;
  boost::condition_variable main_cond;
  bool planner_ok;
  bool iteration_done;
  int sleeping_threads;
  int thread_ids;
  vector<boost::thread*> threads;
  vector<pARAState*> being_expanded;
  vector<int> being_expanded_fval;
  int improve_path_result;
  int bad_cnt;

  //data structures (open and incons lists)
	//CHeap heap;
  myHeap heap;
  vector<pARAState*> incons;
  vector<pARAState*> states;

  //params
  ReplanParams params;
	bool bforwardsearch; //if true, then search proceeds forward, otherwise backward
	pARAState* goal_state;
	pARAState* start_state;
  int goal_state_id;
  int start_state_id;

  //search member variables
	double eps;
  double eps_satisfied;
  int search_expands;
	uint64_t TimeStarted;
	short unsigned int search_iteration;
	short unsigned int replan_number;
  bool use_repair_time;

  //stats
  vector<PlannerStats> stats;
  unsigned int totalExpands;
  double totalTime;
  double totalPlanTime;
  double reconstructTime;

  void astarThread();

	virtual pARAState* GetState(int id);
	virtual void UpdateSuccs(pARAState* parent);
	virtual void UpdatePreds(pARAState* parent);

	virtual int ImprovePath(int thread_id);

	virtual vector<int> GetSearchPath(int& solcost);

  virtual bool outOfTime();
  virtual void initializeSearch();
  virtual void prepareNextSearchIteration();
	virtual bool Search(vector<int>& pathIds, int & PathCost);

};

#endif
