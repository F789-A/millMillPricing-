#pragma once
#include "Instance.h"

class IVndUpper
{
	IVndUpper(const Instance& instance);

	void RunVnd();

	void Solve(const ivector& leaderPrice, const ivector& followePrice, const Instance& instance);

};

class VndUpperVndLower
{

};

class VndUpperExactLower
{

};