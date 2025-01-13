#include "VNDFollowerProblemSolver.h"

#include "Subset.h"

ivector VNDFollowerProblemSolver::GetFirst(const Instance& instance, bool upper)
{
	ivector result = upper ? instance.pUpperBound : instance.qUpperBound;
	for (auto& l : result)
	{
		l /= 2;
	}
	return result;
}

ivector VNDFollowerProblemSolver::GetRandomFromFlip(const ivector& startPrice, const Instance& instance)
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

ivector VNDFollowerProblemSolver::RlsLowerProblem(const ivector& first, const ivector& leaderPrices, int FlipCount, const Instance& instance)
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
