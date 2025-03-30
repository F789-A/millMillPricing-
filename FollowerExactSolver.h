#pragma once
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include "Instance.h"
#include <optional>

struct FollowerSolution
{
	int followerIncome;
	ivector followerPrices;
};

class FollowerExactSolver
{
public:
	bool needAll = false;

	FollowerSolution Solve(const ivector& leaderPrices, const Instance& instance,
		const std::optional<ivector>& hint = std::nullopt);

	static constexpr bool debug = false;

	std::vector<int> zOut;
	std::vector<int> xOut;
private:
	SCIP_RETCODE Solve(const ivector& leaderPrices, const Instance& instance, FollowerSolution& output,
		const std::optional<ivector>& hint = std::nullopt);
};