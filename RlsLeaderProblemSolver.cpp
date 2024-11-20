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

ivector RlsLeaderProblemSolver::GetFromFlip(const ivector& startPrice, std::vector<int>& priceIter, 
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
		if (priceIter[i] > instance.pUpperBound[facilityes[i]])
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

int RlsLeaderProblemSolver::VNDUpperProblem(int FlipCount, const Instance& instance)
{
	ivector leaderPrices = GetFirst(instance, true);
	FollowerCooperativeExactSolver followerSolver(leaderPrices, instance);
	ivector followerPrices = followerSolver.prices;
	clientProblemSolver.Solve(leaderPrices, followerPrices, instance);
	int leaderIncome = clientProblemSolver.leaderIncome;

	std::cout << std::setw(3) << "Itr;" << std::setw(4) << "PInc;"
		<< std::setw(4) << "RInc;" << std::setw(4) << "FInc;" << std::endl;
	std::cout << std::setw(3) << 0 << ";" << std::setw(4) << 0 << ";" << std::setw(4) << leaderIncome << ";" << std::setw(4) << followerSolver.income << ";" << std::endl;

	for (auto l : leaderPrices)
	{
		std::cout << l << " ";
	}
	std::cout << std::endl;
	for (auto l : followerPrices)
	{
		std::cout << l << " ";
	}
	std::cout << std::endl;
	for (const auto& p : clientProblemSolver.debInfo)
	{
		if (p.second)
		{
			std::cout << "l" << p.first << " ";
		}
		else if (p.first != -1)
		{
			std::cout << "f" << p.first << " ";
		}
		else
		{
			std::cout << "u  ";
		}
	}
	std::cout << std::endl;

	int iterationCount = 1;
	int k = 1;
	for(; k <= FlipCount; ++iterationCount)
	{
		ivector leaderRecordPrices = leaderPrices;
		int incomeRecord = leaderIncome;

		SubsetIterator facilityIterator(instance.leaderFacilityCount, k);
		std::vector<int> facilityPrices(k, 0);
		while (!facilityIterator.End())
		{
			ivector tmpLeaderPrices = GetFromFlip(leaderPrices, facilityPrices, facilityIterator, instance);

			RlsFollowerProblemSolver rlsFollowerProblemSolver;
			int followerIterationCount = 0;
			ivector tmpFollowerPrices = rlsFollowerProblemSolver.RlsLowerProblem(followerPrices, tmpLeaderPrices, instance, followerIterationCount);

			clientProblemSolver.Solve(tmpLeaderPrices, tmpFollowerPrices, instance);
			int tmpIncome = clientProblemSolver.leaderIncome;
			if (tmpIncome > incomeRecord)
			{
				leaderRecordPrices = tmpLeaderPrices;
				incomeRecord = tmpIncome;
			}
		}

		FollowerCooperativeExactSolver followerSolverOnRecord(leaderRecordPrices, instance);
		const ivector& followerPricesOnRecord = followerSolverOnRecord.prices;
		clientProblemSolver.Solve(leaderRecordPrices, followerPricesOnRecord, instance);
		int leaderIncomeOnRecord = clientProblemSolver.leaderIncome;

		std::cout << std::setw(3) << iterationCount << ";" << std::setw(4) << incomeRecord << ";"
			<< std::setw(4) << leaderIncomeOnRecord << ";" << std::setw(4) << followerSolverOnRecord.income << ";" 
			<< " k = " << k << std::endl;

		for (auto l : leaderRecordPrices)
		{
			std::cout << l << " ";
		}
		std::cout << std::endl;
		for (auto l : followerPricesOnRecord)
		{
			std::cout << l << " ";
		}
		std::cout << std::endl;
		for (const auto& p : clientProblemSolver.debInfo)
		{
			if (p.second)
			{
				std::cout << "l" << p.first << " ";
			}
			else if (p.first != -1)
			{
				std::cout << "f" << p.first << " ";
			}
			else 
			{
				std::cout << "u  ";
			}
		}
		std::cout << std::endl;

		if (leaderIncomeOnRecord > leaderIncome)
		{
			leaderPrices = leaderRecordPrices;
			leaderIncome = incomeRecord;
			followerPrices = followerPricesOnRecord;
			k = 1;
		}
		else
		{
			++k;
		}
	}

	FollowerCooperativeExactSolver _followerSolver = FollowerCooperativeExactSolver(leaderPrices, instance);
	clientProblemSolver.Solve(leaderPrices, _followerSolver.prices, instance);
	int result = clientProblemSolver.leaderIncome;

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

		int k = 1;
		SubsetIterator facilityIterator(instance.leaderFacilityCount, k);
		std::vector<int> facilityPrices(k, 0);
		while (!facilityIterator.End())
		{
			ivector tmpLeaderPrices = GetFromFlip(leaderPrices, facilityPrices, facilityIterator, instance);

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