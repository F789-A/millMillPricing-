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

int VNDLeaderProblemSolver::VNDUpperProblem(int LeaderFlipCount, int FollowerFlipCount, const Instance& instance, bool& timeExpired)
{
	timeExpired = false;

	std::chrono::high_resolution_clock timer;
	auto startTime = timer.now();

	ivector leaderPrices = GetFirst(instance, true);
	auto followerSolution = followerProblemSolver.Solve(leaderPrices, instance);
	ivector followerPrices = std::move(followerSolution.followerPrices);
	int leaderIncome = followerSolution.leaderIncome;

	int iterationCount = 1;
	int k = 1;
	for(; k <= LeaderFlipCount; ++iterationCount)
	{
		ivector leaderRecordPrices = leaderPrices;
		int incomeRecord = leaderIncome;
		int followerPseudoIncome = 0;

		for (FlipIterator flipIterator(leaderPrices, k, instance.pUpperBound); !flipIterator.End(); ++flipIterator)
		{
			auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - startTime).count() / 1000.0f;
			if (deltaTime > 3600)
			{
				timeExpired = true;
				return leaderIncome;
			}

			ivector tmpLeaderPrices = *flipIterator;
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

	return leaderIncome;
}

int VNDLeaderProblemSolver::LSUpperProblemExactLower(const Instance& instance, bool& ended)
{
	ended = false;
	std::chrono::high_resolution_clock timer;
	auto startTime = timer.now();

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
		auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - startTime).count() / 1000.0f;
		if (deltaTime > 3600)
		{
			ended = true;
			return leaderIncome;
		}

		ivector leaderRecordPrices = leaderPrices;
		int leaderIncomeOnRecord = leaderIncome;

		for (FlipIterator flipIterator(leaderPrices, 1, instance.pUpperBound); !flipIterator.End(); ++flipIterator)
		{
			ivector tmpLeaderPrices = *flipIterator;

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

	return leaderIncome;
}

int VNDLeaderProblemSolver::ExactUpperProblem(const Instance& instance, bool& ended)
{
	ivector leaderPrices(instance.leaderFacilityCount, 0);
	FollowerCooperativeExactOutput followerSolution;
	int leaderIncome = 0;

	for (FlipIterator flipIterator(leaderPrices, instance.leaderFacilityCount, instance.pUpperBound); !flipIterator.End(); ++flipIterator)
	{
		ivector tmpLeaderPrices = *flipIterator;

		followerSolution = followerProblemSolver.Solve(tmpLeaderPrices, instance);
		clientProblemSolver.Solve(tmpLeaderPrices, followerSolution.followerPrices, instance);
		if (clientProblemSolver.leaderIncome > leaderIncome)
		{
			leaderIncome = clientProblemSolver.leaderIncome;
		}
	}

	return leaderIncome;
}

int VNDLeaderProblemSolver::ExactUpperProblem2(const Instance& instance, bool& ended)
{
	ivector leaderPrices(instance.leaderFacilityCount, 0);
	int leaderIncome = 0;

	for (FlipIterator flipIterator(leaderPrices, instance.leaderFacilityCount, instance.pUpperBound); !flipIterator.End(); ++flipIterator)
	{
		ivector tmpLeaderPrices = *flipIterator;

		auto followerPrices = vndFollowerProblemSolver.SearchLowerProblem(tmpLeaderPrices, instance);
		clientProblemSolver.Solve(tmpLeaderPrices, followerPrices, instance);
		if (clientProblemSolver.leaderIncome > leaderIncome)
		{
			leaderIncome = clientProblemSolver.leaderIncome;
		}
	}

	return leaderIncome;
}