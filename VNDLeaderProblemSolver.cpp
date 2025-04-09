#include "VNDLeaderProblemSolver.h"

ivector VNDLeaderProblemSolver::GetFirst(const Instance& instance)
{
	ivector result = instance.pUpperBound;
	for (auto& l : result)
	{
		l /= 2;
	}
	return result;
}

int VNDLeaderProblemSolver::VNDUpperProblem(int LeaderFlipCount, int FollowerFlipCount, const Instance& instance, const std::chrono::milliseconds TimeLimit)
{
	std::chrono::high_resolution_clock timer;
	auto startTime = timer.now();

	ivector leaderPrices = GetFirst(instance);
	auto followerSolution = followerProblemSolver.Solve(leaderPrices, instance);
	ivector followerPrices = std::move(followerSolution.followerPrices);
	int leaderIncome = followerSolution.leaderIncome;

	int iterationCount = 1;
	for(int k = 1; k <= LeaderFlipCount; ++iterationCount)
	{
		ivector leaderRecordPrices = leaderPrices;
		int incomeRecord = leaderIncome;
		int followerPseudoIncome = 0;

		for (FlipIterator flipIterator(leaderPrices, k, instance.pUpperBound); !flipIterator.End(); ++flipIterator)
		{
			auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - startTime);
			if (deltaTime > TimeLimit)
			{
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

			deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - startTime);
			if (deltaTime > TimeLimit)
			{
				return leaderIncome;
			}
		}

		followerSolution = followerProblemSolver.Solve(leaderRecordPrices, instance);
		const ivector& followerPricesOnRecord = followerSolution.followerPrices;
		clientProblemSolver.Solve(leaderRecordPrices, followerPricesOnRecord, instance);
		int leaderIncomeOnRecord = clientProblemSolver.leaderIncome;

		auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - startTime);
		if (deltaTime > TimeLimit)
		{
			return leaderIncome;
		}

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

std::pair<int, std::chrono::milliseconds> VNDLeaderProblemSolver::VNDUpperVNDFirstImproveLower(int LeaderFlipCount, int FollowerFlipCount, const Instance& instance, const std::chrono::milliseconds TimeLimit, bool unlimited)
{
	std::chrono::high_resolution_clock timer;
	auto startTime = timer.now();

	ivector leaderPrices = GetFirst(instance);
	auto followerSolution = followerProblemSolver.Solve(leaderPrices, instance);
	ivector followerPrices = std::move(followerSolution.followerPrices);
	int leaderIncome = followerSolution.leaderIncome;

	int stackSize = !unlimited ? std::lround(instance.leaderFacilityCount * 1.5f) : std::numeric_limits<int>::max();

	int iterationCount = 1;
	for (int k = 1; k <= LeaderFlipCount; ++iterationCount)
	{
		std::vector<std::tuple<int, ivector, ivector>> stack;
		auto insertInStackIfNeed = [&stack, stackSize](int pseudoLeaderIncome, const ivector& leaderNewPrice, const ivector& followerPrice)
		{
			if (stack.size() < stackSize)
			{
				stack.push_back(std::make_tuple(pseudoLeaderIncome, leaderNewPrice, followerPrice));
			}
			else
			{
				int minEl = 0;
				for (int i = 1; i < stack.size(); ++i)
				{
					if (std::get<0>(stack[minEl]) > std::get<0>(stack[i]))
					{
						minEl = i;
					}
				}
				if (std::get<0>(stack[minEl]) < pseudoLeaderIncome)
				{
					stack[minEl] = std::make_tuple(pseudoLeaderIncome, leaderNewPrice, followerPrice);
				}
			}
		};

		for (FlipIterator flipIterator(leaderPrices, k, instance.pUpperBound); !flipIterator.End(); ++flipIterator)
		{
			auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - startTime);
			if (deltaTime > TimeLimit)
			{
				return { leaderIncome, TimeLimit };
			}

			ivector tmpLeaderPrices = *flipIterator;
			ivector tmpFollowerPrices = vndFollowerProblemSolver.VNDLowerProblemFirstImprove(followerPrices, tmpLeaderPrices, FollowerFlipCount, instance);

			clientProblemSolver.Solve(tmpLeaderPrices, tmpFollowerPrices, instance);
			int tmpIncome = clientProblemSolver.leaderIncome;
			insertInStackIfNeed(tmpIncome, tmpLeaderPrices, tmpFollowerPrices);

			deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - startTime);
			if (deltaTime > TimeLimit)
			{
				return { leaderIncome, TimeLimit };
			}
		}

		std::sort(stack.begin(), stack.end(), [](const std::tuple<float, ivector, ivector>& a,
			const std::tuple<float, ivector, ivector>& b)
			{
				return std::get<0>(a) > std::get<0>(b); 
			}
		);
		bool improved = false;
		for (auto& [leaderPseudoIncome, leaderNewPrices, followerHint] : stack)
		{
			followerSolution = followerProblemSolver.Solve(leaderNewPrices, instance, false, followerHint);
			const ivector& followerPricesOnRecord = followerSolution.followerPrices;
			clientProblemSolver.Solve(leaderNewPrices, followerPricesOnRecord, instance);
			int leaderIncomeOnRecord = clientProblemSolver.leaderIncome;

			if (leaderIncomeOnRecord > leaderIncome)
			{
				leaderPrices = leaderNewPrices;
				leaderIncome = leaderIncomeOnRecord;
				followerPrices = followerPricesOnRecord;
				improved = true;
				break;
			}
			if (leaderIncomeOnRecord == leaderPseudoIncome)
			{
				break;
			}
		}

		if (improved)
		{
			k = 1;
		}
		else
		{
			++k;
		}

	}

	auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - startTime);
	return { leaderIncome, deltaTime };
}

std::pair<int, std::chrono::milliseconds>  VNDLeaderProblemSolver::LSUpperProblemExactLower(const Instance& instance, const std::chrono::milliseconds TimeLimit)
{
	std::chrono::high_resolution_clock timer;
	auto startTime = timer.now();

	ivector leaderPrices = GetFirst(instance);
	auto followerSolution = followerProblemSolver.Solve(leaderPrices, instance);
	ivector followerPrices = followerSolution.followerPrices;
	clientProblemSolver.Solve(leaderPrices, followerPrices, instance);
	int leaderIncome = clientProblemSolver.leaderIncome;
	int followerIncome = clientProblemSolver.followerIncome;

	int iterationCount = 1;
	for (; true; ++iterationCount)
	{
		auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - startTime);
		if (deltaTime > TimeLimit)
		{
			return { leaderIncome, deltaTime };
		}

		ivector leaderRecordPrices = leaderPrices;
		int leaderIncomeOnRecord = leaderIncome;

		for (FlipIterator flipIterator(leaderPrices, 1, instance.pUpperBound); !flipIterator.End(); ++flipIterator)
		{
			deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - startTime);
			if (deltaTime > TimeLimit)
			{
				return { leaderIncome, deltaTime };
			}

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

	auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - startTime);
	return { leaderIncome, deltaTime };
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