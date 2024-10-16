#include <vector>
#include <random>
#include <numeric>
#include <map>
#include <queue>
#include <memory>
#include <iostream>
#include <filesystem>
#include <fstream>

#include "FollowerExactSolver.h"
#include "FollowerCooperativeExactSolver.h"
#include "VndSolvers.h"

int Solve(const ivector& leaderPrice, const ivector& followerPrice, const Instance& instance)
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

int SolveLower(const ivector& leaderPrices, const ivector& followerPrices, const Instance& instance)
{
	int income = 0;
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		int tmpIncome = 0;
		int minLeaderCost = std::numeric_limits<int>::max();
		for (int i = 0; i < instance.leaderFacilityCount; ++i)
		{
			int tmpMinLeaderCost = instance.costsLeader[i][j] + leaderPrices[i];
			if (instance.budgets[j] - tmpMinLeaderCost >= 0 && minLeaderCost >= tmpMinLeaderCost)
			{
				minLeaderCost = tmpMinLeaderCost;
			}
		}
		int minFollowerCost = std::numeric_limits<int>::max();
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			int tmpMinFollowerCost = instance.costsFollower[i][j] + followerPrices[i];
			if (instance.budgets[j] - tmpMinFollowerCost >= 0 && minFollowerCost >= tmpMinFollowerCost)
			{
				minFollowerCost = tmpMinFollowerCost;
				tmpIncome = followerPrices[i];
			}
		}
		if (minFollowerCost < minLeaderCost)
		{
			income += tmpIncome;
		}
	}
	return income;
}

ivector GetFirst(const Instance& instance, bool upper)
{
	ivector result = upper ? instance.pUpperBound : instance.qUpperBound;
	for (auto& l : result)
	{
		l /= 2;
	}
	return result;
}

ivector GetRandomFromFlip(const ivector& startPrice, const Instance& instance, bool upper)
{
	static std::seed_seq seed_w({ 123123 });
	static auto random_generator = std::mt19937(seed_w);
	std::uniform_int_distribution<> distrib1(0, startPrice.size() - 1);

	int facility = distrib1(random_generator);

	int pBoundMin = 0;
	int pBoundMax = upper ? instance.pUpperBound[facility] : instance.qUpperBound[facility];

	std::uniform_int_distribution<> distrib2(pBoundMin, pBoundMax);
	ivector result = startPrice;
	result[facility] = distrib2(random_generator);

	return result;
}

ivector VndLowerProblem(const ivector& first, const ivector& leaderPrices, const Instance& instance, int& iterrCount, int& income)
{
	const int maxIterCount = 100 * instance.followerFacilityCount;
	//ivector followerPrices = GetFirst(instance, false);]
	ivector followerPrices = first;
	int maxIncome = SolveLower(leaderPrices, followerPrices, instance);

	int iterCount = 0;

	while (true)
	{
		++iterCount;
		ivector followerRecordPrices = followerPrices;
		int incomeRecord = maxIncome;

		for (int i = 0; i < maxIterCount; ++i)
		{
			ivector tmpFollowerPrices = GetRandomFromFlip(leaderPrices, instance, false);
			int income = SolveLower(leaderPrices, tmpFollowerPrices, instance);
			if (income > incomeRecord)
			{
				followerRecordPrices = tmpFollowerPrices;
				incomeRecord = income;
			}
		}

		if (incomeRecord > maxIncome)
		{
			followerPrices = followerRecordPrices;
			maxIncome = incomeRecord;
		}
		else
		{
			break;
		}
	}

	income = maxIncome;
	iterrCount += iterCount;
	return followerPrices;
}

int VndUpperProblem(const Instance& instance, bool exactLower)
{
	const int maxIterCount = 1000 * instance.leaderFacilityCount;

	int followerIterationCount = 0;
	int followerVndCount = 0;
	int followerIncome = 0;

	ivector leaderPrices = GetFirst(instance, true);
	
	FollowerCooperativeExactSolver folllowerSolver(leaderPrices, instance);
	ivector followerPrices = folllowerSolver.prices;

	followerIncome = folllowerSolver.income;
	int maxIncome = Solve(leaderPrices, followerPrices, instance);

	int iterCount = 0;
	while (true) 
	{
		++iterCount;

		FollowerCooperativeExactSolver folllowerSolver(leaderPrices, instance);
		followerPrices = folllowerSolver.prices;

		ivector leaderRecordPrices = leaderPrices;
		ivector followerRecordPrices = followerPrices;
		int incomeRecord = maxIncome;
		int followerIncomeRecord = followerIncome;

		for (int i = 0; i < maxIterCount; ++i)
		{
			ivector tmpLeaderPrices = GetRandomFromFlip(leaderPrices, instance, true);
			int followerIncomeTmp = 0;
			ivector tmpFollowerPrices = VndLowerProblem(followerPrices, tmpLeaderPrices, instance, followerIterationCount, followerIncomeTmp);
			++followerVndCount;

			int income = Solve(tmpLeaderPrices, tmpFollowerPrices, instance);
			if (income > incomeRecord)
			{
				leaderRecordPrices = tmpLeaderPrices;
				followerRecordPrices = tmpFollowerPrices;
				incomeRecord = income;
				followerIncomeRecord = followerIncomeTmp;
			}
		}

		if (incomeRecord > maxIncome)
		{
			leaderPrices = leaderRecordPrices;
			followerPrices = followerRecordPrices;
			maxIncome = incomeRecord;
			followerIncome = followerIncomeRecord;
		}
		else
		{
			break;
		}
	}

	FollowerCooperativeExactSolver followerSolver(leaderPrices, instance);
	followerPrices = followerSolver.prices;
	int result = Solve(leaderPrices, followerPrices, instance);

	std::cout << "Expected follower income: " << followerIncome 
		<< "; Exact solution for lower: " << folllowerSolver.income 
		<< "; Iteration follower average count: " << (float)followerIterationCount / followerVndCount << std::endl;
	std::cout << "Expected leader income: " << maxIncome 
		<< "; Exact leader income: " << result 
		<< "; Iteration count: " << iterCount << std::endl;

	return result;
}

Instance ReadInstance(const std::string& path, float leaderPart, int clip, int clipClients)
{
	std::ifstream file(path);

	std::string input;
	int m = 0;
	int n = 0;
	int r = 0;
	file >> m;
	file >> n;
	file >> r;

	int M = std::min(clip, m);

	int m1 = static_cast<int>(static_cast<float>(M) * leaderPart);
	int m2 = M - m1;
	int n_r = std::min(n, clipClients);
	int f;
	table costsLeader = table(m1, ivector(n_r));
	for (int i = 0; i < m1; ++i)
	{
		for (int j = 0; j < n_r; ++j)
		{
			file >> costsLeader[i][j];
		}
		for (int j = n_r; j < n; ++j)
		{
			file >> f;
		}
	}
	table costsFollower = table(m2, ivector(n_r));
	for (int i = 0; i < m2; ++i)
	{
		for (int j = 0; j < n_r; ++j)
		{
			file >> costsFollower[i][j];
		}
		for (int j = n_r; j < n; ++j)
		{
			file >> f;
		}
	}
	for (int i = 0; i < std::max(m - M, 0); ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			file >> f;
		}
	}
	ivector budgets = ivector(n_r);
	for (int j = 0; j < n_r; ++j)
	{
		file >> budgets[j];
	}

	return Instance(costsLeader, costsFollower, budgets);
}

int main()
{
	std::ofstream out("C:/Workflow/Cpp/millMillPricing/result.txt");
	std::string path = "C:/Workflow/Cpp/millMillPricing/examples";

	std::vector<std::string> testPaths;
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		testPaths.push_back(entry.path().string());
	}

	for (const auto& inputFile : testPaths)
	{

		std::cout << "Test file: " << inputFile << std::endl;

		Instance instance = ReadInstance(inputFile, 0.5f, 10, 30);

		std::chrono::high_resolution_clock timer;
		auto start = timer.now();

		auto ourAnswer = VndUpperProblem(instance, false);

		auto stop = timer.now();

		auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() / 1000000.0f;


		std::cout << "Result: " << ourAnswer << std::endl;
		std::cout << "Time: " << deltaTime << std::endl;

		out << "Test file: " << inputFile << std::endl;
		out << "Result: " << ourAnswer << std::endl;
		out << "Time: " << deltaTime << std::endl;
	}

	return 0;
}