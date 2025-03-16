#pragma once
#include "Instance.h"

class UpperBoundDM
{
public:
	int Solve(const Instance& instance);

private:
	//SCIP_RETCODE SolveFollower(const ivector& leaderPricesByClient, const Instance& instance, FollowerCooperativeExactOutput& output);
	//SCIP_RETCODE SolveFollowerCooperative(const ivector& leaderPricesByClient, const Instance& instance, FollowerCooperativeExactOutput& output);
	//ivector GetNextPrice(const ivector& startPrice, std::vector<int>& priceIter, SubsetIterator& facilityIter, const Instance& instance);
};