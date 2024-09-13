#include <vector>
#include <random> // TODO mt19937 is to fast?
#include <numeric>
#include <map>
#include <queue>
#include <memory>

#include "GlpkSolvers.h"

struct Node
{
	ivector constraint;
	float upper;
	vector lowerSolution;
	float lower;
	bool leaf;
};

float solve(const vector& leaderPrice, const vector& FollowerPrice, const Instance& instance)
{
	float income = 0;
	for (int j = 0; j < instance.budgets.size(); ++j)
	{
		float tmpIncome = 0;
		float minLeaderCost = std::numeric_limits<float>::max();
		for (int i = 0; i < instance.costsLeader.size(); ++i)
		{
			float tmpMinLeaderCost = instance.costsLeader[i][j] + leaderPrice[i];
			if (instance.budgets[j] - tmpMinLeaderCost >= 0 &&
				minLeaderCost >= tmpMinLeaderCost && 
				tmpIncome < leaderPrice[i])
			{
				tmpIncome = leaderPrice[i];
				minLeaderCost = tmpMinLeaderCost;
			}
		}

		float minFollowerCost = std::numeric_limits<float>::max();
		for (int i = 0; i < instance.costsFollower.size(); ++i)
		{
			float tmpMinFollowerCost = instance.costsLeader[i][j] + FollowerPrice[i];
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

vector getFirstLower(const ivector& constraint, const vector& leaderPrice, const Instance& instance) // TODO not polynominal
{
	FirstVectorSolver firstVectorSolver(constraint, instance);
	if (!firstVectorSolver.exist)
	{
		throw ""; // TODO remove (rewrite this)
	}
	return firstVectorSolver.answer.followerPrices;
}

vector getRandomFromFlipLower(const vector& startPrice, const ivector& constraint, const vector& leaderPrice, const Instance& instance)
{
	std::seed_seq seed_w({ 123123 });
	auto random_generator = std::mt19937(seed_w);
	std::uniform_int_distribution<> distrib1(0, startPrice.size());

	int followerFacility = distrib1(random_generator);

	float pBoundMin = 0.0;
	float pBoundMax = 0.0f;
	for (int j = 0; j < instance.budgets.size(); ++j)
	{
		pBoundMax = std::max(pBoundMax, instance.budgets[j] - instance.costsFollower[followerFacility][j]); // TODO cache this
	}

	for (int j = 0; j < constraint.size(); ++j)
	{
		float minInRow = 0;
		for (int i = 0; i < instance.costsFollower.size(); ++i) // TODO cache this
		{
			if (i == followerFacility || instance.budgets[j] - startPrice[i] - instance.costsFollower[i][j] < 0)
			{
				continue;
			}
			minInRow = std::min(minInRow, instance.budgets[j] - startPrice[i] - instance.costsFollower[i][j]);
		}
		for (int i = 0; i < instance.costsLeader.size(); ++i) // TODO cache this
		{
			if (instance.budgets[j] - startPrice[i] - instance.costsLeader[i][j] < 0)
			{
				continue;
			}
			minInRow = std::min(minInRow, instance.budgets[j] - leaderPrice[i] - instance.costsLeader[i][j]);
		}

		if (constraint[j] == followerFacility) // этот клиент обслуживаетс€ на этом предпри€тии
		{
			pBoundMin = std::max(pBoundMin, std::min(minInRow, instance.budgets[j] - instance.costsFollower[followerFacility][j]));
		}
		else
		{
			pBoundMax = std::min(pBoundMax, std::min(minInRow, instance.budgets[j] - instance.costsFollower[followerFacility][j]));
		}
	}

	std::uniform_real_distribution<> distrib2(pBoundMin, pBoundMax);
	vector result = startPrice;
	result[followerFacility] = distrib2(random_generator);

	return result;
}

LowerAnswer vndLower(const vector& leaderPrices, const ivector& constraint, const Instance& instance)
{
	static constexpr int iterCount = 100;

	vector pricesFollower = getFirstLower(constraint, leaderPrices, instance);
	float maxIncome = solve(leaderPrices, pricesFollower, instance);

	for (int i = 0; i < iterCount; ++i)
	{
		vector tmpPricesFollower = getRandomFromFlipLower(pricesFollower, constraint, leaderPrices, instance);
		float income = solve(leaderPrices, tmpPricesFollower, instance);
		if (income > maxIncome)
		{
			pricesFollower = tmpPricesFollower;
			maxIncome = income;
			i = -1;
		}
	}

	return { pricesFollower, maxIncome };
}

LowerAnswer relaxLower(const vector& leaderPrices, const ivector& constraint, const Instance& instance)
{
	RelaxSolver relax(constraint, instance);
	return relax.answer;
}

LowerAnswer solveLower(const vector& leaderPreces, const Instance& instance)
{
	std::multimap<float, std::shared_ptr<Node>> tree;
	std::queue<std::weak_ptr<Node>> nextNode;
	std::shared_ptr<Node> best;

	nextNode.push(std::make_shared<Node>());

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

		for (int i = 0; i < instance.costsFollower.size() + 1; ++i)
		{
			ivector constraint = node->constraint;
			constraint.push_back(i < instance.costsFollower.size() ? i : std::numeric_limits<int>::max());

			LowerAnswer lowerSolution;
			try
			{
				lowerSolution = vndLower(leaderPreces, constraint, instance);
			}
			catch (...)
			{
				continue;  // TODO fix this
			}
			float upperIncome = relaxLower(leaderPreces, constraint, instance).followerIncome;
			float lowerIncome = lowerSolution.followerIncome;

			if (upperIncome == lowerIncome)
			{
				if (best->upper <= upperIncome)
				{
					best = std::shared_ptr<Node>(new Node{ constraint, upperIncome, lowerSolution.followerPrices, lowerIncome });
				}
			}
			else
			{
				if (upperIncome > maxLowerBound)
				{
					auto newNode = std::shared_ptr<Node>(new Node{constraint, upperIncome, lowerSolution.followerPrices, lowerIncome});
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

vector getFirstUpper(const Instance& instance)
{
	return vector(instance.costsLeader.size(), 10.0f);
}

vector getRandomFromFlipUpper(const vector& startPrice, const Instance& instance)
{
	std::seed_seq seed_w({ 123123 });
	auto random_generator = std::mt19937(seed_w);
	std::uniform_int_distribution<> distrib1(0, startPrice.size());

	int leaderFacility = distrib1(random_generator);

	std::uniform_real_distribution<> distrib2(0.0, instance.pUpperBound[leaderFacility]);

	vector result = startPrice;
	result[leaderFacility] = distrib2(random_generator);

	return result;
}

UpperAnswer vndUpper(const Instance& instance)
{
	static constexpr int iterCount = 100;

	vector pricesLeader = getFirstUpper(instance);
	vector folwerPrice = solveLower(pricesLeader, instance).followerPrices;
	float maxIncome = solve(pricesLeader, folwerPrice, instance);

	for (int i = 0; i < iterCount; ++i)
	{
		vector tmpPricesLeader = getRandomFromFlipUpper(pricesLeader, instance);
		vector tmpfolwerPrice = solveLower(pricesLeader, instance).followerPrices;
		float income = solve(pricesLeader, folwerPrice, instance);
		if (income > maxIncome)
		{
			pricesLeader = tmpPricesLeader;
			folwerPrice = tmpfolwerPrice;
			maxIncome = income;
			i = -1;
		}
	}

	return { pricesLeader, folwerPrice, maxIncome };
}

int main()
{
	Instance instance;

	float result = vndUpper(instance).leaderIncome;

	return 0;
}