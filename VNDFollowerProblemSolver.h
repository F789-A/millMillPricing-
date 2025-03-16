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
#include "ClientProblemSolver.h"

#include "Subset.h"

class VNDFollowerProblemSolver
{
public:
	ivector GetFromFlip(const ivector& startPrice, std::vector<int>& priceIter, SubsetIterator& facilityIter, const Instance& instance);
	ivector VNDLowerProblem(const ivector& first, const ivector& leaderPrices, int FlipCount, const Instance& instance);
	ivector SearchLowerProblem(const ivector& leaderPrices, const Instance& instance);

	ClientProblemSolver clientProblemSolver;
};