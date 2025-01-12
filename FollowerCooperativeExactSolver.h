#pragma once
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include "Instance.h"

class FollowerCooperativeExactSolver
{
public:
	FollowerCooperativeExactSolver(const ivector& leaderPrices, const Instance& instance);

private:
	SCIP_RETCODE SolveProblem(const ivector& leaderPrices, const Instance& instance);

	const bool debug = false;

public:
	int leaderIncome;
	int income;
	ivector prices;
};

class FollowerCooperativeExactSolverStable
{
public:
	FollowerCooperativeExactSolverStable(const ivector& leaderPrices, const ivector& prevFollowerPrices, const Instance& instance);

public:
	SCIP_RETCODE SolveProblem(const ivector& leaderPrices, const ivector& prevFollowerPrices, const Instance& instance);

	const bool debug = false;

public:
	int income;
	ivector prices;
};