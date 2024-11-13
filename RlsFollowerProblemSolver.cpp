#include "RlsFollowerProblemSolver.h"

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
	std::uniform_int_distribution<> distrib1(0, static_cast<int>(startPrice.size()) - 1);

	int facility = distrib1(random_generator);

	int pBoundMin = 0;
	int pBoundMax = instance.qUpperBound[facility];

	std::uniform_int_distribution<> distrib2(pBoundMin, pBoundMax);
	ivector result = startPrice;
	result[facility] = distrib2(random_generator);
	return result;
}

ivector RlsFollowerProblemSolver::GetFromFlip(const ivector& startPrice, int& price, int& facility, const Instance& instance)
{
	ivector result = startPrice;
	result[facility] = price;
	++price;
	if (price > instance.qUpperBound[facility])
	{
		++facility;
		price = 0;
	}
	return result;
}

ivector RlsFollowerProblemSolver::RlsLowerProblem(const ivector& first, const ivector& leaderPrices, const Instance& instance, int& iterrCount)
{
	//const int maxIterCount = 1000 * instance.followerFacilityCount;
	const int maxIterCount = std::accumulate(instance.qUpperBound.begin(), instance.qUpperBound.end(), 0);

	//ivector followerPrices = GetFirst(instance, false);]
	ivector followerPrices = first;

	clientProblemSolver.Solve(leaderPrices, followerPrices, instance);
	int maxIncome = clientProblemSolver.followerIncome;

	while (true)
	{
		++iterrCount;
		ivector followerRecordPrices = followerPrices;
		int incomeRecord = maxIncome;

		int price = 0;
		int facility = 0;
		for (int i = 0; i < maxIterCount; ++i)
		{
			ivector tmpFollowerPrices = GetFromFlip(followerPrices, price, facility, instance);

			clientProblemSolver.Solve(leaderPrices, tmpFollowerPrices, instance);
			int tmpIncome = clientProblemSolver.followerIncome;
			if (tmpIncome > incomeRecord)
			{
				followerRecordPrices = tmpFollowerPrices;
				incomeRecord = tmpIncome;
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

	return followerPrices;
}
