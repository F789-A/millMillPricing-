#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>

#include "Instance.h"

#include "RlsLeaderProblemSolver.h"
#include "HighPointRelaxation.h"

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
	int i = 0;
	std::set<int> ignoreList;
	//ignoreList.insert(0);
	//ignoreList.insert(1);
	for (const auto& inputFile : testPaths)
	{
		if (ignoreList.contains(i))
		{
			i++;
			continue;
		}
		++i;

		std::cout << "Test file: " << inputFile << std::endl;

		Instance instance = ReadInstance(inputFile, 0.5f, 10, 30);

		std::chrono::high_resolution_clock timer;
		auto start = timer.now();


		RlsLeaderProblemSolver solver;

		auto ourAnswer = solver.RlsUpperProblem(instance, false);

		auto stop = timer.now();

		auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() / 1000000.0f;


		//std::cout << "Result: " << ourAnswer << std::endl;
		std::cout << "Time: " << deltaTime << std::endl;

		HighPointRelaxation relax;
		relax.Solve(instance);
		std::cout << "HighPointRelaxation: " << relax.income << std::endl;

		std::cout << "----------------------------------------------" << std::endl;

		//out << "Test file: " << inputFile << std::endl;
		//out << "Result: " << ourAnswer << std::endl;
		//out << "Time: " << deltaTime << std::endl;
	}

	return 0;
}