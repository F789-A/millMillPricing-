#pragma once
#include <vector>

using ivector = std::vector<int>;
using table = std::vector<ivector>;

struct Instance
{
	Instance(const table& costsLeader, const table& costsFollower, const ivector& Budgets);

	table costsLeader;
	table costsFollower;
	ivector budgets;

	ivector qUpperBound;
	ivector pUpperBound;

	int leaderFacilityCount;
	int followerFacilityCount;
	int clientsCount;
};