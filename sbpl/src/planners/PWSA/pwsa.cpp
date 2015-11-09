#include <iostream>

#include "pwsa.h"
#include "pwsa_impl.hpp"
#include "bin_heap.hpp"
#include "weighted_graph.hpp"

#include "../../../pasl/sched/benchmark.hpp"

using namespace std;

uint64_t GetTimeStamp() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

pwsaState* pwsaPlanner::GetState(int id) {
  //if this stateID is out of bounds of our state vector then grow the list
  if(id >= int(states.capacity())) {
    std::cout << "id = " << id;
    throw std::runtime_error("reached MAX_N. Impl a ub vector of states");
  }
  int orig = -1;
  if(initialized[id].load() == -1 && initialized[id].compare_exchange_strong(orig, 0)) {
    states[id] = new pwsaState();
    states[id]->id = id;
    states[id]->g = INFINITECOST;
    states[id]->v = INFINITECOST;
    states[id]->best_parent = NULL;
    states[id]->h = environment_->GetGoalHeuristic(id);
  }
  pwsaState* s = states[id];
  return s;
}

// Note that we only call f on a particular succ if we can decrease
// the g-value to the succ. The check to see whether a node is actually
// visited or not is done inside of pwsa_impl 
template <class FUNC>
void pwsaPlanner::for_each_neighbor_of(int v, const FUNC& f) {
  vector<int> SuccIDV;
  vector<int> CostV;

  // Expanding parent, lock unnecessary as we have CAS in the algo
  pwsaState* parent = states[v];
  parent->v = parent->g;

  environment_->GetSuccs(v, &SuccIDV, &CostV);
  //iterate through successors of the parent
  for(int sind = 0; sind < (int)SuccIDV.size(); sind++) {
    pwsaState* succ = GetState(SuccIDV[sind]);

    int weight = CostV[sind];

    // This condition doesn't put succ into our heap if it's been
    // visited by some other proc (or possibly us) already EVEN if
    // we could get a shorter path to g. This is because we disallow
    // re-expansions.
    if (succ->g > parent->v + weight) {
      // lock, doing mutable updates
      boost::unique_lock<boost::mutex> lock(succ->mutex);
      succ->g = parent->v + weight;
      succ->best_parent = parent;

      f(succ->id, weight);

      lock.unlock();
    }
  }
}

void pwsaPlanner::run_pwsa() {
  int split_cutoff = 10; // k
  int poll_cutoff = 5; // d

  int src = start_state->id;
  int dst = goal_state->id;

  std::atomic<int>* res;
  int* pebbles = nullptr;
  int* predecessors = nullptr;

  auto init = [&] {;};

  auto run = [&] (bool sequential) {
    auto heuristic = [&] (int v) {
      return eps * states[v]->h;
    };
    res = pwsa<pwsaPlanner, Heap<std::tuple<int,int,int>>, decltype(heuristic)>(self(), heuristic, src, dst, split_cutoff, poll_cutoff, pebbles, predecessors);
  };

  auto output = [&] {;};

  auto destroy = [&] {;};

  printf("launching pwsa \n");
  pasl::sched::launch(init, run, output, destroy);
}

pwsaPlanner::pwsaPlanner(DiscreteSpaceInformation* environment, int eps_) :
  params(0.0) {
  environment_ = environment;

  eps = eps_;

  fout = fopen("stats.txt","w");

  goal_state_id = -1;
  start_state_id = -1;

  initialized = pasl::data::mynew_array<std::atomic<int>>(MAX_N);
  std::memset(initialized, -1, MAX_N);

  states.reserve(MAX_N);
}

// Cleanup
pwsaPlanner::~pwsaPlanner() {
  fclose(fout);
}

vector<int> pwsaPlanner::GetSearchPath(int& solcost){
  vector<int> SuccIDV;
  vector<int> CostV;
  vector<int> wholePathIds;

  pwsaState* state;
  pwsaState* final_state;

  // assumes bforwardstate
  state = goal_state;
  final_state = start_state;

  wholePathIds.push_back(state->id);
  solcost = 0;

  while(state->id != final_state->id){
    if(state->best_parent == NULL){
      printf("a state along the path has no parent!\n");
      break;
    }
    if(state->g == INFINITECOST){
      printf("a state along the path has an infinite g-value!\n");
      break;
    }

    environment_->GetSuccs(state->best_parent->id, &SuccIDV, &CostV);
    int actioncost = INFINITECOST;
    for(unsigned int i=0; i<SuccIDV.size(); i++){
      if(SuccIDV[i] == state->id && CostV[i]<actioncost)
        actioncost = CostV[i];
    }
    if(actioncost == INFINITECOST){
      printf("WARNING: actioncost = %d\n", actioncost);
      std::cin.get();
    }
    solcost += actioncost;

    state = state->best_parent;
    wholePathIds.push_back(state->id);
  }

  // assumes bforwardstate
  for(unsigned int i=0; i<wholePathIds.size()/2; i++){
    int other_idx = wholePathIds.size()-i-1;
    int temp = wholePathIds[i];
    wholePathIds[i] = wholePathIds[other_idx];
    wholePathIds[other_idx] = temp;
  }

  return wholePathIds;
}

void pwsaPlanner::initializeSearch() {
  printf("in initialize\n");

  totalExpands = 0;
  totalPlanTime = 0;
  reconstructTime = 0.0;
  
  //call get state to initialize the start and goal states
  goal_state = GetState(goal_state_id);
  start_state = GetState(start_state_id);

  printf("start state = %d\n", start_state->id);
  start_state->g = 0;

  //ensure heuristics are up-to-date
  environment_->EnsureHeuristicsUpdated(true);
}

bool pwsaPlanner::Search(vector<int>& pathIds, int& PathCost){
  CKey key;
  TimeStarted = GetTimeStamp();

  initializeSearch();

  uint64_t before_time = GetTimeStamp();
  run_pwsa();
  double delta_time = double(GetTimeStamp() - before_time)/1000000.0;
  totalPlanTime += delta_time;

  if (goal_state->g == INFINITECOST) {
    printf("could not find a solution (ran out of time)\n");
    return false;
  }

  printf("solution found\n");
  uint64_t before_reconstruct = GetTimeStamp();
  pathIds = GetSearchPath(PathCost);
  reconstructTime = 0.0;
  reconstructTime = double(GetTimeStamp()-before_reconstruct)/1000000.0;
  totalTime = totalPlanTime + reconstructTime;

  return true;
}

//-----------------------------Interface function-----------------------------------------------------
int pwsaPlanner::replan(int start, int goal, vector<int>* solution_stateIDs_V, ReplanParams p, int* solcost){
  printf("planner: replan called\n");
  set_start(start);
  set_goal(goal);
  params = p;
  use_repair_time = params.repair_time >= 0;

  if(goal_state_id < 0){
    printf("ERROR searching: no goal state set\n");
    return 0;
  }
  if(start_state_id < 0){
    printf("ERROR searching: no start state set\n");
    return 0;
  }

  //plan
  vector<int> pathIds; 
  int PathCost;
  bool solnFound = Search(pathIds, PathCost);

  for (int i = 0; i < MAX_N; i++) {
    pwsaState* state = states[i];
    if (state != NULL) {
      if (state->v != INFINITECOST) {
        totalExpands++;
      }
    }
  }

  printf("total expands=%d planning time=%.3f reconstruct path time=%.3f total time=%.3f solution cost=%d\n", 
      totalExpands, totalPlanTime, reconstructTime, totalTime, goal_state->g);

  fprintf(fout, "%d %f %d %d\n", 4, totalPlanTime, totalExpands, goal_state->g);

  //copy the solution
  *solution_stateIDs_V = pathIds;
  *solcost = PathCost;

  start_state_id = -1;
  goal_state_id = -1;

  return (int)solnFound;
}

// assumes bforward state
int pwsaPlanner::set_goal(int id){
  printf("planner: setting goal to %d\n", id);
  goal_state_id = id;
  return 1;
}

// assumes bforward state
int pwsaPlanner::set_start(int id){
  printf("planner: setting start to %d\n", id);
  start_state_id = id;
  return 1;
}


