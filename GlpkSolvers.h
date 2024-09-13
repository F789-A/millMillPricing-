#pragma once
#include "glpk.h"

#include <vector>

using table = std::vector<std::vector<float>>;
using vector = std::vector<float>;
using ivector = std::vector<int>;

struct Instance
{
	Instance() = default; // TODO remove
	Instance(const table& CostsLeader, const table& CostsFollower, const vector& Budgets);

	table costsLeader;
	table costsFollower;
	vector budgets;

	vector pUpperBound;
	vector qUpperBound;

	int leaderFacilityCount;
	int followerFacilityCount;
	int clientsCount;
};

struct UpperAnswer
{
	vector leaderPrices;
	vector followerPrices;
	float leaderIncome;
};

struct LowerAnswer
{
	vector followerPrices;
	float followerIncome;
};


class RelaxSolver
{
public:
	RelaxSolver(const ivector& constraint, const Instance& instance); // TODO add leader prices

	LowerAnswer answer;
};

class FirstVectorSolver
{
public:
	FirstVectorSolver(const ivector& constraint, const Instance& instance);

	bool exist = true;
	LowerAnswer answer;
};