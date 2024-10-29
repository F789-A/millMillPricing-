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
#include "RlsFollowerProblemSolver.h"

class RlsLeaderProblemSolver
{
public:
	int Solve(const ivector& leaderPrice, const ivector& followerPrice, const Instance& instance);

	ivector GetFirst(const Instance& instance, bool upper);
	ivector GetRandomFromFlip(const ivector& startPrice, const Instance& instance);
	ivector GetFromFlip(const ivector& startPrice, int& price, int& facility, const Instance& instance);

	int RlsUpperProblem(const Instance& instance, bool exactLower);
};