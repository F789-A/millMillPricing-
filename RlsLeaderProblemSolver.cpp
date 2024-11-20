#include "RlsLeaderProblemSolver.h"

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
	std::uniform_int_distribution<> distrib1(0, static_cast<int>(startPrice.size()) - 1);

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

int RlsLeaderProblemSolver::RlsUpperProblem(const Instance& instance)
{
	//const int maxIterCount = 1000 * instance.leaderFacilityCount;
	const int maxIterCount = std::accumulate(instance.pUpperBound.begin(), instance.pUpperBound.end(), 0);

	int iterationCount = 0;

	ivector leaderPrices = GetFirst(instance, true);

	int leaderIncome = 0;
	int prevLeaderIncome = 0;
	ivector prevLeaderPrices;

	//std::set<ivector, VectorCmp> tabu;
	
	for(; true; ++iterationCount)
	{
		int leaderIncomeFindedInPrevIter = leaderIncome;

		FollowerCooperativeExactSolver followerSolver(leaderPrices, instance);
		ivector followerPrices = followerSolver.prices;
		clientProblemSolver.Solve(leaderPrices, followerPrices, instance);
		leaderIncome = clientProblemSolver.leaderIncome;

		std::cout << std::setw(3) << iterationCount << ";" << std::setw(4) << leaderIncomeFindedInPrevIter << ";"
			<< std::setw(4) << leaderIncome << ";" << std::setw(4) << followerSolver.income << ";" << std::endl;

		if (prevLeaderIncome > leaderIncome)
		{
			leaderIncome = prevLeaderIncome;
			leaderPrices = prevLeaderPrices;
			break;
		}
		prevLeaderPrices = leaderPrices;
		prevLeaderIncome = leaderIncome;

		ivector leaderRecordPrices = leaderPrices;
		int incomeRecord = leaderIncome;

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

			RlsFollowerProblemSolver rlsFollowerProblemSolver;
			int followerIterationCount = 0;
			ivector tmpFollowerPrices = rlsFollowerProblemSolver.RlsLowerProblem(followerPrices, tmpLeaderPrices, instance, followerIterationCount);
			tmpFollowerPrices = followerSolver.prices;

			clientProblemSolver.Solve(tmpLeaderPrices, tmpFollowerPrices, instance);
			int tmpIncome = clientProblemSolver.leaderIncome;
			if (tmpIncome > incomeRecord)
			{
				leaderRecordPrices = tmpLeaderPrices;
				incomeRecord = tmpIncome;
			}
		}

		if (incomeRecord > leaderIncome)
		{
			leaderPrices = leaderRecordPrices;
			leaderIncome = incomeRecord;
		}
		else
		{

			++iterationCount;

			FollowerCooperativeExactSolver followerSolver(leaderRecordPrices, instance);
			ivector followerPrices = followerSolver.prices;
			clientProblemSolver.Solve(leaderPrices, followerPrices, instance);
			std::cout << std::setw(3) << iterationCount << ";" << std::setw(4) << leaderIncomeFindedInPrevIter << ";"
				<< std::setw(4) << leaderIncome << ";" << std::setw(4) << followerSolver.income << ";" << std::endl;
			break;
		}
	}

	FollowerCooperativeExactSolver followerSolver = FollowerCooperativeExactSolver(leaderPrices, instance);
	ivector followerPrices = followerSolver.prices;

	clientProblemSolver.Solve(leaderPrices, followerPrices, instance);
	int result = clientProblemSolver.leaderIncome;

	std::cout << "Expected leader income: " << leaderIncome
		<< "Exact leader income: " << result
		<< "; Iteration count: " << iterationCount << std::endl;

	return result;
}

int RlsLeaderProblemSolver::RlsUpperProblemExactLower(const Instance& instance)
{
	const int maxIterCount = std::accumulate(instance.pUpperBound.begin(), instance.pUpperBound.end(), 0);

	int iterationCount = 0;

	ivector leaderPrices = GetFirst(instance, true);

	FollowerCooperativeExactSolver followerSolver(leaderPrices, instance);
	ivector followerPrices = followerSolver.prices;
	clientProblemSolver.Solve(leaderPrices, followerPrices, instance);
	int leaderIncome = clientProblemSolver.leaderIncome;

	int followerLastIncome = clientProblemSolver.followerIncome;

	for (; true; ++iterationCount)
	{
		std::cout << std::setw(3) << iterationCount << ";"
			<< std::setw(4) << leaderIncome << ";" << std::setw(4) << followerLastIncome << ";" << std::endl;

		ivector leaderRecordPrices = leaderPrices;
		int incomeRecord = leaderIncome;

		int price = 0;
		int facility = 0;
		for (int i = 0; i < maxIterCount; ++i)
		{
			ivector tmpLeaderPrices = GetFromFlip(leaderPrices, price, facility, instance);

			FollowerCooperativeExactSolver _followerSolver(tmpLeaderPrices, instance);
			ivector tmpFollowerPrices = _followerSolver.prices;

			clientProblemSolver.Solve(tmpLeaderPrices, tmpFollowerPrices, instance);
			int tmpIncome = clientProblemSolver.leaderIncome;
			if (tmpIncome > incomeRecord)
			{
				leaderRecordPrices = tmpLeaderPrices;
				incomeRecord = tmpIncome;

				followerLastIncome = clientProblemSolver.followerIncome;
			}
		}

		if (incomeRecord > leaderIncome)
		{
			leaderPrices = leaderRecordPrices;
			leaderIncome = incomeRecord;
		}
		else
		{
			++iterationCount;

			std::cout << std::setw(3) << iterationCount << ";"
				<< std::setw(4) << leaderIncome << ";" << std::setw(4) << followerLastIncome << ";" << std::endl;
			break;
		}
	}

	FollowerCooperativeExactSolver _followerSolver = FollowerCooperativeExactSolver(leaderPrices, instance);
	followerPrices = _followerSolver.prices;

	clientProblemSolver.Solve(leaderPrices, followerPrices, instance);
	int result = clientProblemSolver.leaderIncome;

	std::cout << "Exact leader income: " << result
		<< "; Iteration count: " << iterationCount << std::endl;

	return result;
}