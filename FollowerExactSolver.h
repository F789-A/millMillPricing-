#pragma once
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include "Instance.h"

class FollowerExactSolver
{
public:
	FollowerExactSolver(const ivector& leaderPrices, const Instance& instance);

private:
	SCIP_RETCODE SolveProblem(const ivector& leaderPrices, const Instance& instance);

public:
	float income;
	ivector prices;
};