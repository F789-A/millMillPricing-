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

	const bool debug = true;

public:
	float income;
	ivector prices;
};