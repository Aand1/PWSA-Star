#ifndef __PWSA_PLANNER_H_
#define __PWSA_PLANNER_H_

#include <sbpl/planners/pwsa.h>
#include <sbpl/planners/ubvec.h>
#include <sbpl/planners/lax_utils.h>
#include <sbpl/planners/bin_heap.h>
#include <sbpl/planners/weighted_graph.h>

#include <pasl/sched/benchmark.hpp>

#include <cstring>

class pwsaPlanner : public SBPLPlanner {

public:
  pwsaPlanner(DiscreteSpaceInformation* environment, bool bforwardsearch);
  ~pwsaPlanner();

  virtual int replan(int start, int goal, vector<int>* solution_stateIDs_V, ReplanParams params, int* solcost);
  virtual int replan(std::vector<int>* solution_stateIDs_V, ReplanParams params, int* solcost);

  virtual int set_goal(int goal_stateID);
  virtual int set_start(int start_stateID);

  template <class FUNC>
  void for_each_neighbor_of(int v, const FUNC& f);

  void run_pwsa();

	virtual pwsa_state* GetState(int id);

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

  pwsaPlanner& self() { return *this; }

  //params
  ReplanParams params;
	pwsa_state* goal_state;
	pwsa_state* start_state;
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

  ubvec<pwsa_state>* states;

  //stats
  vector<PlannerStats> stats;
  unsigned int totalExpands;
  double totalTime;
  double totalPlanTime;
  double reconstructTime;


	virtual vector<int> GetSearchPath(int& solcost);

  virtual void initializeSearch();
	virtual bool Search(vector<int>& pathIds, int & PathCost);
};

class astarState {
  public:
    int pred; // id of node
    unsigned int g; // g-value
    bool expanded;
    bool initialized;
    int h; // heuristic

    astarState() {
      pred = -1;
      expanded = false;
      initialized = false;
      g = INT_MAX;
    }
};

template <class GRAPH, class HEAP, class HEURISTIC>
void wAStar(GRAPH& graph, const HEURISTIC& heuristic,
                            const int& source, const int& destination, 
                            ubvec<astarState>& states) {
  astarState* src = graph.GetState(source);
  src->g = 0;
  src->pred = source;
  HEAP frontier = HEAP();
  frontier.insert(heuristic(source), source);

  astarState* dest = graph.GetState(destination);

  while (dest->g == INT_MAX) {
    int v = frontier.delete_min();
    astarState* vState = graph.GetState(v);
    if (!vState->expanded) {
      vState->expanded = true;

      graph.for_each_neighbor_of(v, [&] (int ngh, int weight) {
        int nghdist = vState->g + weight;
        astarState* nghState = graph.GetState(ngh);
        if (nghdist < nghState->g) {
          frontier.insert(heuristic(ngh) + nghdist, ngh);
          nghState->pred = v;
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
	
  virtual astarState* GetState(int id);


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

  ubvec<astarState>* states;

  //stats
  vector<PlannerStats> stats;
  unsigned int totalExpands;
  double totalTime;
  double totalPlanTime;
  double reconstructTime;


	virtual vector<int> GetSearchPath(int& solcost);

  virtual void initializeSearch();
	virtual bool Search(vector<int>& pathIds, int & PathCost);
};

#endif
