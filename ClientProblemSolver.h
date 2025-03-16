#pragma once
#include "Instance.h"

class ClientProblemSolver
{
public:
	void Solve(const ivector& leaderPrice, const ivector& followerPrice, const Instance& instance);

	int leaderIncome;
	int followerIncome;

	bool needAll = false;
	std::vector<int> xOut;
	std::vector<int> xOutB;

	bool debug = false;
};