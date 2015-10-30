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

#include <boost/thread.hpp>

#include "container.hpp"
#include "native.hpp"
#include <cstring>
#include <sys/time.h>

static inline void pmemset(char * ptr, int value, size_t num) {
  const size_t cutoff = 100000;
  if (num <= cutoff) {
    std::memset(ptr, value, num);
  } else {
    long m = num/2;
    pasl::sched::native::fork2([&] {
      pmemset(ptr, value, m);
    }, [&] {
      pmemset(ptr+m, value, num-m);
    });
  }
}

template <class Number, class Size>
void fill_array_par(std::atomic<Number>* array, Size sz, Number val) {
  pmemset((char*)array, val, sz*sizeof(Number));
}

template <class Body>
void print(bool debug, const Body& b) {
  if (debug) {
    pasl::util::atomic::msg(b);
  }
}

uint64_t GetTimeStamp() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

template <class GRAPH, class HEAP, class HEURISTIC>
std::atomic<int>* pwsa(GRAPH& graph, const HEURISTIC& heuristic,
                        const int& source, const int& destination,
                        int split_cutoff, int poll_cutoff, double exptime,
                        bool debug = false, int* pebbles = nullptr,
                        int* predecessors = nullptr) {
  int N = graph.number_vertices();
  std::atomic<int>* finalized = pasl::data::mynew_array<std::atomic<int>>(N);
  fill_array_par(finalized, N, -1);

  HEAP initF = HEAP();
  int heur = heuristic(source);
  initF.insert(heur, std::make_tuple(source, 0, 0));

  pasl::data::perworker::array<int> work_since_split;
  work_since_split.init(0);

  auto size = [&] (HEAP& frontier) {
    auto sz = frontier.size();
    if (sz == 0) {
      work_since_split.mine() = 0;
      return 0; // no work left
    }
    if (sz > split_cutoff || (work_since_split.mine() > split_cutoff && sz > 1)) {
      return 2; // split
    }
    else {
      return 1; // don't split
    }
  };

  auto fork = [&] (HEAP& src, HEAP& dst) {
    src.split(dst);
    work_since_split.mine() = 0;
  };

  auto set_in_env = [&] (HEAP& f) {;};

  auto do_work = [&] (HEAP& frontier) {
    int work_this_round = 0;
    while (work_this_round < poll_cutoff && frontier.size() > 0) {
      auto tup = frontier.delete_min();
      int v = std::get<0>(tup);
      int vdist = std::get<1>(tup);
      int pred = std::get<2>(tup);
      int orig = -1;
      if (finalized[v].load() == -1 && finalized[v].compare_exchange_strong(orig, vdist)) {
        if (pebbles) pebbles[v] = pasl::sched::threaddag::get_my_id();
        if (predecessors) predecessors[v] = pred;
        if (v == destination) {
          return true;
        }
        graph.for_each_neighbor_of(v, [&] (int ngh, int weight) {
          int nghdist = vdist + weight;
          frontier.insert(heuristic(ngh) + nghdist, std::make_tuple(ngh, nghdist, v));

          // SIMULATE EXPANSION TIME
          uint64_t t0 = GetTimeStamp();
          uint64_t t1;
          uint64_t dt;
          do{
            t1 = GetTimeStamp();
            dt = t1-t0;
          } while(dt < (exptime*1000000.0));
        });
      }
      work_this_round++;
    }
    work_since_split.mine() += work_this_round;
    return false;
  };

  pasl::sched::native::parallel_while_pwsa(initF, size, fork, set_in_env, do_work);
  return finalized;
}

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

    //vector<int> succ_ids;
    //vector<pARAState*> succ_ptrs;
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
