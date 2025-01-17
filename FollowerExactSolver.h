#pragma once
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include "Instance.h"

struct FollowerSolution
{
	int followerIncome;
	ivector followerPrices;
};

class FollowerExactSolver
{
public:
	FollowerSolution Solve(const ivector& leaderPrices, const Instance& instance);

	bool debug = false;

private:
	SCIP_RETCODE Solve(const ivector& leaderPrices, const Instance& instance, FollowerSolution& output);
};