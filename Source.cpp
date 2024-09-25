#include <vector>
#include <random>
#include <numeric>
#include <map>
#include <queue>
#include <memory>
#include <iostream>
#include <filesystem>
#include <fstream>

#include "Solvers.h"
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
			if (instance.budgets[j] - tmpMinLeaderCost >= 0 &&
				minLeaderCost >= tmpMinLeaderCost &&
				tmpIncome < leaderPrice[i])
			{
				tmpIncome = leaderPrice[i];
				minLeaderCost = tmpMinLeaderCost;
			}
		}
		int minFollowerCost = std::numeric_limits<int>::max();
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			int tmpMinFollowerCost = instance.costsLeader[i][j] + followerPrice[i];
			if (instance.budgets[j] - tmpMinFollowerCost >= 0 &&
				minFollowerCost >= tmpMinFollowerCost)
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

ivector VndLowerProblem(const ivector& leaderPrices, const Instance& instance)
{
	const int maxIterCount = 10 * instance.followerFacilityCount;
	ivector followerPrices = GetFirst(instance, false);
	int maxIncome = Solve(leaderPrices, followerPrices, instance);

	while (true)
	{
		ivector followerRecordPrices = followerPrices;
		int incomeRecord = maxIncome;

		for (int i = 0; i < maxIterCount; ++i)
		{
			ivector tmpFollowerPrices = GetRandomFromFlip(leaderPrices, instance, false);
			int income = Solve(leaderPrices, tmpFollowerPrices, instance);
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

	return followerPrices;
}

int VndUpperProblem(const Instance& instance)
{
	const int maxIterCount = 10 * instance.leaderFacilityCount;
	ivector leaderPrices = GetFirst(instance, true);
	ivector followerPrices = VndLowerProblem(leaderPrices, instance);
	int maxIncome = Solve(leaderPrices, followerPrices, instance);

	int iterCount = 0;

	while (true) 
	{
		++iterCount;
		ivector leaderRecordPrices = leaderPrices;
		ivector followerRecordPrices = followerPrices;
		int incomeRecord = maxIncome;

		for (int i = 0; i < maxIterCount; ++i)
		{
			ivector tmpLeaderPrices = GetRandomFromFlip(leaderPrices, instance, true);
			ivector tmpFollowerPrices = VndLowerProblem(tmpLeaderPrices, instance);
			int income = Solve(tmpLeaderPrices, tmpFollowerPrices, instance);
			if (income > incomeRecord)
			{
				leaderRecordPrices = tmpLeaderPrices;
				followerRecordPrices = tmpFollowerPrices;
				incomeRecord = income;
			}
		}

		if (incomeRecord > maxIncome)
		{
			leaderPrices = leaderRecordPrices;
			followerPrices = followerRecordPrices;
			maxIncome = incomeRecord;
			std::cout << iterCount <<") Improvement: " << maxIncome << std::endl;
		}
		else
		{
			break;
		}
	}

	FollowerProblemSolver folllowerSolver(leaderPrices, instance);

	return Solve(leaderPrices, folllowerSolver.prices, instance);
}

Instance ReadInstance(const std::string& path, float leaderPart, int clip)
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

	table costsLeader = table(m1, ivector(n));
	for (int i = 0; i < m1; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			file >> costsLeader[i][j];
		}
	}
	table costsFollower = table(m2, ivector(n));
	for (int i = 0; i < m2; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			file >> costsFollower[i][j];
		}
	}
	int f;
	for (int i = 0; i < std::max(m - M, 0); ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			file >> f;
		}
	}
	ivector budgets = ivector(n);
	for (int j = 0; j < n; ++j)
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

		Instance instance = ReadInstance(inputFile, 0.5f, 10);

		std::chrono::high_resolution_clock timer;
		auto start = timer.now();

		auto ourAnswer = VndUpperProblem(instance);

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