#include <cstring>
#include <iostream>
#include <string>

using namespace std;

#include "../sbpl/headers.h"
#include "../planners/PWSA/pwsa.h"

int plan2d(char* envCfgFilename, bool forwardSearch)
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

  pARAPlanner* planner = new pARAPlanner(&environment_nav2D, forwardSearch);
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
int planxythetalat(char* envCfgFilename, char* motPrimFilename, bool forwardSearch){
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

  pARAPlanner* planner = new pARAPlanner(&environment_navxythetalat, forwardSearch);
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
  planxythetalat(argv[1], argv[2], true);
  //plan2d(argv[1], true);
}
