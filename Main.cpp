#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>

#include "Instance.h"

#include "VNDLeaderProblemSolver.h"
#include "HighPointRelaxation.h"
#include "LowerBoundMD.h"
#include "LowerBoundUM.h"
#include "VNDFollowerProblemSolver.h"

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
	for (int i = leaderFacilityCount + followerCount; i < m; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			file >> f;
		}
	}
	table costsFollower = table(followerCount, ivector(clientCount));
	for (int i = 0; i < followerCount; ++i)
	{
		for (int j = 0; j < clientCount; ++j)
		{
			file >> costsFollower[followerCount - 1 - i][j];
		}
		for (int j = clientCount; j < n; ++j)
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
		//"examples/FLPr_100_100_02.txt",
		//"examples/FLPr_100_100_03.txt",
		//"examples/FLPr_100_100_04.txt",
		//"examples/FLPr_100_100_05.txt",
		//"examples/FLPr_100_100_06.txt",
		//"examples/FLPr_100_100_07.txt",
		//"examples/FLPr_100_100_08.txt",
		//"examples/FLPr_100_100_09.txt",
		//"examples/FLPr_100_100_10.txt",
		/*
		"examplesNew/FLPr_100_100_0.txt",
		"examplesNew/FLPr_100_100_1.txt",
		"examplesNew/FLPr_100_100_2.txt",
		"examplesNew/FLPr_100_100_3.txt",
		"examplesNew/FLPr_100_100_4.txt",
		"examplesNew/FLPr_100_100_5.txt",
		"examplesNew/FLPr_100_100_6.txt",
		"examplesNew/FLPr_100_100_7.txt",
		"examplesNew/FLPr_100_100_8.txt",
		"examplesNew/FLPr_100_100_9.txt",
		*/
	};
	std::vector<int> leaderFacCount{ 2, 5, 10, 20};
	std::vector<int> followerFacCount{ 2, 5, 10 };
	std::vector<int> clientCount{ 30 };
	HighPointRelaxation relax;
	LowerBoundMD lowerBoundMD;
	LowerBoundUM lowerBoundUM;
	VNDLeaderProblemSolver solver;
	std::chrono::high_resolution_clock timer;
	std::chrono::milliseconds timeLimit(3600 * 1000);

	for (auto clCount : clientCount)
	{
		for (auto lfc : leaderFacCount)
		{
			for (auto ffc : followerFacCount)
			{
				for (const auto& inputFile : testPaths)
				{
					Instance instance = ReadInstance(inputFile, lfc, ffc, clCount);

					auto output = solver.VNDUpperVNDFirstImproveLower(1, 1, instance, timeLimit);
					std::cout << "alg:VND_1_1 " << clCount << " " << lfc << " " << ffc << " " << inputFile << " " <<
						output.first << " " << output.second.count() / 1000.0f << std::endl;

					//auto start = timer.now();
					//auto res = lowerBoundUM.Solve(instance);
					//auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - start).count() / 1000.0f;
					//std::cout << res << " " << deltaTime << " ";
					//std::cout << "alg:UM " << clCount << " " << lfc << " " << ffc << " " << inputFile << " " <<
					//	res << " " << deltaTime << std::endl;
				}
			}
		}
	}

	/*std::cout << "alg:HPR" << std::endl;
	for (auto clCount : clientCount)
	{
		for (auto lfc : leaderFacCount)
		{
			for (const auto& inputFile : testPaths)
			{
				Instance instance = ReadInstance(inputFile, lfc, 0, clCount);
				auto start = timer.now();
				relax.Solve(instance);
				auto res = relax.income;
				auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - start).count() / 1000.0f;
				for (auto ffc : followerFacCount)
				{
					std::cout << clCount << " " << lfc << " " << ffc << " " << inputFile << " ";
					std::cout << res << " " << deltaTime << " ";
					std::cout << std::endl;
				}
			}
		}
	}*/

	/*std::cout << "alg:UM" << std::endl;
	for (auto clCount : clientCount)
	{
		for (auto lfc : leaderFacCount)
		{
			for (auto ffc : followerFacCount)
			{
				std::cout << clCount << " " << lfc << " " << ffc << std::endl;
				for (const auto& inputFile : testPaths)
				{
					Instance instance = ReadInstance(inputFile, lfc, ffc, clCount);
					auto start = timer.now();
					auto res = lowerBoundUM.Solve(instance);
					auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - start).count() / 1000.0f;
					std::cout << res << " " << deltaTime << " ";
				}
				std::cout << std::endl;
			}
		}
	}*/

	/*std::cout << "alg:MD" << std::endl;
	for (auto clCount : clientCount)
	{
		for (auto lfc : leaderFacCount)
		{
			for (auto ffc : followerFacCount)
			{
				std::cout << clCount << " " << lfc << " " << ffc << std::endl;
				for (const auto& inputFile : testPaths)
				{
					Instance instance = ReadInstance(inputFile, lfc, ffc, clCount);
					auto start = timer.now();
					lowerBoundMD.Solve(instance);
					auto res = lowerBoundMD.income;
					auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - start).count() / 1000.0f;
					std::cout << res << " " << deltaTime << " ";
				}
				std::cout << std::endl;
			}
		}
	}*/

	/*for (const auto& inputFile : testPaths)
	{
		for (auto clCount : clientCount)
		{
			for (auto lfc : leaderFacCount)
			{
				for (auto ffc : followerFacCount)
				{
					Instance instance = ReadInstance(inputFile, lfc, ffc, clCount);

					auto output = solver.VNDUpperVNDFirstImproveLower(1, 1, instance, timeLimit);
					std::cout << "alg:VND_1_1 " << clCount << " " << lfc << " " << ffc << " " << inputFile << " " <<
						output.first << " " << output.second.count() / 1000.0f << std::endl;
				}
			}
		}
	}

	for (const auto& inputFile : testPaths)
	{
		for (auto clCount : clientCount)
		{
			for (auto lfc : leaderFacCount)
			{
				for (auto ffc : followerFacCount)
				{
					Instance instance = ReadInstance(inputFile, lfc, ffc, clCount);

					auto output = solver.VNDUpperVNDFirstImproveLower(1, 2, instance, timeLimit);
					std::cout << "alg:VND_1_2 " << clCount << " " << lfc << " " << ffc << " " << inputFile << " " <<
						output.first << " " << output.second.count() / 1000.0f << std::endl;
				}
			}
		}
	}

	for (const auto& inputFile : testPaths)
	{
		for (auto clCount : clientCount)
		{
			for (auto lfc : leaderFacCount)
			{
				for (auto ffc : followerFacCount)
				{
					Instance instance = ReadInstance(inputFile, lfc, ffc, clCount);

					auto output = solver.VNDUpperVNDFirstImproveLower(2, 1, instance, timeLimit);
					std::cout << "alg:VND_2_1 " << clCount << " " << lfc << " " << ffc << " " << inputFile << " " <<
						output.first << " " << output.second.count() / 1000.0f << std::endl;
				}
			}
		}
	}

	/*std::cout << "alg:VND_EX" << std::endl;
	for (const auto& inputFile : testPaths)
	{
		std::cout << "Test_file: " << inputFile << std::endl;
		for (auto clCount : clientCount)
		{
			for (auto lfc : leaderFacCount)
			{
				for (auto ffc : followerFacCount)
				{
					Instance instance = ReadInstance(inputFile, lfc, ffc, clCount);

					bool ended = false;
					auto start = timer.now();
					auto res = solver.LSUpperProblemExactLower(instance, ended);
					auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - start).count() / 1000.0f;
					std::cout << clCount << " " << lfc << " " << ffc << std::endl;
					if (ended)
						std::cout << res << " " << -deltaTime << std::endl;
					else
						std::cout << res << " " << deltaTime << std::endl;
				}
			}
		}
	}*/

	/*std::cout << "alg:EX" << std::endl;
	for (const auto& inputFile : testPaths)
	{
		std::cout << "Test_file: " << inputFile << std::endl;
		for (auto clCount : clientCount)
		{
			for (auto lfc : leaderFacCount)
			{
				for (auto ffc : followerFacCount)
				{
					Instance instance = ReadInstance(inputFile, lfc, ffc, clCount);

					bool ended = false;
					auto start = timer.now();
					auto res = solver.ExactUpperProblem(instance, ended);
					auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - start).count() / 1000.0f;
					std::cout << clCount << " " << lfc << " " << ffc << std::endl;
					if (ended)
						std::cout << res << " " << -deltaTime << std::endl;
					else
						std::cout << res << " " << deltaTime << std::endl;
				}
			}
		}
	}
	*/
	return 0;
}