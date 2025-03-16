#include "LowerBoundUM.h"
#include <algorithm>
#include "FollowerCooperativeExactSolver.h"
#include <iostream>
#include <chrono>

int LowerBoundUM::Solve(const Instance& instance)
{
	std::chrono::high_resolution_clock timer;
	auto startTime = timer.now();

	int pUB = *std::max_element(instance.pUpperBound.begin(), instance.pUpperBound.end());
	int leaderIncome = 0;
	std::vector<int> prices(instance.leaderFacilityCount);
	FollowerCooperativeExactSolver followerSolver;
	for (int p = 1; p < pUB; ++p)
	{
		auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - startTime).count() / 1000.0f;
		if (deltaTime > 3600)
		{
			return leaderIncome;
		}

		std::fill(prices.begin(), prices.end(), p);
		auto res = followerSolver.Solve(prices, instance, true);
		leaderIncome = std::max(leaderIncome, res.leaderIncome);
	}

	return leaderIncome;
}