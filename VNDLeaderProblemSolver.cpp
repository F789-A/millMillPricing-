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

void VNDLeaderProblemSolver::PrintClientInfo(const ivector& leaderPrices, const ivector& followerPrices, const std::vector<std::pair<int, bool>>& debInfo)
{
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
	for (const auto& p : debInfo)
	{
		if (p.first != -1)
		{
			if (p.second)
			{
				std::cout << "l" << p.first << " ";
			}
			else
			{
				std::cout << "f" << p.first << " ";
			}
		}
		else
		{
			std::cout << "u  ";
		}
	}
	std::cout << std::endl;
}

int VNDLeaderProblemSolver::VNDUpperProblem(int FlipCount, const Instance& instance)
{
	ivector leaderPrices = GetFirst(instance, true);
	FollowerCooperativeExactSolver followerSolver(leaderPrices, instance);
	ivector followerPrices = followerSolver.prices;
	clientProblemSolver.Solve(leaderPrices, followerPrices, instance);
	int leaderIncome = clientProblemSolver.leaderIncome;

	std::cout << std::setw(3) << "Itr;" << std::setw(4) << "PInc;"
		<< std::setw(4) << "RInc;" << std::setw(4) << "FInc;" << "FPInc" << std::endl;

	std::cout << std::setw(3) << 0 << ";" << std::setw(4) << 0 << ";" << std::setw(4) << leaderIncome << ";" << std::setw(4) << "0"  
		<< ";" << std::setw(4) << followerSolver.income  << std::endl;
	PrintClientInfo(leaderPrices, followerPrices, clientProblemSolver.debInfo);

	int iterationCount = 1;
	int k = 1;
	for(; k <= FlipCount; ++iterationCount)
	{
		ivector leaderRecordPrices = leaderPrices;
		int incomeRecord = leaderIncome;
		int followerPseudoIncome = 0;

		SubsetIterator facilityIterator(instance.leaderFacilityCount, k);
		std::vector<int> facilityPrices(k, 0);
		while (!facilityIterator.End())
		{
			ivector tmpLeaderPrices = GetFromFlip(leaderPrices, facilityPrices, facilityIterator, instance);

			VNDFollowerProblemSolver rlsFollowerProblemSolver;
			int followerIterationCount = 0;
			ivector tmpFollowerPrices = rlsFollowerProblemSolver.RlsLowerProblem(followerPrices, tmpLeaderPrices, 2, instance);

			clientProblemSolver.Solve(tmpLeaderPrices, tmpFollowerPrices, instance);
			int tmpIncome = clientProblemSolver.leaderIncome;
			if (tmpIncome > incomeRecord)
			{
				leaderRecordPrices = std::move(tmpLeaderPrices);
				incomeRecord = tmpIncome;
				followerPseudoIncome = clientProblemSolver.followerIncome;
			}
		}

		FollowerCooperativeExactSolver followerSolverOnRecord(leaderRecordPrices, instance);
		const ivector& followerPricesOnRecord = followerSolverOnRecord.prices;
		clientProblemSolver.Solve(leaderRecordPrices, followerPricesOnRecord, instance);
		int leaderIncomeOnRecord = clientProblemSolver.leaderIncome;

		std::cout << std::setw(3) << iterationCount << ";" << std::setw(4) << incomeRecord << ";"
			<< std::setw(4) << leaderIncomeOnRecord << ";" << std::setw(4) << followerPseudoIncome << ";" << std::setw(4)
			<< followerSolverOnRecord.income << ";" << " k = " << k << std::endl;
		PrintClientInfo(leaderRecordPrices, followerPricesOnRecord, clientProblemSolver.debInfo);

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

	FollowerCooperativeExactSolver _followerSolver = FollowerCooperativeExactSolver(leaderPrices, instance);
	clientProblemSolver.Solve(leaderPrices, _followerSolver.prices, instance);
	int result = clientProblemSolver.leaderIncome;

	return result;
}

int VNDLeaderProblemSolver::LSUpperProblemExactLower(const Instance& instance)
{
	std::ofstream out("C:/Workflow/Cpp/millMillPricing/resultGrapgics.txt");

	ivector leaderPrices = GetFirst(instance, true);
	FollowerCooperativeExactSolver followerSolver(leaderPrices, instance);
	ivector followerPrices = followerSolver.prices;
	clientProblemSolver.Solve(leaderPrices, followerPrices, instance);
	int leaderIncome = clientProblemSolver.leaderIncome;
	int followerIncome = clientProblemSolver.followerIncome;

	std::cout << std::setw(3) << 0 << ";"
		<< std::setw(4) << leaderIncome << ";" << std::setw(4) << followerIncome << ";" << std::endl;

	int iterationCount = 1;
	for (; true; ++iterationCount)
	{
		out << "Start iteration " << iterationCount << std::endl;
		out << "Start leader price ";
		for (auto l : leaderPrices)
		{
			out << l << " ";
		}
		out << std::endl;
		out << "Start follower price ";
		for (auto l : followerPrices)
		{
			out << l << " ";
		}
		out << std::endl;
		std::vector<std::vector<int>> leaderIncomesOnTmpPrices(instance.leaderFacilityCount);
		std::vector<std::vector<int>> followerIncomesOnTmpPrices(instance.leaderFacilityCount);
		std::vector<std::vector<std::vector<int>>> followerPriceOnTmpPrices(instance.leaderFacilityCount);
		std::vector<std::vector<int>> tmpPrices(instance.leaderFacilityCount);

		ivector leaderRecordPrices = leaderPrices;
		int incomeRecord = leaderIncome;
		ivector followerRecordPrices = followerPrices;
		int followerIncomeOnRecord = followerIncome;

		SubsetIterator facilityIterator(instance.leaderFacilityCount, 1);
		std::vector<int> facilityPrices(1, 0);
		while (!facilityIterator.End())
		{
			int fas = (*facilityIterator)[0];
			int pr = facilityPrices[0];

			ivector tmpLeaderPrices = GetFromFlip(leaderPrices, facilityPrices, facilityIterator, instance);

			ivector tmpFollowerPrices;
			if (followerPriceOnTmpPrices[fas].size() != 0) {
				FollowerCooperativeExactSolverStable _followerSolver(tmpLeaderPrices, followerPriceOnTmpPrices[fas].back(), instance);
				tmpFollowerPrices = _followerSolver.prices;
			}
			else
			{
				FollowerCooperativeExactSolver _followerSolver(tmpLeaderPrices, instance);
				tmpFollowerPrices = _followerSolver.prices;
			}
			
			clientProblemSolver.Solve(tmpLeaderPrices, tmpFollowerPrices, instance);
			const int tmpIncome = clientProblemSolver.leaderIncome;
			if (tmpIncome > incomeRecord)
			{
				leaderRecordPrices = tmpLeaderPrices;
				incomeRecord = tmpIncome;
				followerRecordPrices = tmpFollowerPrices;

				followerIncomeOnRecord = clientProblemSolver.followerIncome;
			}

			leaderIncomesOnTmpPrices[fas].push_back(tmpIncome);
			followerIncomesOnTmpPrices[fas].push_back(clientProblemSolver.followerIncome);
			followerPriceOnTmpPrices[fas].push_back(tmpFollowerPrices);
			tmpPrices[fas].push_back(pr);
		}

		std::cout << std::setw(3) << iterationCount << ";"
			<< std::setw(4) << incomeRecord << ";" << std::setw(4) << followerIncomeOnRecord << ";" << std::endl;

		for (int i = 0; i < instance.leaderFacilityCount; ++i)
		{
			for (auto l : tmpPrices[i])
			{
				out << l << " ";
			}
			out << std::endl;
			for (auto l : leaderIncomesOnTmpPrices[i])
			{
				out << l << " ";
			}
			out << std::endl;
			for (auto l : followerIncomesOnTmpPrices[i])
			{
				out << l << " ";
			}
			out << std::endl;
			for (auto& l : followerPriceOnTmpPrices[i])
			{
				for (auto ll : l)
				{
					out << ll << " ";
				}
			}
			out << std::endl;
		}

		if (incomeRecord > leaderIncome)
		{
			leaderPrices = leaderRecordPrices;
			leaderIncome = incomeRecord;

			followerPrices = followerRecordPrices;
			followerIncome = followerIncomeOnRecord;
		}
		else
		{
			++iterationCount;
			break;
		}
	}

	std::cout << "Exact leader income: " << leaderIncome
		<< "; Iteration count: " << iterationCount << std::endl;

	return leaderIncome;
}