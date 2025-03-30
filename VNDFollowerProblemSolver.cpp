#include "VNDFollowerProblemSolver.h"

#include "Subset.h"

FlipIterator::FlipIterator(const ivector& startPrice, const int k, const ivector& upperBound)
	: subsetIterator(startPrice.size(), k), startPrice(startPrice), current(startPrice), upperBounds(upperBound)
{
	const std::vector<int>& facilities = *subsetIterator;

	for (auto k : facilities)
	{
		current[k] = 0;
	}

}

FlipIterator& FlipIterator::operator++()
{
	const std::vector<int>& facilities = *subsetIterator;
	
	for (int i = 0; i < facilities.size(); ++i)
	{
		int j = facilities[i];

		++current[j];
		if (current[j] > upperBounds[j])
		{
			current[j] = 0;
			if (i == facilities.size() - 1)
			{
				for (auto k : facilities)
				{
					current[k] = startPrice[k];
				}
				++subsetIterator;
				if (!subsetIterator.End())
				{
					for (auto k : facilities)
					{
						current[k] = 0;
					}
				}
			}
		}
		else
		{
			break;
		}
	}

	return *this;
}

const std::vector<int>& FlipIterator::operator*() const
{
	return current;
}

bool FlipIterator::End() const
{
	return subsetIterator.End();
}

ivector VNDFollowerProblemSolver::VNDLowerProblem(const ivector& first, const ivector& leaderPrices, int FlipCount, const Instance& instance)
{
	ivector followerPrices = first;
	clientProblemSolver.Solve(leaderPrices, followerPrices, instance);
	int maxIncome = clientProblemSolver.followerIncome;

	int iterationCount = 1;
	for (int k = 1; k <= FlipCount; ++iterationCount)
	{
		ivector followerRecordPrices = followerPrices;
		int incomeRecord = maxIncome;

		for (FlipIterator flipIterator(followerPrices, k, instance.qUpperBound); !flipIterator.End(); ++flipIterator)
		{
			ivector tmpFollowerPrices = *flipIterator;

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

ivector VNDFollowerProblemSolver::VNDLowerProblemFirstImprove(const ivector& first, const ivector& leaderPrices, int FlipCount, const Instance& instance)
{
	ivector followerPrices = first;
	clientProblemSolver.Solve(leaderPrices, followerPrices, instance);
	int followerIncome = clientProblemSolver.followerIncome;

	int iterationCount = 1;
	for (int k = 1; k <= FlipCount; ++iterationCount)
	{
		bool wasImproved = false;
		for (FlipIterator flipIterator(followerPrices, k, instance.qUpperBound); !flipIterator.End(); ++flipIterator)
		{

			ivector tmpFollowerPrices = *flipIterator;

			clientProblemSolver.Solve(leaderPrices, tmpFollowerPrices, instance);
			int tmpIncome = clientProblemSolver.followerIncome;

			if (tmpIncome > followerIncome)
			{
				followerPrices = std::move(tmpFollowerPrices);
				followerIncome = tmpIncome;
				k = 1;
				wasImproved = true;
				break;
			}
		}
		if (!wasImproved)
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

	for (FlipIterator flipIterator(followerPrices, instance.followerFacilityCount, instance.qUpperBound); !flipIterator.End(); ++flipIterator)
	{
		ivector tmpFollowerPrices = *flipIterator;

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