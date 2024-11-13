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
#include "VectorCmp.h"
#include "ClientProblemSolver.h"

class RlsFollowerProblemSolver
{
public:
	ivector GetFirst(const Instance& instance, bool upper);
	ivector GetRandomFromFlip(const ivector& startPrice, const Instance& instance);
	ivector GetFromFlip(const ivector& startPrice, int& price, int& facility, const Instance& instance);
	ivector RlsLowerProblem(const ivector& first, const ivector& leaderPrices, const Instance& instance, int& iterrCount);

	ClientProblemSolver clientProblemSolver;
};