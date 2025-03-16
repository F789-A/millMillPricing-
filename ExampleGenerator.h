#pragma once
#include <random>
#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>

class ExampleGenerator
{
public:
	static void Generate(int FacilityNumber, int ClientNumber, int Count)
	{
		static std::seed_seq seed_w({ 123123 });
		static auto random_generator = std::mt19937(seed_w);
		std::string fileName = "examplesNew/FLPr_" + std::to_string(FacilityNumber) +"_" + std::to_string(ClientNumber) + "_";

		int budgetUpper = 10;
		int budgetLower = 3;
		int lenX = 4;
		int lenY = 4;

		std::uniform_int_distribution<> distribX(0, lenX); // [0, lenX]
		std::uniform_int_distribution<> distribY(0, lenY);
		std::uniform_int_distribution<> distribBudgets(budgetLower, budgetUpper);


		for (int i = 0; i < Count; ++i)
		{
			std::ofstream file(fileName + std::to_string(i) +".txt");

			file << FacilityNumber << " " << ClientNumber << " 5" << std::endl;

			std::vector<std::pair<int, int>> facilityesCoord;
			for (int j = 0; j < FacilityNumber; ++j)
			{
				int facilityX = distribX(random_generator);
				int facilityY = distribY(random_generator);
				facilityesCoord.push_back({ facilityX, facilityY });
			}

			for (int j = 0; j < ClientNumber; ++j)
			{
				int clientX = distribX(random_generator);
				int clientY = distribY(random_generator);
				for (int k = 0; k < FacilityNumber; ++k)
				{
					int len = std::lroundf(std::sqrtf(std::powf(facilityesCoord[k].first - clientX, 2) + std::powf(facilityesCoord[k].second - clientY, 2)));
					file << len << " ";
				}
				file << std::endl;
			}
			for (int j = 0; j < ClientNumber; ++j)
			{
				int budget = distribBudgets(random_generator);
				file << budget << " ";
			}
		}
	}
};