#include "RlsFollowerProblemSolver.h"

int RlsFollowerProblemSolver::SolveLower(const ivector& leaderPrices, const ivector& followerPrices, const Instance& instance)
{
	int income = 0;
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		int tmpIncome = 0;
		int minLeaderCost = std::numeric_limits<int>::max();
		for (int i = 0; i < instance.leaderFacilityCount; ++i)
		{
			int tmpMinLeaderCost = instance.costsLeader[i][j] + leaderPrices[i];
			if (instance.budgets[j] - tmpMinLeaderCost >= 0 && minLeaderCost >= tmpMinLeaderCost)
			{
				minLeaderCost = tmpMinLeaderCost;
			}
		}
		int minFollowerCost = std::numeric_limits<int>::max();
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			int tmpMinFollowerCost = instance.costsFollower[i][j] + followerPrices[i];
			if (instance.budgets[j] - tmpMinFollowerCost >= 0 && minFollowerCost >= tmpMinFollowerCost)
			{
				minFollowerCost = tmpMinFollowerCost;
				tmpIncome = followerPrices[i];
			}
		}
		if (minFollowerCost < minLeaderCost)
		{
			income += tmpIncome;
		}
	}
	return income;
}

ivector RlsFollowerProblemSolver::GetFirst(const Instance& instance, bool upper)
{
	ivector result = upper ? instance.pUpperBound : instance.qUpperBound;
	for (auto& l : result)
	{
		l /= 2;
	}
	return result;
}

ivector RlsFollowerProblemSolver::GetRandomFromFlip(const ivector& startPrice, const Instance& instance)
{
	static std::seed_seq seed_w({ 123123 });
	static auto random_generator = std::mt19937(seed_w);
	std::uniform_int_distribution<> distrib1(0, startPrice.size() - 1);

	int facility = distrib1(random_generator);

	int pBoundMin = 0;
	int pBoundMax = instance.qUpperBound[facility];

	std::uniform_int_distribution<> distrib2(pBoundMin, pBoundMax);
	ivector result = startPrice;
	result[facility] = distrib2(random_generator);
	return result;
}

ivector RlsFollowerProblemSolver::RlsLowerProblem(const ivector& first, const ivector& leaderPrices, const Instance& instance, int& iterrCount, int& income)
{
	const int maxIterCount = 1000 * instance.followerFacilityCount;
	//ivector followerPrices = GetFirst(instance, false);]
	ivector followerPrices = first;
	int maxIncome = SolveLower(leaderPrices, followerPrices, instance);

	int iterCount = 0;

	while (true)
	{
		++iterCount;
		ivector followerRecordPrices = followerPrices;
		int incomeRecord = maxIncome;

		for (int i = 0; i < maxIterCount; ++i)
		{
			ivector tmpFollowerPrices = GetRandomFromFlip(leaderPrices, instance);
			int income = SolveLower(leaderPrices, tmpFollowerPrices, instance);
			if (income > incomeRecord)
			{
				followerRecordPrices = tmpFollowerPrices;
				incomeRecord = income;
			}
		}

		if (incomeRecord > maxIncome)
		{
			followerPrices = followerRecordPrices;
			maxIncome = incomeRecord;
		}
		else
		{
			break;
		}
	}

	income = maxIncome;
	iterrCount += iterCount;
	return followerPrices;
}
