#include <cstring>
#include <iostream>
#include <string>

using namespace std;

#include <sbpl/headers.h>
#include <sbpl/planners/pwsaplanner.h>

#include <pasl/sched/benchmark.hpp>
#include <pasl/sequtil/cmdline.hpp>

int plan2d(char* envCfgFilename, int eps)
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

  pwsaPlanner* planner = new pwsaPlanner(&environment_nav2D, true);
  ReplanParams params(20.0);
  params.initial_eps = 1.5;
  params.dec_eps = 0.2;
  params.return_first_solution = true;
  params.num_threads = 16;
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
int planxythetalat(char* envCfgFilename, char* motPrimFilename, int eps){
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

  pwsaPlanner* planner = new pwsaPlanner(&environment_navxythetalat, eps);
  ReplanParams params(20.0);
  params.initial_eps = 1.5;
  params.dec_eps = 0.2;
  params.return_first_solution = true;
  params.num_threads = 32;
	printf("start planning...\n");
	bool bRet = planner->replan(MDPCfg.startstateid, MDPCfg.goalstateid,
                         &solution_stateIDs_V, params, &cost);
	printf("done planning\n");
	printf("size of solution=%d\n", (unsigned int)solution_stateIDs_V.size());
 
  
  bRet = planner->replan(MDPCfg.startstateid, MDPCfg.goalstateid,
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

//	delete planner;

	return bRet;
}

int main(int argc, char *argv[]) {
  char* s1 = "../env_examples/nav3d/env1.cfg";
  char* s2 = "../matlab/mprim/pr2.mprim";
//  char* fake_argv[2];
//  fake_argv[0] = "-p"; 
//  fake_argv[1] = "-p"; 
  pasl::util::cmdline::set(argc,argv);
  planxythetalat(s1, s2, 10);
}
