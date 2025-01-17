#include "VNDLeaderProblemSolver.h"

ivector VNDLeaderProblemSolver::GetFirst(const Instance& instance, bool upper)
{
	ivector result = upper ? instance.pUpperBound : instance.qUpperBound;
	for (auto& l : result)
	{
		l /= 2;
	}
	return result;
}

ivector VNDLeaderProblemSolver::GetRandomFromFlip(const ivector& startPrice, const Instance& instance)
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

ivector VNDLeaderProblemSolver::GetFromFlip(const ivector& startPrice, std::vector<int>& priceIter, 
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

int VNDLeaderProblemSolver::VNDUpperProblem(int LeaderFlipCount, int FollowerFlipCount, const Instance& instance)
{
	ivector leaderPrices = GetFirst(instance, true);
	auto followerSolution = followerProblemSolver.Solve(leaderPrices, instance);
	ivector followerPrices = std::move(followerSolution.followerPrices);
	int leaderIncome = followerSolution.leaderIncome;

	//std::cout << std::setw(3) << 0 << ";" << std::setw(4) << 0 << ";" << std::setw(4) << leaderIncome << ";" << std::setw(4) << "0"  
	//	<< ";" << std::setw(4) << followerSolution.followerIncome  << std::endl;

	int iterationCount = 1;
	int k = 1;
	for(; k <= LeaderFlipCount; ++iterationCount)
	{
		ivector leaderRecordPrices = leaderPrices;
		int incomeRecord = leaderIncome;
		int followerPseudoIncome = 0;

		SubsetIterator facilityIterator(instance.leaderFacilityCount, k);
		std::vector<int> facilityPrices(k, 0);
		while (!facilityIterator.End())
		{
			ivector tmpLeaderPrices = GetFromFlip(leaderPrices, facilityPrices, facilityIterator, instance);
			ivector tmpFollowerPrices = vndFollowerProblemSolver.VNDLowerProblem(followerPrices, tmpLeaderPrices, FollowerFlipCount, instance);

			clientProblemSolver.Solve(tmpLeaderPrices, tmpFollowerPrices, instance);
			int tmpIncome = clientProblemSolver.leaderIncome;
			if (tmpIncome > incomeRecord)
			{
				leaderRecordPrices = std::move(tmpLeaderPrices);
				incomeRecord = tmpIncome;
				followerPseudoIncome = clientProblemSolver.followerIncome;
			}
		}

		followerSolution = followerProblemSolver.Solve(leaderRecordPrices, instance);
		const ivector& followerPricesOnRecord = followerSolution.followerPrices;
		clientProblemSolver.Solve(leaderRecordPrices, followerPricesOnRecord, instance);
		int leaderIncomeOnRecord = clientProblemSolver.leaderIncome;

		//std::cout << std::setw(3) << iterationCount << ";" << std::setw(4) << incomeRecord << ";"
		//	<< std::setw(4) << leaderIncomeOnRecord << ";" << std::setw(4) << followerPseudoIncome << ";" << std::setw(4)
		//	<< followerSolution.followerIncome << ";" << " k = " << k << std::endl;

		if (leaderIncomeOnRecord > leaderIncome)
		{
			leaderPrices = leaderRecordPrices;
			leaderIncome = leaderIncomeOnRecord;
			followerPrices = followerPricesOnRecord;
			k = 1;
		}
		else
		{
			++k;
		}
	}

	followerSolution = followerProblemSolver.Solve(leaderPrices, instance);
	clientProblemSolver.Solve(leaderPrices, followerSolution.followerPrices, instance);
	int result = clientProblemSolver.leaderIncome;

	return result;
}

int VNDLeaderProblemSolver::LSUpperProblemExactLower(const Instance& instance)
{
	ivector leaderPrices = GetFirst(instance, true);
	auto followerSolution = followerProblemSolver.Solve(leaderPrices, instance);
	ivector followerPrices = followerSolution.followerPrices;
	clientProblemSolver.Solve(leaderPrices, followerPrices, instance);
	int leaderIncome = clientProblemSolver.leaderIncome;
	int followerIncome = clientProblemSolver.followerIncome;

	//std::cout << std::setw(3) << 0 << ";"
		//<< std::setw(4) << leaderIncome << ";" << std::setw(4) << followerIncome << ";" << std::endl;

	int iterationCount = 1;
	for (; true; ++iterationCount)
	{
		ivector leaderRecordPrices = leaderPrices;
		int leaderIncomeOnRecord = leaderIncome;

		SubsetIterator facilityIterator(instance.leaderFacilityCount, 1);
		std::vector<int> facilityPrices(1, 0);
		while (!facilityIterator.End())
		{
			int fas = (*facilityIterator)[0];
			int pr = facilityPrices[0];

			ivector tmpLeaderPrices = GetFromFlip(leaderPrices, facilityPrices, facilityIterator, instance);

			followerSolution = followerProblemSolver.Solve(tmpLeaderPrices, instance);
			const ivector& tmpFollowerPrices = followerSolution.followerPrices;
			clientProblemSolver.Solve(tmpLeaderPrices, tmpFollowerPrices, instance);
			const int tmpIncome = clientProblemSolver.leaderIncome;
			if (tmpIncome > leaderIncomeOnRecord)
			{
				leaderRecordPrices = std::move(tmpLeaderPrices);
				leaderIncomeOnRecord = tmpIncome;
			}
		}

		if (leaderIncomeOnRecord > leaderIncome)
		{
			leaderPrices = std::move(leaderRecordPrices);
			leaderIncome = leaderIncomeOnRecord;
		}
		else
		{
			break;
		}
	}

	//std::cout << "Exact leader income: " << leaderIncome
		//<< "; Iteration count: " << iterationCount << std::endl;

	return leaderIncome;
}