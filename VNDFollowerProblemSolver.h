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
	ivector GetFirst(const Instance& instance, bool upper);
	ivector GetRandomFromFlip(const ivector& startPrice, const Instance& instance);
	ivector GetFromFlip(const ivector& startPrice, int& price, int& facility, const Instance& instance);
	ivector GetFromFlip(const ivector& startPrice, std::vector<int>& priceIter,
		SubsetIterator& facilityIter, const Instance& instance);
	ivector RlsLowerProblem(const ivector& first, const ivector& leaderPrices, int FlipCount, const Instance& instance);

	ClientProblemSolver clientProblemSolver;
};