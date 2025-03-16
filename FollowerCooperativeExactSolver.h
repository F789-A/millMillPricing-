#pragma once
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include "Instance.h"

#include "FollowerExactSolver.h"
#include "ClientProblemSolver.h"

struct FollowerCooperativeExactOutput
{
	int leaderIncome;
	int followerIncome;
	ivector followerPrices;
};

class FollowerCooperativeExactSolver
{
public:
	FollowerCooperativeExactOutput Solve(const ivector& leaderPrices, const Instance& instance, bool forceLeaderUpperBound = false);

private:
	SCIP_RETCODE Solve(const ivector& leaderPrices, const Instance& instance, FollowerCooperativeExactOutput& output, bool forceLeaderUpperBound = false);

	const bool debug = false;

	FollowerExactSolver followerExactSolver;
};

/*class FollowerCooperativeExactSolverStable
{
public:
	FollowerCooperativeExactSolverStable(const ivector& leaderPrices, const ivector& prevFollowerPrices, const Instance& instance);

public:
	SCIP_RETCODE SolveProblem(const ivector& leaderPrices, const ivector& prevFollowerPrices, const Instance& instance);

	const bool debug = false;

public:
	int income;
	ivector prices;
};*/