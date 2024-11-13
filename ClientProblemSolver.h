#pragma once
#include "Instance.h"

class ClientProblemSolver
{
public:
	void Solve(const ivector& leaderPrice, const ivector& followerPrice, const Instance& instance);

	int leaderIncome;
	int followerIncome;
	std::vector<char> clientChose;
	std::vector<int> _clientChose; // TODO remove
};