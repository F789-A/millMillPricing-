#pragma once
#include <vector>
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include "Instance.h"

class FollowerProblemSolver
{
public:
	FollowerProblemSolver(const ivector& leaderPrices, const Instance& instance);

private:
	SCIP_RETCODE SolveProblem(const ivector& leaderPrices, const Instance& instance);

public:
	float income;
	ivector prices;
};