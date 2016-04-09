#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
//include <sys/time.h>
#include <math.h>

#include "timing.hpp"
//include "constants.h"
//include "ScenarioLoader.h"

using namespace std;

//define EXP_TIME (SEC_PER_EXPAND*1000000.0)

#define INFINITE 1000000000
int dx[8] = {1,1,-1,-1,1,0,-1,0};
int dy[8] = {1,-1,1,-1,0,1,0,-1};
int dcost[8] = {14142,14142,14142,14142,10000,10000,10000,10000};

int num_threads;
double wA_eps;
double pA_eps;
double SEC_PER_EXPAND;

int start_x;
int start_y;
int goal_x;
int goal_y;
int size_x;
int size_y;
//char map_name[64];

class state{
  public:
    int obs;
    int x;
    int y;
    double g;
    multimap<int,state*>::iterator iter;
};

state** being_expanded;
int* being_expanded_fval;
int path_length;
int num_discovered;
int num_expands;
bool search_done;
pthread_cond_t cond;
pthread_mutex_t mutex;
int ids;


class myQueue{
  public:
    void clear(){m.clear();}
    bool empty(){return m.empty();}
    bool contains(state s){return s.iter!=m.end();}
    void initIter(state* s){s->iter=m.end();}
    inline double goalH(state* s){
      double dx = s->x - goal_x;
      double dy = s->y - goal_y;
      return sqrt(dx*dx+dy*dy)*10000;
    }
    void insert(state* s){
      if(s->iter!=m.end())
        m.erase(s->iter);
      int f = s->g + wA_eps*goalH(s);
      s->iter = m.insert(pair<int,state*>(f,s));
      /*
      printf("  queue={");
      for(multimap<int,state*>::iterator it=m.begin(); it!=m.end(); it++)
        printf("(%d,%d|%d),",it->second->x,it->second->y,it->first);
      printf("\n");
      */
    }
    state* remove(double* fval){
      multimap<int,state*>::iterator it;
      while(!search_done){
        it=m.begin();
        if(it!=m.end()){
          *fval = it->first;
          state* s = it->second;
          m.erase(s->iter);
          s->iter = m.end();
          return s;
        }
        pthread_mutex_unlock(&mutex);
        //printf("crappy spin\n");
        pthread_mutex_lock(&mutex);
      }
      return NULL;
    }

  private:
    multimap<int,state*> m;
};


vector<vector<state> > grid;
myQueue q;


// uint64_t GetTimeStamp() {
//   struct timeval tv;
//   gettimeofday(&tv,NULL);
//   return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
// }

void astar_thread( void *ptr ){
  pthread_mutex_lock(&mutex);
  int thread_id = ids;
  ids++;
  while(!search_done){
    //printf("thread %d get state\n",thread_id);

    //get a state to expand
    double min_fval;
    state* s = q.remove(&min_fval);
    if(s==NULL){
      //printf("thread %d no states left\n",thread_id);
      //search_done = true;
      break;
    }
    being_expanded[thread_id] = s;
    being_expanded_fval[thread_id] = min_fval;
    //printf("thread %d is expanding (%d %d)\n",thread_id,s->x,s->y);
    num_expands++;

    for(int i=0; i<num_threads; i++){
      if(being_expanded[i] && being_expanded_fval[i] < min_fval)
        min_fval = being_expanded_fval[i];
    }
    state* goal = &(grid[goal_x][goal_y]);
    if(goal->g <= min_fval){
      //printf("thread %d goal found\n",thread_id);
      search_done = true;
      break;
    }

    pthread_mutex_unlock(&mutex);


    for(int i=0; i<8; i++){
      int newX = s->x+dx[i];
      int newY = s->y+dy[i];
      state* t = &(grid[newX][newY]);
      //artificial collision checking time

      //usleep(1);
      timing::busy_loop_secs(SEC_PER_EXPAND);
      // uint64_t t0 = GetTimeStamp();
      // uint64_t t1;
      // uint64_t dt;
      // do{
      //   t1 = GetTimeStamp();
      //   dt = t1-t0;
      // } while(dt<EXP_TIME);

      if(newX<0 || newX==size_x || newY<0 || newY==size_y || t->obs)
        continue;

      //critical section for updating g-value and inserting into the heap
      //printf("thread %d add successor\n",thread_id);
      pthread_mutex_lock(&mutex);
      if(t->g==INFINITE)
        num_discovered++;
      if(t->g>s->g+dcost[i]){
        t->g = s->g+dcost[i];
        q.insert(t);
      }
      pthread_mutex_unlock(&mutex);
      //printf("thread %d done adding successor\n",thread_id);
    }

    //lock for the start of the next iteration
    being_expanded[thread_id] = NULL;
    //printf("thread %d post expand get lock\n",thread_id);
    pthread_mutex_lock(&mutex);
    //printf("thread %d post expand locked\n",thread_id);
  }
  //printf("thread %d exit\n",thread_id);
  ids--;
  if(ids==0){
    //printf("thread %d: wake up main thread\n",thread_id);
    pthread_cond_signal(&cond);
  }
  pthread_mutex_unlock(&mutex);
  pthread_exit(0);
}

void solveMaze(int& path_length, int& num_discovered, int& num_expands){
  num_expands = 0;
  num_discovered = 1;
  grid[start_x][start_y].g = 0;
  q.clear();
  q.insert(&grid[start_x][start_y]);

  search_done = false;
  ids = 0;
  pthread_mutex_lock(&mutex);
  vector<pthread_t> threads;
  being_expanded = new state*[num_threads];
  being_expanded_fval = new int[num_threads];
  for(int i=0; i<num_threads; i++){
    being_expanded[i] = NULL;
    pthread_t thread;
    pthread_create (&thread, NULL, (void* (*)(void*)) &astar_thread, (void*) NULL);
    threads.push_back(thread);
  }
  //printf("main thread go to sleep\n");
  pthread_cond_wait(&cond, &mutex);
  //printf("main thread awake\n");
  for(int i=0; i<num_threads; i++)
    pthread_join(threads[i], NULL);
  delete [] being_expanded;
  delete [] being_expanded_fval;
  pthread_mutex_unlock(&mutex);

  if(grid[goal_x][goal_y].g==INFINITE || q.contains(grid[goal_x][goal_y])){
    printf("Queue is empty....failed to find goal!\n");
    std::exit(EXIT_FAILURE);
  }
  path_length = grid[goal_x][goal_y].g;
}

// void readScenario(char* scen_name,int idx){
//   ScenarioLoader sl(scen_name);
//   Experiment e = sl.GetNthExperiment(sl.GetNumExperiments()-idx-1);
//   if(e.GetDistance() <= 10.0)
//     e = sl.GetNthExperiment(idx);
//
//   start_y = e.GetStartX();
//   start_x = e.GetStartY();
//   goal_y = e.GetGoalX();
//   goal_x = e.GetGoalY();
//
//   const char* map_filename = e.GetMapName();
//   //printf("load %s\n",map_filename);
//   FILE* fin = fopen(map_filename,"r");
//   fscanf(fin,"type octile\nheight %d\n",&size_x);
//   fscanf(fin,"width %d\n map\n",&size_y);
//   grid.clear();
//   grid.resize(size_x);
//   for(int i=0; i<grid.size(); i++)
//     grid[i].resize(size_y);
//   for(int i=0; i<grid.size(); i++){
//     for(int j=0; j<grid[i].size(); j++){
//       char c;
//       fscanf(fin,"%c",&c);
//       if(c=='\n'){
//         j--;
//         continue;
//       }
//       else if(c == '.' || c=='G')
//         grid[i][j].obs = 0;
//       else if(c == '@' || c=='O' || c=='T' || c=='S' || c=='W')
//         grid[i][j].obs = 1;
//       else{
//         printf("uh oh! funny character %c\n",c);
//       }
//       grid[i][j].closed = false;
//       grid[i][j].g = INFINITE;
//       grid[i][j].x = i;
//       grid[i][j].y = j;
//       q.initIter(&(grid[i][j]));
//     }
//   }
//   /*
//   printf("size_x=%d size_y=%d\n",size_x,size_y);
//   printf("start_x=%d start_y=%d\n",start_x,start_y);
//   printf("%d %d %d\n%d %d %d\n%d %d %d\n",
//          grid[start_x-1][start_y-1].obs, grid[start_x][start_y-1].obs, grid[start_x+1][start_y-1].obs,
//          grid[start_x-1][start_y].obs, grid[start_x][start_y].obs, grid[start_x+1][start_y].obs,
//          grid[start_x-1][start_y+1].obs, grid[start_x][start_y+1].obs, grid[start_x+1][start_y+1].obs);
//   printf("goal_x=%d goal_y=%d\n",goal_x,goal_y);
//   printf("%d %d %d\n%d %d %d\n%d %d %d\n",
//          grid[goal_x-1][goal_y-1].obs, grid[goal_x][goal_y-1].obs, grid[goal_x+1][goal_y-1].obs,
//          grid[goal_x-1][goal_y].obs, grid[goal_x][goal_y].obs, grid[goal_x+1][goal_y].obs,
//          grid[goal_x-1][goal_y+1].obs, grid[goal_x][goal_y+1].obs, grid[goal_x+1][goal_y+1].obs);
//   */
// }

char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

int parse_or_default_int(int argc, char** argv, const std::string& name, int d) {
  if (cmdOptionExists(argv, argv+argc, name)) {
    return atoi(getCmdOption(argv, argv+argc, name));
  }
  else return d;
}

double parse_or_default_double(int argc, char** argv, const std::string& name, double d) {
  if (cmdOptionExists(argv, argv+argc, name)) {
    return atof(getCmdOption(argv, argv+argc, name));
  }
  else return d;
}

char* parse_or_default_string(int argc, char** argv, const std::string& name, char* d) {
  if (cmdOptionExists(argv, argv+argc, name)) {
    return getCmdOption(argv, argv+argc, name);
  }
  else return d;
}

int main(int argc, char** argv){
  //FILE* fout = fopen("stats.csv","w");
  pthread_mutex_init(&mutex,NULL);
  pthread_cond_init(&cond,NULL);

  // ************************************************************************
  start_y = parse_or_default_int(argc, argv, "-sc", 1); //e.GetStartX();
  start_x = parse_or_default_int(argc, argv, "-sr", 1); //e.GetStartY();
  goal_y = parse_or_default_int(argc, argv, "-dc", 1); //e.GetGoalX();
  goal_x = parse_or_default_int(argc, argv, "-dr", 1); //e.GetGoalY();

  num_threads = parse_or_default_int(argc, argv, "-proc", 1);
  wA_eps = parse_or_default_double(argc, argv, "-w", 1.0);
  pA_eps = parse_or_default_double(argc, argv, "-eps", wA_eps);
  SEC_PER_EXPAND = parse_or_default_double(argc, argv, "-exptime", 0.0);
  double opt = parse_or_default_double(argc, argv, "-opt", 1.0);

  const char* map_filename = parse_or_default_string(argc, argv, "-map", "maps/simple_map.map");
  //printf("load %s\n",map_filename);
  FILE* fin = fopen(map_filename,"r");
  fscanf(fin,"type octile\nheight %d\n",&size_x);
  fscanf(fin,"width %d\n map\n",&size_y);
  grid.clear();
  grid.resize(size_x);
  for(int i=0; i<grid.size(); i++)
    grid[i].resize(size_y);
  for(int i=0; i<grid.size(); i++){
    for(int j=0; j<grid[i].size(); j++){
      char c;
      fscanf(fin,"%c",&c);
      if(c=='\n'){
        j--;
        continue;
      }
      else if(c == '.' || c=='G')
        grid[i][j].obs = 0;
      else if(c == '@' || c=='O' || c=='T' || c=='S' || c=='W')
        grid[i][j].obs = 1;
      else{
        printf("uh oh! funny character %c\n",c);
      }
      //grid[i][j].closed = false;
      grid[i][j].g = INFINITE;
      grid[i][j].x = i;
      grid[i][j].y = j;
      q.initIter(&(grid[i][j]));
    }
  }
  // ************************************************************************

  uint64_t t0 = timing::now();
  solveMaze(path_length, num_discovered, num_expands);
  uint64_t t1 = timing::now();

  double dt = double(t1-t0)/1000000.0;
  printf("exectime %f\n", dt);
  printf("expanded %d\n", num_expands);
  double pathlen = double(path_length)/10000.0;
  printf("pathlen %f\n", pathlen);
  printf("deviation %f\n", pathlen / opt);

  //loop over maps
  // for(int i=1; i<argc; i++){
  //   for(int j=0; j<1; j++){
  //     //load map
  //     readScenario(argv[i],j);
  //
  //     //run planner
  //     uint64_t t0 = GetTimeStamp();
  //     solveMaze(path_length, num_discovered, num_expands);
  //     uint64_t t1 = GetTimeStamp();
  //
  //     //report stats
  //     double dt = double(t1-t0)/1000000.0;
  //     printf("exectime %f\n", dt);
  //     printf("pathlen %d\n", path_length);
  //     //printf("map %d, goal %d: Path Length=%d Visited Nodes=%d Explored Nodes=%d Planning Time=%f\n",i-1,j,/*double(path_length)/10000.0*/path_length,num_discovered,num_expands,dt);
  //     //fprintf(fout,"%d %f %f %f %f %d %f\n",num_threads,wA_eps,pA_eps,SEC_PER_EXPAND,dt,num_expands,double(path_length)/10000.0);
  //   }
  // }
  // fclose(fout);
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);

  return 0;
}
