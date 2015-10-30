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
 *     * Neither the name of the Carnegie Mellon University nor the names of its
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
#include "pARA.h"
using namespace std;

//TODO: add mutexes to the environment to protect the hash table
//TODO: implement the new open list

#define num_threads 8

pARAPlanner::pARAPlanner(DiscreteSpaceInformation* environment, bool bSearchForward) :
  params(0.0) {
  bforwardsearch = bSearchForward;
  environment_ = environment;
  replan_number = 0;

  goal_state_id = -1;
  start_state_id = -1;

  boost::unique_lock<boost::mutex> lock(mutex);
  //vector<boost::thread*> threads;
  //vector<pARAState*> being_expanded;
  planner_ok = true;
  sleeping_threads = 0;
  thread_ids = 0;
  for(int i=0; i<num_threads; i++){
    boost::thread* thread = new boost::thread(boost::bind(&pARAPlanner::astarThread, this));
    threads.push_back(thread);
    being_expanded.push_back(NULL);
  }
  main_cond.wait(lock);
}

pARAPlanner::~pARAPlanner(){
  boost::unique_lock<boost::mutex> lock(mutex);
  planner_ok = false;
  worker_cond.notify_all();
  lock.unlock();

  for(int i=0; i<num_threads; i++)
    threads[i]->join();
}

void pARAPlanner::astarThread(){
  boost::unique_lock<boost::mutex> lock(mutex);
  int thread_id = ids;
  thread_ids++;

  while(1){
    //the mutex is locked
    sleeping_threads++;
    if(sleeping_threads==num_threads)
      main_cond.notify_one();
    worker_cond.wait(lock);
    sleeping_threads--;
    lock.unlock();

    if(!planner_ok)
      break;
    
    int ret = ImprovePath(thread_id);
    if(ret>=0)
      improve_path_result = ret;
    lock.lock();
  }
}

pARAState* pARAPlanner::GetState(int id){	
  //if this stateID is out of bounds of our state vector then grow the list
  if(id >= int(states.size())){
    for(int i=states.size(); i<=id; i++)
      states.push_back(NULL);
  }
  //if we have never seen this state then create one
  if(states[id]==NULL){
    states[id] = new pARAState();
    states[id]->id = id;
    states[id]->replan_number = -1;
  }
  //initialize the state if it hasn't been for this call to replan
  pARAState* s = states[id];
  if(s->replan_number != replan_number){
    s->g = INFINITECOST;
    s->v = INFINITECOST;
    s->iteration_closed = -1;
    s->replan_number = replan_number;
    s->best_parent = NULL;
    s->heapindex = 0;
    s->in_incons = false;

    //compute heuristics
    if(bforwardsearch)
      s->h = environment_->GetGoalHeuristic(s->id);
    else
      s->h = environment_->GetStartHeuristic(s->id);
  }
  return s;
}

//used for forward search
void pARAPlanner::UpdateSuccs(pARAState* parent){
  vector<int> SuccIDV;
  vector<int> CostV;
  CKey key;

  environment_->GetSuccs(parent->id, &SuccIDV, &CostV);
  //printf("SuccIDV size=%d\n",SuccIDV.size());

  boost::unique_lock<boost::mutex> lock(mutex);
  //iterate through successors of the parent
  for(int sind = 0; sind < (int)SuccIDV.size(); sind++){
    pARAState* succ = GetState(SuccIDV[sind]);

    //see if we can improve the value of succ
    //taking into account the cost of action
    //printf("g%d > v%d + cost%d\n",succ->g,parent->v,CostV[sind]);
    if(succ->g > parent->v + CostV[sind]){
      succ->g = parent->v + CostV[sind];
      succ->best_parent = parent;
      //printf("better path to state\n");
      //printf("state iteration=%d, search iteration=%d\n",succ->iteration_closed, search_iteration);

      //we only allow one expansion per search iteration
      //so insert into heap if not closed yet
      if(succ->iteration_closed != search_iteration){
        key.key[0] = succ->g + int(eps * succ->h);
        //if the state is already in the heap, just update its priority
        if(succ->heapindex != 0){
          //printf("update heap\n");
          heap.updateheap(succ,key);
        }
        else{ //otherwise add it to the heap
          //printf("insert heap\n");
          heap.insertheap(succ,key);
        }
      }
      //if the state has already been expanded once for this iteration
      //then add it to the incons list so we can keep track of states
      //that we know we have better costs for
      else if(!succ->in_incons){
        incons.push_back(succ);
        succ->in_incons = true;
      }
    } 
  } 
  lock.unlock();
}

void pARAPlanner::UpdatePreds(pARAState* parent){
  vector<int> PredIDV;
  vector<int> CostV;
  CKey key;

  environment_->GetPreds(parent->id, &PredIDV, &CostV);

  //iterate through predecessors of the parent
  for(int pind = 0; pind < (int)PredIDV.size(); pind++){
    pARAState* pred = GetState(PredIDV[pind]);

    //see if we can improve the value of pred
    if(pred->g > parent->v + CostV[pind]){ 
      pred->g = parent->v + CostV[pind];
      pred->best_parent = parent;

      if(pred->iteration_closed != search_iteration){
        key.key[0] = pred->g + int(eps * pred->h);
        if(pred->heapindex != 0)
          heap.updateheap(pred,key);
        else
          heap.insertheap(pred,key);
      }
      //take care of incons list
      else if(!pred->in_incons){
        incons.push_back(pred);
        pred->in_incons = true;
      }
    }
  } 
}

//returns 1 if the solution is found, 0 if the solution does not exist and 2 if it ran out of time
int pARAPlanner::ImprovePath(int thread_id){

  //expand states until done
  boost::scoped_lock<boost::mutex> lock(mutex);
  if(iteration_done)
    return -1;
  CKey min_key = heap.getminkeyheap();
  while(!heap.emptyheap() && 
        min_key.key[0] < INFINITECOST && 
        goal_state->g > min_key.key[0] &&
        !outOfTime()){

    //get the state		
    pARAState* state = (pARAState*)heap.deleteminheap();

    if(state->v == state->g)
      printf("ERROR: consistent state is being expanded\n");

    //mark the state as expanded
    state->v = state->g;
    state->iteration_closed = search_iteration;
    search_expands++;
    being_expanded[thread_id] = state;
    lock.unlock();

    if(bforwardsearch)
      UpdateSuccs(state);
    else
      UpdatePreds(state);

    //if(expands%100000 == 0)
      //printf("expands so far=%u\n", expands);

    being_expanded[thread_id] = NULL;
    lock.lock();

    if(iteration_done)
      return -1;

    //get the min key for the next iteration
    min_key = heap.getminkeyheap();
    //printf("min_key =%d\n",min_key.key[0]);
  }

  //search_expands += expands;
   
  //scoped lock unlocks upon return (goes out of scope)
  iteration_done = true;
  if(goal_state->g == INFINITECOST && (heap.emptyheap() || min_key.key[0] >= INFINITECOST))
    return 0;//solution does not exists
  if(!heap.emptyheap() && goal_state->g > min_key.key[0])
    return 2; //search exited because it ran out of time
  printf("search exited with a solution for eps=%.3f\n", eps);
  return 1;
}

vector<int> pARAPlanner::GetSearchPath(int& solcost){
  vector<int> SuccIDV;
  vector<int> CostV;
  vector<int> wholePathIds;

  pARAState* state;
  pARAState* final_state;
  if(bforwardsearch){
    state = goal_state;
    final_state = start_state;
  }
  else{
    state = start_state;
    final_state = goal_state;
  } 

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

    if(bforwardsearch)
      environment_->GetSuccs(state->best_parent->id, &SuccIDV, &CostV);
    else
      environment_->GetPreds(state->best_parent->id, &SuccIDV, &CostV);
    int actioncost = INFINITECOST;
    for(unsigned int i=0; i<SuccIDV.size(); i++){
      if(SuccIDV[i] == state->id && CostV[i]<actioncost)
        actioncost = CostV[i];
    }
    if(actioncost == INFINITECOST)
      printf("WARNING: actioncost = %d\n", actioncost);
    solcost += actioncost;

    state = state->best_parent;
    wholePathIds.push_back(state->id);
  }

  //if we searched forward then the path reconstruction 
  //worked backward from the goal, so we have to reverse the path
  if(bforwardsearch){
    //in place reverse
    for(unsigned int i=0; i<wholePathIds.size()/2; i++){
      int other_idx = wholePathIds.size()-i-1;
      int temp = wholePathIds[i];
      wholePathIds[i] = wholePathIds[other_idx];
      wholePathIds[other_idx] = temp;
    }
  }

  return wholePathIds;
}

bool pARAPlanner::outOfTime(){
  double time_used = double(clock() - TimeStarted)/CLOCKS_PER_SEC;
  if(time_used >= params.max_time)
    printf("out of max time\n");
  if(use_repair_time && eps_satisfied != INFINITECOST && time_used >= params.repair_time)
    printf("used all repair time...\n");
  //we are out of time if:
         //we used up the max time limit OR
         //we found some solution and used up the minimum time limit
  if(params.return_first_solution)
    return false;
  return time_used >= params.max_time || 
         (use_repair_time && eps_satisfied != INFINITECOST && time_used >= params.repair_time);
}

void pARAPlanner::initializeSearch(){
  //it's a new search, so increment replan_number and reset the search_iteration
  replan_number++;
  search_iteration = 0;
  search_expands = 0;

  //clear open list, incons list, and stats list
  heap.makeemptyheap();
  incons.clear();
  stats.clear();

  //initialize epsilon variable
  eps = params.initial_eps;
  eps_satisfied = INFINITECOST;

  //call get state to initialize the start and goal states
  goal_state = GetState(goal_state_id);
  start_state = GetState(start_state_id);

  //put start state in the heap
  start_state->g = 0;
  CKey key;
  key.key[0] = eps*start_state->h;
  heap.insertheap(start_state, key);

  //ensure heuristics are up-to-date
  environment_->EnsureHeuristicsUpdated((bforwardsearch==true));
}

bool pARAPlanner::Search(vector<int>& pathIds, int& PathCost){
  CKey key;
  TimeStarted = clock();

  initializeSearch();

  //the main loop of ARA*
  while(eps_satisfied > params.final_eps && !outOfTime()){

    //run weighted A*
    clock_t before_time = clock();
    int before_expands = search_expands;

    iteration_done = false;
    boost::unique_lock<boost::mutex> lock(mutex);
    worker_cond.notify_all();
    main_cond.wait(lock);

    int ret = improve_path_result;
    if(ret == 1) //solution found for this iteration
      eps_satisfied = eps;
    int delta_expands = search_expands - before_expands;
    double delta_time = double(clock()-before_time)/CLOCKS_PER_SEC;

    //print the bound, expands, and time for that iteration
    printf("bound=%f expands=%d cost=%d time=%.3f\n", 
        eps_satisfied, delta_expands, goal_state->g, delta_time);

    //update stats
    totalPlanTime += delta_time;
    totalExpands += delta_expands;
    PlannerStats tempStat;
    tempStat.eps = eps_satisfied;
    tempStat.expands = delta_expands;
    tempStat.time = delta_time;
    tempStat.cost = goal_state->g;
    stats.push_back(tempStat);

    //no solution exists
    if(ret == 2){
      printf("Solution does not exist\n");
      return false;
    }

    //if we're just supposed to find the first solution
    //or if we ran out of time, we're done
    if(params.return_first_solution || ret == 0)
      break;

    prepareNextSearchIteration();
  }

  if(goal_state->g == INFINITECOST){
    printf("could not find a solution (ran out of time)\n");
    return false;
  }
  if(eps_satisfied == INFINITECOST)
    printf("WARNING: a solution was found but we don't have quality bound for it!\n");

  printf("solution found\n");
  clock_t before_reconstruct = clock();
  pathIds = GetSearchPath(PathCost);
  reconstructTime = double(clock()-before_reconstruct)/CLOCKS_PER_SEC;
  totalTime = totalPlanTime + reconstructTime;

  return true;
}

void pARAPlanner::prepareNextSearchIteration(){
  //decrease epsilon
  eps -= params.dec_eps;
  if(eps < params.final_eps)
    eps = params.final_eps;

  //dump the inconsisten states into the open list
  CKey key;
  while(!incons.empty()){
    pARAState* s = incons.back();
    incons.pop_back();
    s->in_incons = false;
    key.key[0] = s->g + int(eps * s->h);
    heap.insertheap(s,key);
  }

  //recompute priorities for states in OPEN and reorder it
  for (int i=1; i<=heap.currentsize; ++i){
    pARAState* state = (pARAState*)heap.heap[i].heapstate;
    heap.heap[i].key.key[0] = state->g + int(eps * state->h); 
  }
  heap.makeheap();

  search_iteration++;
}


//-----------------------------Interface function-----------------------------------------------------
int pARAPlanner::replan(vector<int>* solution_stateIDs_V, ReplanParams p){
  int solcost;
  return replan(solution_stateIDs_V, p, &solcost);
}

int pARAPlanner::replan(int start, int goal, vector<int>* solution_stateIDs_V, ReplanParams p, int* solcost){
  set_start(start);
  set_goal(goal);
  return replan(solution_stateIDs_V, p, solcost);
}

int pARAPlanner::replan(vector<int>* solution_stateIDs_V, ReplanParams p, int* solcost){
  printf("planner: replan called\n");
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
  printf("total expands=%d planning time=%.3f reconstruct path time=%.3f total time=%.3f solution cost=%d\n", 
      totalExpands, totalPlanTime, reconstructTime, totalTime, goal_state->g);

  //copy the solution
  *solution_stateIDs_V = pathIds;
  *solcost = PathCost;

  start_state_id = -1;
  goal_state_id = -1;

  return (int)solnFound;
}

int pARAPlanner::set_goal(int id){
  printf("planner: setting goal to %d\n", id);
  if(bforwardsearch)
    goal_state_id = id;
  else
    start_state_id = id;
  return 1;
}

int pARAPlanner::set_start(int id){
  printf("planner: setting start to %d\n", id);
  if(bforwardsearch)
    start_state_id = id;
  else
    goal_state_id = id;
  return 1;
}

//-------------------------------------------------------------------------------------------
//OPEN list


//-------------------------------------------------------------------------------------------


void pARAPlanner::get_search_stats(vector<PlannerStats>* s){
  s->clear();
  s->reserve(stats.size());
  for(unsigned int i=0; i<stats.size(); i++){
    s->push_back(stats[i]);
  }
}

