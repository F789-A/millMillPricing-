#pragma once
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include "Instance.h"

class FollowerExactSolver
{
public:
	SCIP_RETCODE Solve(const ivector& leaderPrices, const Instance& instance);

	const bool debug = false;

public:
	int income;
	ivector prices;
};