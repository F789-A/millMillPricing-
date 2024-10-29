#include "RlsLeaderProblemSolver.h"

int RlsLeaderProblemSolver::Solve(const ivector& leaderPrice, const ivector& followerPrice, const Instance& instance)
{
	int income = 0;
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		int tmpIncome = 0;
		int minLeaderCost = std::numeric_limits<int>::max();
		for (int i = 0; i < instance.leaderFacilityCount; ++i)
		{
			int tmpMinLeaderCost = instance.costsLeader[i][j] + leaderPrice[i];
			if (instance.budgets[j] - tmpMinLeaderCost >= 0 && minLeaderCost >= tmpMinLeaderCost)
			{
				minLeaderCost = tmpMinLeaderCost;
				tmpIncome = leaderPrice[i];
			}
		}
		int minFollowerCost = std::numeric_limits<int>::max();
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			int tmpMinFollowerCost = instance.costsFollower[i][j] + followerPrice[i];
			if (instance.budgets[j] - tmpMinFollowerCost >= 0 && minFollowerCost >= tmpMinFollowerCost)
			{
				minFollowerCost = tmpMinFollowerCost;
			}
		}
		if (minLeaderCost < minFollowerCost)
		{
			income += tmpIncome;
		}
	}
	return income;
}

ivector RlsLeaderProblemSolver::GetFirst(const Instance& instance, bool upper)
{
	ivector result = upper ? instance.pUpperBound : instance.qUpperBound;
	for (auto& l : result)
	{
		l /= 2;
	}
	return result;
}

ivector RlsLeaderProblemSolver::GetRandomFromFlip(const ivector& startPrice, const Instance& instance)
{
	static std::seed_seq seed_w({ 123123 });
	static auto random_generator = std::mt19937(seed_w);
	std::uniform_int_distribution<> distrib1(0, startPrice.size() - 1);

	int facility = distrib1(random_generator);

	int pBoundMin = 0;
	int pBoundMax = instance.pUpperBound[facility];

	std::uniform_int_distribution<> distrib2(pBoundMin, pBoundMax);
	ivector result = startPrice;
	result[facility] = distrib2(random_generator);

	return result;
}

ivector RlsLeaderProblemSolver::GetFromFlip(const ivector& startPrice, int& price, int& facility, const Instance& instance)
{
	ivector result = startPrice;
	result[facility] = price;
	++price;
	if (price > instance.pUpperBound[facility])
	{
		++facility;
		price = 0;
	}
	return result;
}

int RlsLeaderProblemSolver::RlsUpperProblem(const Instance& instance, bool exactLower)
{
	//const int maxIterCount = 1000 * instance.leaderFacilityCount;
	const int maxIterCount = std::accumulate(instance.pUpperBound.begin(), instance.pUpperBound.end(), 0);

	int followerIterationCount = 0;
	int followerRlsCount = 0;
	int iterationCount = 0;

	ivector leaderPrices = GetFirst(instance, true);

	int followerIncome = 0;
	int leaderIncome = 0;

	int prevLeaderIncome = 0;

	std::set<ivector, VectorCmp> tabu;

	while (true)
	{
		++iterationCount;

		FollowerCooperativeExactSolver followerSolver(leaderPrices, instance);
		ivector followerPrices = followerSolver.prices;

		int tmp = leaderIncome;

		followerIncome = followerSolver.income;
		leaderIncome = Solve(leaderPrices, followerPrices, instance);

		std::cout << iterationCount << "; " << tmp << "; " << leaderIncome << std::endl;

		if (leaderIncome == prevLeaderIncome)
		{
			break;
		}
		
		prevLeaderIncome = leaderIncome;

		ivector leaderRecordPrices = leaderPrices;
		ivector followerRecordPrices = followerPrices;
		int incomeRecord = leaderIncome;
		int followerIncomeRecord = followerIncome;

		int price = 0;
		int facility = 0;
		for (int i = 0; i < maxIterCount; ++i)
		{
			//ivector tmpLeaderPrices = GetRandomFromFlip(leaderPrices, instance);
			//while (tabu.contains(tmpLeaderPrices))
			//{
			//	tmpLeaderPrices = GetRandomFromFlip(leaderPrices, instance);
			//}
			//tabu.insert(tmpLeaderPrices);
			//ivector tmpLeaderPrices = GetRandomFromFlip(leaderPrices, instance);
			ivector tmpLeaderPrices = GetFromFlip(leaderPrices, price, facility, instance);

			int followerIncomeTmp = 0;

			RlsFollowerProblemSolver rlsFollowerProblemSolver;

			ivector tmpFollowerPrices = rlsFollowerProblemSolver.RlsLowerProblem(followerPrices, tmpLeaderPrices, instance, followerIterationCount, followerIncomeTmp);
			++followerRlsCount;

			int income = Solve(tmpLeaderPrices, tmpFollowerPrices, instance);
			if (income > incomeRecord)
			{
				leaderRecordPrices = tmpLeaderPrices;
				followerRecordPrices = tmpFollowerPrices;
				incomeRecord = income;
				followerIncomeRecord = followerIncomeTmp;
			}
		}

		if (incomeRecord > leaderIncome)
		{
			leaderPrices = leaderRecordPrices;
			followerPrices = followerRecordPrices;
			leaderIncome = incomeRecord;
			followerIncome = followerIncomeRecord;
		}
		else
		{
			break;
		}
	}

	FollowerCooperativeExactSolver followerSolver = FollowerCooperativeExactSolver(leaderPrices, instance);
	ivector followerPrices = followerSolver.prices;
	int result = Solve(leaderPrices, followerPrices, instance);

	std::cout << "Expected follower income: " << followerIncome
		<< "; Exact follower income: " << followerSolver.income
		<< "; Iteration follower average count: " << (float)followerIterationCount / followerRlsCount << std::endl;
	std::cout << "Expected leader income: " << leaderIncome
		<< "; Exact leader income: " << result
		<< "; Iteration count: " << iterationCount << std::endl;

	return result;
}