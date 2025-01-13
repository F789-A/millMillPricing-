#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>

#include "Instance.h"

#include "VNDLeaderProblemSolver.h"
#include "HighPointRelaxation.h"

Instance ReadInstance(const std::string& path, int leaderFacilityCount, int followerCount, int clientCount)
{
	std::ifstream file(path);

	std::string input;
	int m = 0;
	int n = 0;
	int r = 0;
	file >> m;
	file >> n;
	file >> r;

	assert(leaderFacilityCount + followerCount <= m && clientCount <= n);

	int f;
	table costsLeader = table(leaderFacilityCount, ivector(clientCount));
	for (int i = 0; i < leaderFacilityCount; ++i)
	{
		for (int j = 0; j < clientCount; ++j)
		{
			file >> costsLeader[i][j];
		}
		for (int j = clientCount; j < n; ++j)
		{
			file >> f;
		}
	}
	table costsFollower = table(followerCount, ivector(clientCount));
	for (int i = 0; i < followerCount; ++i)
	{
		for (int j = 0; j < clientCount; ++j)
		{
			file >> costsFollower[i][j];
		}
		for (int j = clientCount; j < n; ++j)
		{
			file >> f;
		}
	}
	for (int i = 0; i < m - (leaderFacilityCount + followerCount); ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			file >> f;
		}
	}
	ivector budgets = ivector(clientCount);
	for (int j = 0; j < clientCount; ++j)
	{
		file >> budgets[j];
	}

	return Instance(costsLeader, costsFollower, budgets);
}

int main()
{
	std::vector<std::string> testPaths
	{
		"examples/FLPr_100_100_01.txt",
		"examples/FLPr_100_100_02.txt",
		"examples/FLPr_100_100_03.txt",
		"examples/FLPr_100_100_04.txt",
		"examples/FLPr_100_100_05.txt",
		"examples/FLPr_100_100_06.txt",
		"examples/FLPr_100_100_07.txt",
		"examples/FLPr_100_100_08.txt",
		"examples/FLPr_100_100_09.txt",
		"examples/FLPr_100_100_10.txt",
	};

	//std::string path = "C:/Workflow/Cpp/millMillPricing/examples";
	//for (const auto& entry : std::filesystem::directory_iterator(path))
	//{
		//testPaths.push_back(entry.path().string());
	//}

	for (const auto& inputFile : testPaths)
	{
		std::cout << "Test file: " << inputFile << std::endl;

		Instance instance = ReadInstance(inputFile, 0.5f, 10, 30);

		std::chrono::high_resolution_clock timer;
		auto start = timer.now();

		VNDLeaderProblemSolver solver;

		auto inc = solver.LSUpperProblemExactLower(instance);
		std::cout << "Exact leader income: " << inc << std::endl;

		auto stop = timer.now();
		auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(timer.now() - start).count() / 1000000.0f;

		std::cout << "Time: " << deltaTime << std::endl;

		HighPointRelaxation relax;
		relax.Solve(instance);
		std::cout << "HighPointRelaxation: " << relax.income << std::endl;

		std::cout << "----------------------------------------------" << std::endl;

		break;
	}

	return 0;
}