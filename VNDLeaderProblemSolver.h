#pragma once
#include <vector>
#include <random>
#include <numeric>
#include <map>
#include <queue>
#include <memory>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <set>

#include "FollowerExactSolver.h"
#include "FollowerCooperativeExactSolver.h"
#include "VNDFollowerProblemSolver.h"
#include "ClientProblemSolver.h"
#include "Subset.h"

class VNDLeaderProblemSolver
{
public:
	ivector GetFirst(const Instance& instance, bool upper);
	ivector GetRandomFromFlip(const ivector& startPrice, const Instance& instance);
	ivector GetFromFlip(const ivector& startPrice, std::vector<int>& priceIter,
		SubsetIterator& facilityIter, const Instance& instance);

	int VNDUpperProblem(int FlipCount, const Instance& instance);
	int LSUpperProblemExactLower(const Instance& instance);

	void PrintClientInfo(const ivector& leaderPrices, const ivector& followerPrices, const std::vector<std::pair<int, bool>>& debInfo);

	ClientProblemSolver clientProblemSolver;
};