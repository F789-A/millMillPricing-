#pragma once
#include <vector>
#include <scip/scip.h>
#include <scip/scipdefplugins.h>

using table = std::vector<std::vector<float>>;
using vector = std::vector<float>;
using ivector = std::vector<int>;

struct Instance
{
	Instance(const table& costs, const vector& Budgets);

	table costs;
	vector budgets;

	vector pUpperBound;

	int facilityCount;
	int clientsCount;
};

struct Answer
{
	vector prices;
	float income;
};


class ProblemSolver
{
public:
	ProblemSolver(const ivector& constraint, const Instance& instance, bool relax);

	SCIP_RETCODE SolveProblem(const ivector& constraint, const Instance& instance, bool relax);
	Answer answer;
};

class FirstVectorSolver
{
public:
	FirstVectorSolver(const ivector& constraint, const Instance& instance);

	SCIP_RETCODE SolveProblem(const ivector& constraint, const Instance& instance);

	bool exist = true;
	vector prices;
};