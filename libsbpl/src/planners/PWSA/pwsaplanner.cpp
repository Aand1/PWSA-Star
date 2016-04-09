#include <iostream>

#include <sbpl/headers.h>

using namespace std;

uint64_t GetTS_pwsa() {
  struct timeval tv; 
  gettimeofday(&tv,NULL);
  return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

pwsa_state* pwsaPlanner::GetState(int id) {
  bool orig = false;
  pwsa_state* state = states->getState(id);
  if (state->h == -1) {
    state->h = environment_->GetGoalHeuristic(id);
  }
  return state;
}

// Note that we only call f on a particular succ if we can decrease
// the g-value to the succ. The check to see whether a node is actually
// visited or not is done inside of pwsa_impl 
template <class FUNC>
void pwsaPlanner::for_each_neighbor_of(int v, const FUNC& f) {
  vector<int> SuccIDV;
  vector<int> CostV;

  environment_->GetSuccs(v, &SuccIDV, &CostV);
  //iterate through successors of the parent
  for(int sind = 0; sind < (int)SuccIDV.size(); sind++) {

    int nghId = SuccIDV[sind];
    int weight = CostV[sind];

    f(nghId, weight);
  }
}

void pwsaPlanner::run_pwsa() {
  int split_cutoff = 5; // k
  int poll_cutoff = 5; // d

  int src = start_state_id;
  int dst = goal_state_id;

  int* predecessors = nullptr;

  auto init = [&] {;};

  double captured_eps = eps;
  auto run = [&] (bool sequential) {
    auto heuristic = [&] (pwsa_state* state) {
      return (int) (captured_eps * state->h);
    };

    cout << "launched\n";
    pwsa_pc<pwsaPlanner, Heap<int>, decltype(heuristic)>(self(), heuristic, 
                                                         src, dst, split_cutoff, 
                                                         poll_cutoff, 0.0, false);
    cout << "done\n";
  };

  auto output = [&] {;};

  auto destroy = [&] {;};

  printf("running with %d threads\n", num_threads);

  pasl::sched::launch(init, run, output, destroy, num_threads);
  printf("[sameer] pasl sched done");
}

pwsaPlanner::pwsaPlanner(DiscreteSpaceInformation* environment, bool bforwardsearch) :
  params(0.0) {

  printf("initializing pwsaPlanner!\n");
  environment_ = environment;

  fout = fopen("stats.txt","w");

  goal_state_id = -1;
  start_state_id = -1;
}

// Cleanup
pwsaPlanner::~pwsaPlanner() {
  fclose(fout);
}

vector<int> pwsaPlanner::GetSearchPath(int& solcost) {
  vector<int> SuccIDV;
  vector<int> CostV;
  vector<int> wholePathIds;

  pwsa_state* state;
  // assumes bforwardstate
  state = goal_state;
  int cur_id = goal_state_id;

  wholePathIds.push_back(cur_id);
  solcost = 0;

  while(cur_id != start_state_id) {
    vertpack gpred = state->gpred.load();
    if (gpred.pred == -1) {
      printf("a state along the path has no parent!\n");
      break;
    }
    if (gpred.distance == INT_MAX) {
      printf("a state along the path has an infinite g-value!\n");
      break;
    }

    environment_->GetSuccs(gpred.pred, &SuccIDV, &CostV);
    int actioncost = INFINITECOST;
    for(unsigned int i=0; i<SuccIDV.size(); i++) {
      if(SuccIDV[i] == cur_id && CostV[i] < actioncost) {
        actioncost = CostV[i];
      }
    }
    if(actioncost == INFINITECOST) {
      printf("WARNING: actioncost = %d\n", actioncost);
      std::cin.get();
    }
    solcost += actioncost;

    state = GetState(gpred.pred);
    cur_id = gpred.pred;
    wholePathIds.push_back(cur_id);
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

  //ensure heuristics are up-to-date
  environment_->EnsureHeuristicsUpdated(true);
}


bool pwsaPlanner::Search(vector<int>& pathIds, int& PathCost) {
  CKey key;
  TimeStarted = GetTS_pwsa();

  initializeSearch();

  uint64_t before_time = GetTS_pwsa();

  run_pwsa();

  double delta_time = double(GetTS_pwsa() - before_time)/1000000.0;
  totalPlanTime += delta_time;

  if (GetState(goal_state_id)->gpred.load().distance == INT_MAX) {
    printf("Could not find a solution --- ran out of time\n");
    return false;
  }

  printf("solution found\n");
  uint64_t before_reconstruct = GetTS_pwsa();
  pathIds = GetSearchPath(PathCost);
  reconstructTime = 0.0;
  reconstructTime = double(GetTS_pwsa()-before_reconstruct)/1000000.0;
  totalTime = totalPlanTime + reconstructTime;

  cout << "computing total expands\n";
  


  return true;
}

//-----------------------------Interface function-----------------------------------------------------

int pwsaPlanner::replan(vector<int>* solution_stateIDs_V, ReplanParams p, int* solcost) {
  return replan(start_state_id, goal_state_id, solution_stateIDs_V, p, solcost);
}

int pwsaPlanner::replan(int start, int goal, vector<int>* solution_stateIDs_V, ReplanParams p, int* solcost) {

  states = new(ubvec<pwsa_state>);

  printf("planner: replan called\n");
  set_start(start);
  set_goal(goal);
  params = p;
  printf("params.inital_eps = %lf \n", params.initial_eps);
  eps = params.initial_eps;
  num_threads = params.num_threads;

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
  std::atomic<bool>* exps;

  bool solnFound = Search(pathIds, PathCost);

  printf("Final pathcost = %d\n", PathCost);


  printf("total expands=%d planning time=%.3f reconstruct path time=%.3f total time=%.3f solution cost=%d\n", 
      totalExpands, totalPlanTime, reconstructTime, totalTime, goal_state->gpred.load().distance);

  fprintf(fout, "%d %f %d %d\n", 4, totalPlanTime, totalExpands, goal_state->gpred.load().distance);

  //copy the solution
  *solution_stateIDs_V = pathIds;
  *solcost = PathCost;

  start_state_id = -1;
  goal_state_id = -1;

  printf("returning from replan\n");

  delete(states);

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

////////////////////ASTAR//////////////////////


astarState* wAStarPlanner::GetState(int id) {
  astarState* state = states->getState(id);
  if (!state->initialized) {
    state->h = environment_->GetGoalHeuristic(id);
    state->initialized = true;
  }
  return state;
}

// Note that we only call f on a particular succ if we can decrease
// the g-value to the succ. The check to see whether a node is actually
// visited or not is done inside of pwsa_impl 
template <class FUNC>
void wAStarPlanner::for_each_neighbor_of(int v, const FUNC& f) {
  vector<int> SuccIDV;
  vector<int> CostV;

  astarState* parent = GetState(v);

  environment_->GetSuccs(v, &SuccIDV, &CostV);
  //iterate through successors of the parent
  for(int sind = 0; sind < (int)SuccIDV.size(); sind++) {

    int nghId = SuccIDV[sind];
    int weight = CostV[sind];

    f(nghId, weight);
  }
}

void wAStarPlanner::run_wAStar() {

  int src = start_state_id;
  int dst = goal_state_id;


  auto init = [&] {;};

  auto run = [&] (bool sequential) {
    auto heuristic = [&] (int v) {
      return eps * GetState(v)->h;
    };

    wAStar<wAStarPlanner, Heap<int>, decltype(heuristic)>(self(), heuristic, src, dst, *states);
  };

  auto output = [&] {;};

  auto destroy = [&] {;};

  pasl::sched::launch(init, run, output, destroy, num_threads);
}

wAStarPlanner::wAStarPlanner(DiscreteSpaceInformation* environment, bool bforwardsearch) :
  params(0.0) {

  printf("initializing wAStarPlanner!\n");
  environment_ = environment;

  fout = fopen("stats.txt","w");

  goal_state_id = -1;
  start_state_id = -1;
}

// Cleanup
wAStarPlanner::~wAStarPlanner() {
  fclose(fout);
}

vector<int> wAStarPlanner::GetSearchPath(int& solcost){
  vector<int> SuccIDV;
  vector<int> CostV;
  vector<int> wholePathIds;

  astarState* state;
  // assumes bforwardstate
  state = goal_state;
  int cur_id = goal_state_id;

  wholePathIds.push_back(cur_id);
  solcost = 0;

  while(cur_id != start_state_id) {
    if (state->pred == -1) {
      printf("a state along the path has no parent!\n");
      break;
    }
    if (state->g == INT_MAX) {
      printf("a state along the path has an infinite g-value!\n");
      break;
    }

    printf("cur id = %d pred = %d\n", cur_id, state->pred);

    environment_->GetSuccs(state->pred, &SuccIDV, &CostV);
    printf("cur id = %d pred = %d\n", cur_id, state->pred);
    int actioncost = INFINITECOST;
    for(unsigned int i=0; i<SuccIDV.size(); i++) {
      if(SuccIDV[i] == cur_id && CostV[i] < actioncost) {
        actioncost = CostV[i];
      }
    }
    if(actioncost == INFINITECOST) {
      printf("WARNING: actioncost = %d\n", actioncost);
      std::cin.get();
    }
    solcost += actioncost;

    cur_id = state->pred;
    state = GetState(cur_id);
    wholePathIds.push_back(cur_id);
  }

  // assumes bforwardstate
  for(unsigned int i=0; i<wholePathIds.size()/2; i++) {
    int other_idx = wholePathIds.size()-i-1;
    int temp = wholePathIds[i];
    wholePathIds[i] = wholePathIds[other_idx];
    wholePathIds[other_idx] = temp;
  }

  return wholePathIds;
}

void wAStarPlanner::initializeSearch() {
  printf("in initialize\n");

  totalExpands = 0;
  totalPlanTime = 0;
  reconstructTime = 0.0;
  
  //call get state to initialize the start and goal states
  goal_state = GetState(goal_state_id);
  start_state = GetState(start_state_id);

  start_state->g = 0;

  //ensure heuristics are up-to-date
  environment_->EnsureHeuristicsUpdated(true);
}

bool wAStarPlanner::Search(vector<int>& pathIds, int& PathCost) {
  CKey key;
  TimeStarted = GetTS_pwsa();

  initializeSearch();

  uint64_t before_time = GetTS_pwsa();

  run_wAStar();

  double delta_time = double(GetTS_pwsa() - before_time)/1000000.0;
  totalPlanTime += delta_time;

  if (GetState(goal_state_id)->g == INT_MAX) {
    printf("Could not find a solution --- ran out of time\n");
    return false;
  }

  printf("solution found\n");
  uint64_t before_reconstruct = GetTS_pwsa();
  pathIds = GetSearchPath(PathCost);
  reconstructTime = 0.0;
  reconstructTime = double(GetTS_pwsa()-before_reconstruct)/1000000.0;
  totalTime = totalPlanTime + reconstructTime;

  return true;
}

//-----------------------------Interface function-----------------------------------------------------

int wAStarPlanner::replan(vector<int>* solution_stateIDs_V, ReplanParams p, int* solcost) {
  return replan(start_state_id, goal_state_id, solution_stateIDs_V, p, solcost);
}

int wAStarPlanner::replan(int start, int goal, vector<int>* solution_stateIDs_V, ReplanParams p, int* solcost) {

  states = new(ubvec<astarState>);

  printf("planner: replan called\n");
  set_start(start);
  set_goal(goal);
  params = p;
  printf("params.inital_eps = %lf \n", params.initial_eps);
  eps = params.initial_eps;
  num_threads = params.num_threads;

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
  std::atomic<bool>* exps;

  bool solnFound = Search(pathIds, PathCost);

  printf("Final pathcost = %d\n", PathCost);


  printf("total expands=%d planning time=%.3f reconstruct path time=%.3f total time=%.3f solution cost=%d\n", 
      totalExpands, totalPlanTime, reconstructTime, totalTime, goal_state->g);

  fprintf(fout, "%d %f %d %d\n", 4, totalPlanTime, totalExpands, goal_state->g);

  //copy the solution
  *solution_stateIDs_V = pathIds;
  *solcost = PathCost;

  start_state_id = -1;
  goal_state_id = -1;

  printf("returning from replan\n");

  delete states;

  return (int)solnFound;
}

// assumes bforward state
int wAStarPlanner::set_goal(int id){
  printf("planner: setting goal to %d\n", id);
  goal_state_id = id;
  return 1;
}

// assumes bforward state
int wAStarPlanner::set_start(int id){
  printf("planner: setting start to %d\n", id);
  start_state_id = id;
  return 1;
}

