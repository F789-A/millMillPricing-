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

class FlipIterator
{
public:
	FlipIterator(const ivector& startPrice, const int k, const ivector& upperBound);

	FlipIterator& operator++();
	const std::vector<int>& operator*() const;

	bool End() const;

private:
	const ivector& startPrice;
	const ivector& upperBounds;
	std::vector<int> current;
	SubsetIterator subsetIterator;
};

class VNDFollowerProblemSolver
{
public:
	ivector VNDLowerProblem(const ivector& first, const ivector& leaderPrices, int FlipCount, const Instance& instance);
	ivector VNDLowerProblemFirstImprove(const ivector& first, const ivector& leaderPrices, int FlipCount, const Instance& instance);
	ivector SearchLowerProblem(const ivector& leaderPrices, const Instance& instance);

	ClientProblemSolver clientProblemSolver;
};