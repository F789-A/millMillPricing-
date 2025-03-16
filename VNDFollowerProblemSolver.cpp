#include "VNDFollowerProblemSolver.h"

#include "Subset.h"


ivector VNDFollowerProblemSolver::GetFromFlip(const ivector& startPrice, std::vector<int>& priceIter,
	SubsetIterator& facilityIter, const Instance& instance)
{
	ivector result = startPrice;

	const std::vector<int>& facilityes = *facilityIter;

	for (int i = 0; i < facilityes.size(); ++i)
	{
		result[facilityes[i]] = priceIter[i];
	}

	for (int i = facilityIter.Cardinality() - 1; i >= 0; --i)
	{
		++priceIter[i];
		if (priceIter[i] > instance.qUpperBound[facilityes[i]])
		{
			priceIter[i] = 0;
			if (i == 0)
			{
				++facilityIter;
			}
		}
		else
		{
			break;
		}
	}

	return result;
}

ivector VNDFollowerProblemSolver::VNDLowerProblem(const ivector& first, const ivector& leaderPrices, int FlipCount, const Instance& instance)
{
	ivector followerPrices = first;
	clientProblemSolver.Solve(leaderPrices, followerPrices, instance);
	int maxIncome = clientProblemSolver.followerIncome;

	int iterationCount = 1;
	int k = 1;
	for (; k <= FlipCount; ++iterationCount)
	{
		ivector followerRecordPrices = followerPrices;
		int incomeRecord = maxIncome;

		SubsetIterator facilityIterator(instance.followerFacilityCount, k);
		std::vector<int> facilityPrices(k, 0);
		while (!facilityIterator.End())
		{
			ivector tmpFollowerPrices = GetFromFlip(followerPrices, facilityPrices, facilityIterator, instance);

			clientProblemSolver.Solve(leaderPrices, tmpFollowerPrices, instance);
			int tmpIncome = clientProblemSolver.followerIncome;
			if (tmpIncome > incomeRecord)
			{
				followerRecordPrices = std::move(tmpFollowerPrices);
				incomeRecord = tmpIncome;
			}
		}

		if (incomeRecord > maxIncome)
		{
			followerPrices = followerRecordPrices;
			maxIncome = incomeRecord;
			k = 1;
		}
		else
		{
			++k;
		}
	}

	return followerPrices;
}

ivector VNDFollowerProblemSolver::SearchLowerProblem(const ivector& leaderPrices, const Instance& instance)
{
	ivector followerPrices(instance.followerFacilityCount, 0);

	int leaderIncome = 0;
	int followerIncome = 0;

	SubsetIterator facilityIterator(instance.followerFacilityCount, instance.followerFacilityCount);
	std::vector<int> facilityPrices(instance.followerFacilityCount, 0);
	while (!facilityIterator.End())
	{
		ivector tmpFollowerPrices = GetFromFlip(followerPrices, facilityPrices, facilityIterator, instance);

		clientProblemSolver.Solve(leaderPrices, tmpFollowerPrices, instance);
		int tmpFollowerIncome = clientProblemSolver.followerIncome;
		int tmpLeaderIncome = clientProblemSolver.followerIncome;
		if (tmpFollowerIncome > followerIncome || (followerIncome == tmpFollowerIncome && tmpLeaderIncome > leaderIncome))
		{
			followerPrices = std::move(tmpFollowerPrices);
			followerIncome = tmpFollowerIncome;
		}
	}
	return followerPrices;
}