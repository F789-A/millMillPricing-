#pragma once
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include "Instance.h"

class HighPointRelaxation
{
public:
	SCIP_RETCODE Solve(const Instance& instance);

	const bool debug = false;

public:
	float income;
	ivector prices;
};