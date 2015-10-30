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
#include <iostream>

#include "pARA.h"
#include <boost/thread/thread.hpp>
#include "pARA_constants.h"
using namespace std;

uint64_t GetTimeStamp() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

//-------------------------------------------------------------------------------------------
//OPEN list

myHeap::myHeap(vector<pARAState*>* expanding, bool* done_flag, DiscreteSpaceInformation* e){
  being_expanded = expanding;
  done = done_flag;
  env = e;
}
void myHeap::setEps(double e){
  eps = e;
}
void myHeap::clear(){
  m.clear();
}
bool myHeap::empty(){
  return m.empty();
}
void myHeap::initIter(pARAState* s){
  s->iter=m.end();
}
void myHeap::insert(pARAState* s, int key){
  if(s->iter!=m.end())
    m.erase(s->iter);
  s->iter = m.insert(pair<int,pARAState*>(key,s));
}
int myHeap::min_value(){
  return m.begin()->first;
}
void myHeap::analyze(){
  printf("queue={\n");
  int cnt = 0;
  for(multimap<int,pARAState*>::iterator it=m.begin(); it!=m.end(); it++){
    if(cnt > 10){
      printf("...\n");
      break;
    }
    printf("       f=%d, g=%d, h=%d: \n",it->first,it->second->g,env->fastHeuristic(m.begin()->second->id,it->second->id));
    //env->PrintState(it->second->id,true);
    cnt++;
  }
  printf("}\n");
  int ties = -1;
  int first_f = m.begin()->first;
  for(multimap<int,pARAState*>::iterator it=m.begin(); it!=m.end(); it++){
    if(it->first == first_f)
      ties++;
  }
  vector<int> validV;
  cnt = 0;
  for(multimap<int,pARAState*>::iterator it=m.begin(); it!=m.end(); it++){
    if(cnt > 10){
      printf("...\n");
      break;
    }
    bool valid = true;
    int cnt2 = 0;
    for(multimap<int,pARAState*>::iterator it2=m.begin(); it2!=it; it2++){
      if(int(it->second->g) - int(it2->second->g) > eps*env->fastHeuristic(it->second->id,it2->second->id)){
        printf("invalid (%d,%d): %d - %d > %f*%d\n",
                cnt,cnt2,
                it->second->g, it2->second->g, eps, 
                env->fastHeuristic(it->second->id,it2->second->id));
        valid = false;
        break;
      }
      cnt2++;
    }
    if(valid)
      validV.push_back(cnt);
    cnt++;
  }
  printf("parallel states={");
  for(unsigned int i=0; i<validV.size(); i++)
    printf("%d,",validV[i]);
  printf("}\n");
  printf("%d parallel states with %d f-val ties\n",int(validV.size()),ties);
  std::cin.get();
}
pARAState* myHeap::remove(boost::unique_lock<boost::mutex>* lock, int* fval, int thread_id){
  //analyze();
  pARAState* local_being_expanded[NUM_THREADS];
  multimap<int,pARAState*>::iterator it;
  while(!(*done)){
    //printf("thread %d: start of remove\n",thread_id);
    bool nothing_expanding = true;
    for(int i=0; i<NUM_THREADS; i++){
      local_being_expanded[i] = being_expanded->at(i);
      if(local_being_expanded[i])
        nothing_expanding = false;
    }
    //if nothing is being expanded 
    if(nothing_expanding){
      if(m.empty() || m.begin()->first >= INFINITECOST){
        //if min_key is >= INFINITE then return NULL
        //if heap is empty then return NULL
        return NULL;
      }
    }
    int cnt = 0;
    for(it=m.begin(); it!=m.end(); it++){
      //if the min key is infinite, start over
      //if(it->first >= INFINITECOST || cnt >= 2*NUM_THREADS)
      if(it->first >= INFINITECOST)
        break;

      //if the being_expanded changed (some state is done being expanded) start over 
      bool changed = false;
      bool valid = true;
      for(int i=0; i<NUM_THREADS; i++){
        if(local_being_expanded[i] != being_expanded->at(i)){
          changed = true;
          break;
        }
        if(local_being_expanded[i] &&
            int(it->second->g) - int(local_being_expanded[i]->g) > eps*env->fastHeuristic(it->second->id,local_being_expanded[i]->id)){
            //printf("thread %d: invalid (with being expanded) (%d): %d - %d > %f*%d\n",thread_id,
                //cnt,
                //it->second->g, local_being_expanded[i]->g, eps, 
                //env->fastHeuristic(it->second->id,local_being_expanded[i]->id));
          valid = false;
          break;
        }
      }
      if(changed){
        //printf("thread %d: being expanded changed!\n",thread_id);
        break;
      }

      if(valid){
        int cnt2 = 0;
        for(multimap<int,pARAState*>::iterator it2=m.begin(); it2!=it; it2++){
          if(int(it->second->g) - int(it2->second->g) > eps*env->fastHeuristic(it->second->id,it2->second->id)){
            valid = false;
            //printf("thread %d: invalid (%d,%d): %d - %d > %f*%d\n",thread_id,
                //cnt,cnt2,
                //it->second->g, it2->second->g, eps, 
                //env->fastHeuristic(it->second->id,it2->second->id));
            break;
          }
          cnt2++;
        }
      }
      else{
        //printf("thread %d: not independent of the current expansion...\n",thread_id);
      }

      if(valid){
        //printf("thread %d: got a state!\n",thread_id);
        *fval = it->first;
        pARAState* s = it->second;
        m.erase(s->iter);
        s->iter = m.end();
        return s;
      }
      else{
        //printf("thread %d: invalid!\n",thread_id);
      }
      cnt++;
    }
    lock->unlock();
    //printf("thread %d: crappy spin\n",thread_id);
    lock->lock();
  }
  return NULL;
}

//-------------------------------------------------------------------------------------------

pARAPlanner::pARAPlanner(DiscreteSpaceInformation* environment, bool bSearchForward) :
  heap(&being_expanded, &iteration_done, environment), params(0.0) {
  bforwardsearch = bSearchForward;
  environment_ = environment;
  replan_number = 0;
  //reconstructTime = 0.0;

  fout = fopen("stats.txt","w");

  goal_state_id = -1;
  start_state_id = -1;

  boost::unique_lock<boost::mutex> lock(mutex);
  //vector<boost::thread*> threads;
  //vector<pARAState*> being_expanded;
  planner_ok = true;
  sleeping_threads = 0;
  thread_ids = 0;
  for(int i=0; i<NUM_THREADS; i++){
    boost::thread* thread = new boost::thread(boost::bind(&pARAPlanner::astarThread, this));
    threads.push_back(thread);
    being_expanded.push_back(NULL);
    being_expanded_fval.push_back(0);
  }
  main_cond.wait(lock);
}

pARAPlanner::~pARAPlanner(){
  fclose(fout);
  boost::unique_lock<boost::mutex> lock(mutex);
  planner_ok = false;
  worker_cond.notify_all();
  lock.unlock();

  for(int i=0; i<NUM_THREADS; i++)
    threads[i]->join();
}

void pARAPlanner::astarThread(){
  boost::unique_lock<boost::mutex> lock(mutex);
  int thread_id = thread_ids;
  thread_ids++;

  while(1){
    //the mutex is locked
    sleeping_threads++;
    if(sleeping_threads==NUM_THREADS)
      main_cond.notify_one();
    worker_cond.wait(lock);
    sleeping_threads--;
    lock.unlock();

    if(!planner_ok)
      break;
    
    int ret = ImprovePath(thread_id);
    if(ret>=0){
      printf("thread %d: setting the improve_path_result to %d\n",thread_id,ret);
      improve_path_result = ret;
    }
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
    s->in_incons = false;
    heap.initIter(s);

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
  //vector<int> better_parent(SuccIDV.size(),0);
  //printf("SuccIDV size=%d\n",SuccIDV.size());
  //environment_->PrintState(parent->id,true);

  boost::unique_lock<boost::mutex> lock(mutex);
  //iterate through successors of the parent
  for(int sind = 0; sind < (int)SuccIDV.size(); sind++){
    pARAState* succ = GetState(SuccIDV[sind]);

    //printf("%d->%d\n",parent->id,succ->id);

    /*
    if(succ->id != SuccIDV[sind]){
      printf("ID MISMATCH 1! (%d != %d)\n",succ->id,SuccIDV[sind]);
      std::cin.get();
    }
    parent->succ_ids.push_back(SuccIDV[sind]);
    parent->succ_ptrs.push_back(succ);
    better_parent[sind] = 0;
    */

    //see if we can improve the value of succ
    //taking into account the cost of action
    //printf("g%d > v%d + cost%d\n",succ->g,parent->v,CostV[sind]);
    if(succ->g > parent->v + CostV[sind]){
      succ->g = parent->v + CostV[sind];
      succ->best_parent = parent;
      //better_parent[sind] = 1;
      //printf("better path to state\n");
      //printf("state iteration=%d, search iteration=%d\n",succ->iteration_closed, search_iteration);

      //we only allow one expansion per search iteration
      //so insert into heap if not closed yet
      if(succ->iteration_closed != search_iteration){
        key.key[0] = succ->g + int(eps * succ->h);
        heap.insert(succ,key.key[0]);
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

  /*
  for(unsigned int i=0; i<SuccIDV.size(); i++){
    pARAState* s = GetState(SuccIDV[i]);
    if(s->id != SuccIDV[i]){
      printf("ID MISMATCH 2! (%d != %d)\n",s->id,SuccIDV[i]);
      std::cin.get();
    }
    if(!checkConnection(GetState(SuccIDV[i]))){
      printf("expanded %d\n",parent->id);
      printf("%d successors, %d cached successors\n",SuccIDV.size(),parent->succ_ids.size());
      for(unsigned int i=0; i<SuccIDV.size(); i++){
        printf("gen %d, cache id %d, cache state id %d, better parent=%d\n",SuccIDV[i],parent->succ_ids[i],parent->succ_ptrs[i]->id,better_parent[i]);
      }
      //environment_->THE_VERB = true;
      //environment_->GetSuccs(parent->id, &SuccIDV, &CostV);
      std::cin.get();
    }
  }
  */
}

void pARAPlanner::checkConnections(){
  for(unsigned int i=0; i<states.size(); i++){
    checkConnection(states[i]);
  }
}

bool pARAPlanner::checkConnection(pARAState* s){
  if(s==start_state)
    return true;
  vector<int> SuccIDV;
  vector<int> CostV;
  pARAState* parent = s->best_parent;
  environment_->GetSuccs(parent->id, &SuccIDV, &CostV);
  for(unsigned int i=0; i<SuccIDV.size(); i++){
    if(SuccIDV[i] == s->id)
      return true;
  }
  printf("ERROR: a parent connection is broken (%d->%d)\n",parent->id,s->id);
  /*
  environment_->p2pCheck(parent->id,s->id);
  printf("cc: input id=%d\n",s->id);
  printf("cc: best parent=%d (%d)\n",parent->id,s->best_parent->id);

  printf("cc: re-expand check of parent %d\n",parent->id);
  printf("cc: %d successors, %d cached successors\n",SuccIDV.size(),parent->succ_ids.size());
  for(unsigned int i=0; i<SuccIDV.size(); i++){
    printf("cc: gen %d, cache id %d, cache state id %d\n",SuccIDV[i],parent->succ_ids[i],parent->succ_ptrs[i]->id);
  }
  */
  
  return false;
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
        heap.insert(pred,key.key[0]);
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
  double rate = 0;
  double rate_cnt = 0;

  //expand states until done
  boost::unique_lock<boost::mutex> lock(mutex);
  if(iteration_done)
    return -1;
  while(!outOfTime()){

    //get the state		
    int fval;
    //printf("thread %d: enter open\n",thread_id);
    pARAState* state = heap.remove(&lock, &fval,thread_id);
    if(state==NULL){
      printf("thread %d: heap remove returned NULL\n",thread_id);
      break;
    }
    //if(state == goal_state)
      //break;

    /*
    int min_fval = fval;
    for(int i=0; i<NUM_THREADS; i++){
      if(being_expanded[i] && being_expanded_fval[i] < min_fval)
        min_fval = being_expanded_fval[i];
    }
    if(state == goal_state ||
       goal_state->g <= min_fval){
    */
    if(goal_state->g <= (unsigned int)fval){
      printf("thread %d: goal g-val is less than min fval\n",thread_id);
      iteration_done = true;
      return 1;
      break;
    }

    if(state->v == state->g)
      printf("ERROR: consistent state is being expanded\n");

    //mark the state as expanded
    state->v = state->g;
    state->iteration_closed = search_iteration;
    search_expands++;
    int cnt = 1;
    for(int i=0; i<NUM_THREADS; i++)
      cnt += being_expanded[i]!=NULL;
    //printf("thread %d: expanding with %d other threads\n",thread_id,cnt);
    //environment_->PrintState(state->id,true);
    if(thread_id==0){
      rate += cnt;
      rate_cnt++;
      if(rate_cnt==100){
        printf("avg parallel threads = %f\n", rate/rate_cnt);
        rate = 0;
        rate_cnt = 0;
      }
    }
    if(cnt==0)
      bad_cnt++;
    else
      bad_cnt=0;
    //if(bad_cnt>3)
      //heap.analyze();
    being_expanded[thread_id] = state;
    //being_expanded_fval[thread_id] = fval;
    lock.unlock();

    if(bforwardsearch)
      UpdateSuccs(state);
    else
      UpdatePreds(state);

    //if(expands%100000 == 0)
      //printf("expands so far=%u\n", expands);

    //printf("thread %d: finished expansion\n",thread_id);
    being_expanded[thread_id] = NULL;
    lock.lock();

    if(iteration_done){
      printf("thread %d: iteration done\n",thread_id);
      return -1;
    }
  }

  //search_expands += expands;
   
  //scoped lock unlocks upon return (goes out of scope)
  iteration_done = true;
  if(goal_state->g == INFINITECOST && (heap.empty() || heap.min_value() >= INFINITECOST))
    return 0;//solution does not exists
  if(!heap.empty() && int(goal_state->g) > heap.min_value())
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
    if(actioncost == INFINITECOST){
      printf("WARNING: actioncost = %d\n", actioncost);
      std::cin.get();
    }
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
  double time_used = double(GetTimeStamp() - TimeStarted)/1000000.0;
  //we are out of time if:
         //we used up the max time limit OR
         //we found some solution and used up the minimum time limit
  if(params.return_first_solution)
    return false;
  if(time_used >= params.max_time)
    printf("out of max time\n");
  if(use_repair_time && eps_satisfied != INFINITECOST && time_used >= params.repair_time)
    printf("used all repair time...\n");
  return time_used >= params.max_time || 
         (use_repair_time && eps_satisfied != INFINITECOST && time_used >= params.repair_time);
}

void pARAPlanner::initializeSearch(){
  //it's a new search, so increment replan_number and reset the search_iteration
  replan_number++;
  search_iteration = 0;
  search_expands = 0;
  totalExpands = 0;
  totalPlanTime = 0;
  reconstructTime = 0.0;


  //clear open list, incons list, and stats list
  heap.clear();
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
  heap.insert(start_state, key.key[0]);

  //ensure heuristics are up-to-date
  environment_->EnsureHeuristicsUpdated((bforwardsearch==true));
}

bool pARAPlanner::Search(vector<int>& pathIds, int& PathCost){
  CKey key;
  TimeStarted = GetTimeStamp();

  initializeSearch();

  //the main loop of ARA*
  while(eps_satisfied > params.final_eps && !outOfTime()){

    //run weighted A*
    uint64_t before_time = GetTimeStamp();
    int before_expands = search_expands;

    iteration_done = false;
    bad_cnt = 0;
    heap.setEps(eps);
    boost::unique_lock<boost::mutex> lock(mutex);
    worker_cond.notify_all();
    main_cond.wait(lock);

    //checkConnections();

    int ret = improve_path_result;
    if(ret == 1) //solution found for this iteration
      eps_satisfied = eps;
    int delta_expands = search_expands - before_expands;
    double delta_time = double(GetTimeStamp() - before_time)/1000000.0;

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
  uint64_t before_reconstruct = GetTimeStamp();
  pathIds = GetSearchPath(PathCost);
  reconstructTime = 0.0;
  reconstructTime = double(GetTimeStamp()-before_reconstruct)/1000000.0;
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
    heap.insert(s,key.key[0]);
  }

  //recompute priorities for states in OPEN and reorder it
  /*
  for (int i=1; i<=heap.currentsize; ++i){
    pARAState* state = (pARAState*)heap.heap[i].heapstate;
    heap.heap[i].key.key[0] = state->g + int(eps * state->h); 
  }
  heap.makeheap();
  */

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
  params.initial_eps = INITIAL_EPS;
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

  fprintf(fout, "%d %f %f %d %d\n", NUM_THREADS, params.initial_eps, totalPlanTime, totalExpands, goal_state->g);

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



void pARAPlanner::get_search_stats(vector<PlannerStats>* s){
  s->clear();
  s->reserve(stats.size());
  for(unsigned int i=0; i<stats.size(); i++){
    s->push_back(stats[i]);
  }
}

