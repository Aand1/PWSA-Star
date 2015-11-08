#include <cstring>
#include <iostream>
#include <string>

using namespace std;

#include "../sbpl/headers.h"
#include "../planners/PWSA/pwsa.h"

#include "../../pasl/sched/benchmark.hpp"
#include "../../pasl/sequtil/cmdline.hpp"

//namespace par = pasl::sched::native;
//
//static long seq_fib (long n){
//  if (n < 2)
//    return n;
//  else
//    return seq_fib (n - 1) + seq_fib (n - 2);
//}
//
//static long par_fib(long n) {
//  if (n <= 0 || n < 2)
//    return seq_fib(n);
//  long a, b;
//  par::fork2([n, &a] { a = par_fib(n-1); },
//             [n, &b] { b = par_fib(n-2); });
//  return a + b;
//}



int plan2d(char* envCfgFilename)
{
  MDPConfig MDPCfg;
	// Initialize Environment (should be called before initializing anything else)
	EnvironmentNAV2D environment_nav2D;
	if (!environment_nav2D.InitializeEnv(envCfgFilename)) {
		printf("ERROR: InitializeEnv failed\n");
		throw new SBPL_Exception();
	}

	// Initialize MDP Info
	if (!environment_nav2D.InitializeMDPCfg(&MDPCfg)) {
		printf("ERROR: InitializeMDPCfg failed\n");
		throw new SBPL_Exception();
	}

	// plan a path
	vector<int> solution_stateIDs_V;
  int cost;

  pwsaPlanner* planner = new pwsaPlanner(&environment_nav2D);
  ReplanParams params(20.0);
  params.initial_eps = 10.0;
  params.dec_eps = 0.2;
  params.return_first_solution = true;
	printf("start planning...\n");
	bool bRet = planner->replan(MDPCfg.startstateid, MDPCfg.goalstateid,
                         &solution_stateIDs_V, params, &cost);
	printf("done planning\n");
	printf("size of solution=%d\n", (unsigned int)solution_stateIDs_V.size());

	//print a path
	if (bRet) {
		//print the solution
		printf("Solution is found\n");
	}
	else {
		printf("Solution does not exist\n");
	}

	delete planner;

	return bRet;
}

/*******************************************************************************
 *******************************************************************************/
int planxythetalat(char* envCfgFilename, char* motPrimFilename){
  MDPConfig MDPCfg;
  vector<sbpl_2Dpt_t> perimeterptsV;

	// Initialize Environment (should be called before initializing anything else)
	EnvironmentNAVXYTHETALAT environment_navxythetalat;

	if (!environment_navxythetalat.InitializeEnv(envCfgFilename, perimeterptsV, motPrimFilename)) {
		printf("ERROR: InitializeEnv failed\n");
		throw new SBPL_Exception();
	}

	// Initialize MDP Info
	if (!environment_navxythetalat.InitializeMDPCfg(&MDPCfg)) {
		printf("ERROR: InitializeMDPCfg failed\n");
		throw new SBPL_Exception();
	}

	// plan a path
	vector<int> solution_stateIDs_V;
  int cost;

  pwsaPlanner* planner = new pwsaPlanner(&environment_navxythetalat);
  ReplanParams params(20.0);
  params.initial_eps = 10.0;
  params.dec_eps = 0.2;
  params.return_first_solution = true;
	printf("start planning...\n");
	bool bRet = planner->replan(MDPCfg.startstateid, MDPCfg.goalstateid,
                         &solution_stateIDs_V, params, &cost);
	printf("done planning\n");
	printf("size of solution=%d\n", (unsigned int)solution_stateIDs_V.size());

	// print a path
	if (bRet) {
		// print the solution
		printf("Solution is found\n");
	}
	else {
		printf("Solution does not exist\n");
	}

	delete planner;

	return bRet;
}

int main(int argc, char *argv[]){
  char* s1 = "../../env_examples/nav3d/env1.cfg";
  char* s2 = "../../matlab/mprim/pr2.mprim";
//  char* fake_argv[2];
//  fake_argv[0] = "-p"; 
//  fake_argv[1] = "-p"; 
  pasl::util::cmdline::set(argc,argv);
  planxythetalat(s1, s2);
}
