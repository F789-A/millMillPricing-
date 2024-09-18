#include <vector>
#include <random> // TODO mt19937 is to fast?
#include <numeric>
#include <map>
#include <queue>
#include <memory>
#include <iostream>
#include <filesystem>
#include <fstream>

#include <scip/scip.h>
#include <scip/scipdefplugins.h>

#include "Solvers.h"

struct Node
{
	ivector constraint;
	float upper;
	vector lowerSolution;
	float lower;
	bool leaf;
};

float Solve(const vector& price, const Instance& instance)
{
	float income = 0;
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		float minFollowerCost = std::numeric_limits<float>::max();
		for (int i = 0; i < instance.facilityCount; ++i)
		{
			float tmpCost = instance.costs[i][j] + price[i];
			if (instance.budgets[j] - tmpCost >= 0 && minFollowerCost >= tmpCost)
			{
				minFollowerCost = tmpCost;
			}
		}

		if (minFollowerCost < std::numeric_limits<float>::max())
		{
			income += minFollowerCost;
		}
	}

	return income;
}

vector getFirst(const ivector& constraint, const Instance& instance, bool& oExist) // TODO not polynominal
{
	FirstVectorSolver firstVectorSolver(constraint, instance);
	oExist = firstVectorSolver.exist; // TODO remove (rewrite this)
	return firstVectorSolver.prices;
}

vector getRandomFromFlip(const vector& startPrice, const ivector& constraint, const Instance& instance)
{
	static std::seed_seq seed_w({ 123123 });
	static auto random_generator = std::mt19937(seed_w);
	std::uniform_int_distribution<> distrib1(0, startPrice.size() - 1);

	int followerFacility = distrib1(random_generator);

	float pBoundMin = 0.0;
	float pBoundMax = instance.pUpperBound[followerFacility];

	for (int j = 0; j < constraint.size(); ++j)
	{
		float minInRow = 0;
		for (int i = 0; i < instance.facilityCount; ++i) // TODO cache this
		{
			if (i == followerFacility || instance.budgets[j] - startPrice[i] - instance.costs[i][j] < 0)
			{
				continue;
			}
			minInRow = std::min(minInRow, instance.budgets[j] - startPrice[i] - instance.costs[i][j]);
		}

		if (constraint[j] == followerFacility) // этот клиент обслуживаетс€ на этом предпри€тии
		{
			pBoundMin = std::max(pBoundMin, std::min(minInRow, instance.budgets[j] - instance.costs[followerFacility][j]));
		}
		else
		{
			pBoundMax = std::min(pBoundMax, std::min(minInRow, instance.budgets[j] - instance.costs[followerFacility][j]));
		}
	}

	std::uniform_real_distribution<> distrib2(pBoundMin, pBoundMax);
	vector result = startPrice;
	result[followerFacility] = distrib2(random_generator);

	return result;
}

Answer VndProblem(const ivector& constraint, const Instance& instance, bool& oExist)
{
	const int iterCount = 100 * instance.facilityCount;

	vector prices = getFirst(constraint, instance, oExist);
	if (!oExist)
	{
		return {};
	}
	float maxIncome = Solve(prices, instance);

	for (int i = 0; i < iterCount; ++i)
	{
		vector tmpPrices = getRandomFromFlip(prices, constraint, instance);
		float income = Solve(tmpPrices, instance);
		if (income > maxIncome)
		{
			prices = tmpPrices;
			maxIncome = income;
			i = -1;
		}
	}

	return { prices, maxIncome };
}

Answer RelaxProblem(const ivector& constraint, const Instance& instance)
{
	ProblemSolver relax(constraint, instance, true);
	return relax.answer;
}

Answer SolveBaB(const Instance& instance)
{
	std::multimap<float, std::shared_ptr<Node>> tree;
	std::queue<std::weak_ptr<Node>> nextNode;
	std::shared_ptr<Node> best;

	std::shared_ptr<Node> stNode = std::make_shared<Node>(); // TODO remove;

	nextNode.push(stNode);

	float maxLowerBound = std::numeric_limits<float>::min();

	while (!nextNode.empty())
	{
		auto wNode = nextNode.front();
		nextNode.pop();
		if (wNode.expired())
		{
			continue;
		}
		auto node = wNode.lock();

		for (int i = 0; i < instance.facilityCount + 1; ++i)
		{
			ivector constraint = node->constraint;
			constraint.push_back(i < instance.facilityCount ? i : std::numeric_limits<int>::max());

			bool exist = true; // TODO rewrite
			Answer solution = VndProblem(constraint, instance, exist);
			if (!exist)
			{
				continue;  
			}

			float upperIncome = RelaxProblem(constraint, instance).income;
			float lowerIncome = solution.income;

			if (upperIncome == lowerIncome)
			{
				if (best->upper <= upperIncome)
				{
					best = std::shared_ptr<Node>(new Node{ constraint, upperIncome, solution.prices, lowerIncome });
				}
			}
			else
			{
				if (upperIncome > maxLowerBound)
				{
					auto newNode = std::shared_ptr<Node>(new Node{constraint, upperIncome, solution.prices, lowerIncome});
					tree.insert({ upperIncome, newNode });
					nextNode.push(newNode);
				}
			}

			maxLowerBound = std::max(maxLowerBound, lowerIncome);

			auto lbIt = tree.lower_bound(lowerIncome);
			auto it = tree.begin();
			while (it != lbIt)
			{
				it = tree.erase(it);
			}
		}
	}

	return { best->lowerSolution, best->lower };
}

Instance ReadInstance(const std::string& path)
{
	std::ifstream file(path);

	std::string input;
	int m = 0;
	int n = 0;
	int r = 0;
	file >> m;
	file >> n;
	file >> r;

	table costs = table(m, vector(n));
	for (int i = 0; i < m; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			file >> costs[i][j];
		}
	}

	vector budgets = vector(n);
	for (int j = 0; j < n; ++j)
	{
		file >> budgets[j];
	}

	return Instance(costs, budgets);
}

int main()
{
	std::string path = "C:/Workflow/Cpp/millMillPricing/examples";
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		Instance instance = ReadInstance( entry.path().string());

		//ProblemSolver solver({}, instance, false);
		//auto solverAnswer = solver.answer;

		auto ourAnswer = SolveBaB(instance);

		std::cout << entry.path() << std::endl;
		//std::cout << solverAnswer.income << " " << ourAnswer.income << std::endl;
	}

	return 0;
}