#include "UpperBoundDM.h"
#include <algorithm>
#include "FollowerCooperativeExactSolver.h"
#include <iostream>
#include <chrono>

#include "Subset.h"

#include "VNDLeaderProblemSolver.h"

int UpperBoundDM::Solve(const Instance& instance)
{
	ivector leaderPricesByClient(instance.leaderFacilityCount, 0);
	FollowerCooperativeExactOutput followerSolution;
	int leaderIncome = 0;

	SubsetIterator facilityIterator(instance.leaderFacilityCount, instance.leaderFacilityCount);
	std::vector<int> facilityPrices(instance.leaderFacilityCount, 0);
	while (!facilityIterator.End())
	{
		//ivector tmpLeaderPricesByClient = GetFromFlip(leaderPrices, facilityPrices, facilityIterator, instance);

		//followerSolution = SolveFollowerProblem(tmpLeaderPrices, instance);
		//clientProblemSolver.Solve(tmpLeaderPrices, followerSolution.followerPrices, instance);
		//if (clientProblemSolver.leaderIncome > leaderIncome)
		//{
		//	leaderIncome = clientProblemSolver.leaderIncome;
		//}
	}

	return leaderIncome;
}

/*SCIP_RETCODE LowerBoundDM::SolveFollowerCooperative(const ivector& leaderPrices, const Instance& instance, FollowerCooperativeExactOutput& output)
{
	auto toIdx = [&instance](int i, int j)
	{
		return i * instance.clientsCount + j;
	};

	int facilityCount = instance.leaderFacilityCount + instance.followerFacilityCount;
	table costs;
	costs.reserve(facilityCount);
	for (int i = 0; i < instance.leaderFacilityCount; ++i)
	{
		costs.push_back(instance.costsLeader[i]);
	}
	for (int i = 0; i < instance.followerFacilityCount; ++i)
	{
		costs.push_back(instance.costsFollower[i]);
	}
	ivector upperBounds;
	upperBounds.reserve(facilityCount);
	for (int i = 0; i < instance.leaderFacilityCount; ++i)
	{
		upperBounds.push_back(instance.pUpperBound[i]);
	}
	for (int i = 0; i < instance.followerFacilityCount; ++i)
	{
		upperBounds.push_back(instance.qUpperBound[i]);
	}

	FollowerCooperativeExactOutput rowSol;
	SolveFollower(leaderPrices, rowSol);
	int targetIncome = rowSol.followerIncome;

	SCIP* scip = nullptr;
	SCIP_CALL(SCIPcreate(&scip));
	SCIP_CALL(SCIPincludeDefaultPlugins(scip));
	SCIPsetMessagehdlrQuiet(scip, true);
	SCIP_CALL(SCIPcreateProbBasic(scip, "lowerCooperative"));
	SCIP_CALL(SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE));

	std::vector<SCIP_VAR*> z_ij(facilityCount * instance.clientsCount, nullptr);
	for (int i = 0; i < instance.leaderFacilityCount * instance.clientsCount; ++i)
	{
		SCIP_CALL(SCIPcreateVarBasic(scip, &z_ij[i], "", 0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_INTEGER));
		SCIP_CALL(SCIPaddVar(scip, z_ij[i]));
	}
	for (int i = instance.leaderFacilityCount * instance.clientsCount; i < facilityCount * instance.clientsCount; ++i)
	{
		SCIP_CALL(SCIPcreateVarBasic(scip, &z_ij[i], "", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_INTEGER));
		SCIP_CALL(SCIPaddVar(scip, z_ij[i]));
	}
	std::vector<SCIP_VAR*> x_ij(facilityCount * instance.clientsCount, nullptr);
	for (auto& x : x_ij)
	{
		SCIP_CALL(SCIPcreateVarBasic(scip, &x, "", 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY));
		SCIP_CALL(SCIPaddVar(scip, x));
	}
	std::vector<SCIP_VAR*> p_i(instance.followerFacilityCount, nullptr);
	for (auto& p : p_i)
	{
		SCIP_CALL(SCIPcreateVarBasic(scip, &p, "", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_INTEGER));
		SCIP_CALL(SCIPaddVar(scip, p));
	}

	SCIP_CONS* mainConstr;
	SCIP_CALL(SCIPcreateConsBasicLinear(scip, &mainConstr, "", 0, nullptr, nullptr, targetIncome, targetIncome));
	for (int i = instance.leaderFacilityCount * instance.clientsCount; i < facilityCount * instance.clientsCount; ++i)
	{
		SCIP_CALL(SCIPaddCoefLinear(scip, mainConstr, z_ij[i], 1.0));
	}
	SCIP_CALL(SCIPaddCons(scip, mainConstr));

	std::vector<SCIP_CONS*> constr1(instance.clientsCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr1[j], "", 0, nullptr, nullptr, 0.0, SCIPinfinity(scip)));
		for (int i = 0; i < facilityCount; ++i)
		{
			SCIP_CALL(SCIPaddCoefLinear(scip, constr1[j], x_ij[toIdx(i, j)], instance.budgets[j] - costs[i][j]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr1[j], z_ij[toIdx(i, j)], -1.0));
		}
		SCIP_CALL(SCIPaddCons(scip, constr1[j]));
	}

	std::vector<SCIP_CONS*> constr2(instance.clientsCount * facilityCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int k = 0; k < instance.leaderFacilityCount; ++k)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr2[toIdx(k, j)], "", 0, nullptr, nullptr, -costs[k][j] - leaderPrices[k], SCIPinfinity(scip)));
			for (int i = 0; i < facilityCount; ++i)
			{
				SCIP_CALL(SCIPaddCoefLinear(scip, constr2[toIdx(k, j)], x_ij[toIdx(i, j)], -costs[i][j]));
				SCIP_CALL(SCIPaddCoefLinear(scip, constr2[toIdx(k, j)], z_ij[toIdx(i, j)], -1.0));
			}
			SCIP_CALL(SCIPaddCons(scip, constr2[toIdx(k, j)]));
		}
		for (int k = instance.leaderFacilityCount; k < facilityCount; ++k)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr2[toIdx(k, j)], "", 0, nullptr, nullptr, -costs[k][j], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr2[toIdx(k, j)], p_i[k - instance.leaderFacilityCount], 1.0));
			for (int i = 0; i < facilityCount; ++i)
			{
				SCIP_CALL(SCIPaddCoefLinear(scip, constr2[toIdx(k, j)], x_ij[toIdx(i, j)], -costs[i][j]));
				SCIP_CALL(SCIPaddCoefLinear(scip, constr2[toIdx(k, j)], z_ij[toIdx(i, j)], -1.0));
			}
			SCIP_CALL(SCIPaddCons(scip, constr2[toIdx(k, j)]));
		}
	}

	std::vector<SCIP_CONS*> constr3(instance.clientsCount * facilityCount, nullptr);
	std::vector<SCIP_CONS*> constr4(instance.clientsCount * facilityCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < instance.leaderFacilityCount; ++i)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr3[toIdx(i, j)], "", 0, nullptr, nullptr, -upperBounds[i] - leaderPrices[i], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], x_ij[toIdx(i, j)], -upperBounds[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], z_ij[toIdx(i, j)], -1.0));
			SCIP_CALL(SCIPaddCons(scip, constr3[toIdx(i, j)]));

			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr4[toIdx(i, j)], "", 0, nullptr, nullptr, -upperBounds[i] + leaderPrices[i], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], x_ij[toIdx(i, j)], -upperBounds[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], z_ij[toIdx(i, j)], 1.0));
			SCIP_CALL(SCIPaddCons(scip, constr4[toIdx(i, j)]));
		}


		int offset = instance.leaderFacilityCount;
		for (int i = instance.leaderFacilityCount; i < facilityCount; ++i)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr3[toIdx(i, j)], "", 0, nullptr, nullptr, -upperBounds[i], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], p_i[i - offset], 1.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], x_ij[toIdx(i, j)], -upperBounds[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], z_ij[toIdx(i, j)], -1.0));
			SCIP_CALL(SCIPaddCons(scip, constr3[toIdx(i, j)]));

			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr4[toIdx(i, j)], "", 0, nullptr, nullptr, -upperBounds[i], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], p_i[i - offset], -1.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], x_ij[toIdx(i, j)], -upperBounds[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], z_ij[toIdx(i, j)], 1.0));
			SCIP_CALL(SCIPaddCons(scip, constr4[toIdx(i, j)]));
		}
	}

	std::vector<SCIP_CONS*> constr5(instance.clientsCount * facilityCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < facilityCount; ++i)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr5[toIdx(i, j)], "", 0, nullptr, nullptr, -SCIPinfinity(scip), 0.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr5[toIdx(i, j)], x_ij[toIdx(i, j)], -upperBounds[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr5[toIdx(i, j)], z_ij[toIdx(i, j)], 1.0));
			SCIP_CALL(SCIPaddCons(scip, constr5[toIdx(i, j)]));
		}
	}

	std::vector<SCIP_CONS*> constr6(instance.clientsCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr6[j], "", 0, nullptr, nullptr, -SCIPinfinity(scip), 1.0));
		for (int i = 0; i < facilityCount; ++i)
		{
			SCIP_CALL(SCIPaddCoefLinear(scip, constr6[j], x_ij[toIdx(i, j)], 1.0));
		}
		SCIP_CALL(SCIPaddCons(scip, constr6[j]));
	}

	ivector leaderBestOffer(instance.clientsCount, std::numeric_limits<int>::max());
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < instance.leaderFacilityCount; ++i)
		{
			if (instance.budgets[j] - instance.costsLeader[i][j] - leaderPrices[i] >= 0)
			{
				leaderBestOffer[j] = std::min(leaderBestOffer[j], instance.costsLeader[i][j] + leaderPrices[i]);
			}
		}
	}

	std::vector<SCIP_CONS*> constr7;
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		if (leaderBestOffer[j] == std::numeric_limits<int>::max())
		{
			continue;
		}
		for (int i = instance.leaderFacilityCount; i < facilityCount; ++i)
		{
			SCIP_CONS* constr = nullptr;
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr, "", 0, nullptr, nullptr, -SCIPinfinity(scip), std::max(0, leaderBestOffer[j] - costs[i][j] - 1)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr, z_ij[toIdx(i, j)], 1.0));
			SCIP_CALL(SCIPaddCons(scip, constr));
			constr7.push_back(constr);
		}
	}

	SCIP_SOL* initSol = nullptr;
	SCIP_CALL(SCIPcreateSol(scip, &initSol, nullptr));
	ClientProblemSolver cps2;
	cps2.needAll = true;
	cps2.Solve(leaderPrices, rowSol.followerPrices, instance);
	for (int i = 0; i < instance.leaderFacilityCount; ++i)
	{
		for (int j = 0; j < instance.clientsCount; ++j)
		{
			if (cps2.xOutB[j] == 1 && cps2.xOut[j] == i)
			{
				SCIP_CALL(SCIPsetSolVal(scip, initSol, z_ij[toIdx(i, j)], leaderPrices[i]));
				SCIP_CALL(SCIPsetSolVal(scip, initSol, x_ij[toIdx(i, j)], 1));
			}
			else
			{
				SCIP_CALL(SCIPsetSolVal(scip, initSol, z_ij[toIdx(i, j)], 0));
				SCIP_CALL(SCIPsetSolVal(scip, initSol, x_ij[toIdx(i, j)], 0));
			}
		}
	}
	for (int i = instance.leaderFacilityCount; i < facilityCount; ++i)
	{
		for (int j = 0; j < instance.clientsCount; ++j)
		{
			if (cps2.xOutB[j] == -1 && cps2.xOut[j] == (i - instance.leaderFacilityCount))
			{
				SCIP_CALL(SCIPsetSolVal(scip, initSol, z_ij[toIdx(i, j)], rowSol.followerPrices[i - instance.leaderFacilityCount]));
				SCIP_CALL(SCIPsetSolVal(scip, initSol, x_ij[toIdx(i, j)], 1));
			}
			else
			{
				SCIP_CALL(SCIPsetSolVal(scip, initSol, z_ij[toIdx(i, j)], 0));
				SCIP_CALL(SCIPsetSolVal(scip, initSol, x_ij[toIdx(i, j)], 0));
			}
		}
	}
	for (int k = 0; k < p_i.size(); ++k)
	{
		SCIP_CALL(SCIPsetSolVal(scip, initSol, p_i[k], rowSol.followerPrices[k]));
	}

	//SCIP_Bool feasible;
	//SCIP_CALL(SCIPcheckSol(scip, initSol, TRUE, TRUE, TRUE, TRUE, TRUE, &feasible));
	//std::cout << feasible << std::endl;
	//assert(feasible);

	SCIP_Bool stored = false;
	SCIP_CALL(SCIPaddSolFree(scip, &initSol, &stored));
	assert(stored);

	SCIP_CALL(SCIPsolve(scip));
	SCIP_SOL* sol = nullptr;
	sol = SCIPgetBestSol(scip);

	assert(sol != nullptr);

	//puck data
	output.leaderIncome = std::lround(SCIPgetSolOrigObj(scip, sol));
	output.followerIncome = targetIncome;
	output.followerPrices.resize(instance.followerFacilityCount);
	for (int i = 0; i < instance.followerFacilityCount; ++i)
	{
		output.followerPrices[i] = std::lround(SCIPgetSolVal(scip, sol, p_i[i]));
	}

	if (false)
	{
		std::cout << "--------------------------------------------------" << std::endl;
		for (int i = 0; i < facilityCount; ++i)
		{
			for (int j = 0; j < instance.clientsCount; ++j)
			{
				double z = std::lround(SCIPgetSolVal(scip, sol, z_ij[toIdx(i, j)]));
				std::cout << std::setw(3) << z;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
		for (int i = 0; i < facilityCount; ++i)
		{
			for (int j = 0; j < instance.clientsCount; ++j)
			{
				double x = std::lround(SCIPgetSolVal(scip, sol, x_ij[toIdx(i, j)]));
				std::cout << std::setw(3) << x;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			int p = std::lround(SCIPgetSolVal(scip, sol, p_i[i]));
			std::cout << std::setw(3) << p;
		}
		std::cout << std::endl;
		std::cout << "--------------------------------------------------" << std::endl;
	}

	ClientProblemSolver cps;
	cps.Solve(leaderPrices, output.followerPrices, instance);
	assert(cps.leaderIncome == output.leaderIncome);
	assert(cps.followerIncome == output.followerIncome);


	SCIP_CALL(SCIPreleaseCons(scip, &mainConstr));
	for (auto& l : constr1)
	{
		SCIP_CALL(SCIPreleaseCons(scip, &l));
	}
	for (auto& l : constr2)
	{
		SCIP_CALL(SCIPreleaseCons(scip, &l));
	}
	for (auto& l : constr3)
	{
		SCIP_CALL(SCIPreleaseCons(scip, &l));
	}
	for (auto& l : constr4)
	{
		SCIP_CALL(SCIPreleaseCons(scip, &l));
	}
	for (auto& l : constr5)
	{
		SCIP_CALL(SCIPreleaseCons(scip, &l));
	}
	for (auto& l : constr6)
	{
		SCIP_CALL(SCIPreleaseCons(scip, &l));
	}
	for (auto& l : constr7)
	{
		SCIP_CALL(SCIPreleaseCons(scip, &l));
	}
	for (auto& l : z_ij)
	{
		SCIP_CALL(SCIPreleaseVar(scip, &l));
	}
	for (auto& l : x_ij)
	{
		SCIP_CALL(SCIPreleaseVar(scip, &l));
	}
	for (auto& l : p_i)
	{
		SCIP_CALL(SCIPreleaseVar(scip, &l));
	}
	SCIP_CALL(SCIPfree(&scip));

	return SCIP_OKAY;
}

SCIP_RETCODE LowerBoundUM::SolveFollower(const ivector& leaderPricesByClient, const Instance& instance, FollowerSolution& output)
{
	SCIP* scip = nullptr;
	SCIP_CALL(SCIPcreate(&scip));
	SCIP_CALL(SCIPincludeDefaultPlugins(scip));
	SCIPsetMessagehdlrQuiet(scip, true);
	SCIP_CALL(SCIPcreateProbBasic(scip, "lower"));
	SCIP_CALL(SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE));

	std::vector<SCIP_CONS*> constraints;
	constraints.reserve(2 * instance.clientsCount + 4 * instance.clientsCount * instance.followerFacilityCount);
	std::vector<std::vector<SCIP_VAR*>> z_ij(instance.followerFacilityCount, std::vector<SCIP_VAR*>(instance.clientsCount, nullptr));
	std::vector<std::vector<SCIP_VAR*>> x_ij(instance.followerFacilityCount, std::vector<SCIP_VAR*>(instance.clientsCount, nullptr));
	std::vector<SCIP_VAR*> p_i(instance.followerFacilityCount, nullptr);
	for (auto& z_i : z_ij)
	{
		for (auto& z : z_i)
		{
			SCIP_CALL(SCIPcreateVarBasic(scip, &z, "", 0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_INTEGER));
			SCIP_CALL(SCIPaddVar(scip, z));
		}
	}
	for (auto& x_i : x_ij)
	{
		for (auto& x : x_i)
		{
			SCIP_CALL(SCIPcreateVarBasic(scip, &x, "", 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY));
			SCIP_CALL(SCIPaddVar(scip, x));
		}
	}
	for (auto& p : p_i)
	{
		SCIP_CALL(SCIPcreateVarBasic(scip, &p, "", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_INTEGER));
		SCIP_CALL(SCIPaddVar(scip, p));
	}


	ivector minCostForLeaderByClient(instance.clientsCount, std::numeric_limits<int>::max());
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < instance.leaderFacilityCount; ++i)
		{
			minCostForLeaderByClient[j] = std::min(minCostForLeaderByClient[j], instance.costsLeader[i][j]);
		}
	}
	ivector reformulatedBudgets = instance.budgets;
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		if (instance.budgets[j] - (minCostForLeaderByClient[j] + leaderPricesByClient[j]) >= 0)
		{
			reformulatedBudgets[j] = std::min(reformulatedBudgets[j], minCostForLeaderByClient[j] + leaderPricesByClient[j] - 1);
		}
	}

	for (int j = 0; j < instance.clientsCount; ++j)
	{
		constraints.push_back(nullptr);
		auto& cnstr = constraints.back();
		SCIP_CALL(SCIPcreateConsBasicLinear(scip, &cnstr, "", 0, nullptr, nullptr, 0.0, SCIPinfinity(scip)));
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			SCIP_CALL(SCIPaddCoefLinear(scip, cnstr, x_ij[i][j], reformulatedBudgets[j] - instance.costsFollower[i][j]));
			SCIP_CALL(SCIPaddCoefLinear(scip, cnstr, z_ij[i][j], -1.0));
		}
		SCIP_CALL(SCIPaddCons(scip, cnstr));
	}

	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int k = 0; k < instance.followerFacilityCount; ++k)
		{
			constraints.push_back(nullptr);
			auto& cnstr = constraints.back();
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &cnstr, "", 0, nullptr, nullptr, -instance.costsFollower[k][j], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, cnstr, p_i[k], 1.0));
			for (int i = 0; i < instance.followerFacilityCount; ++i)
			{
				SCIP_CALL(SCIPaddCoefLinear(scip, cnstr, x_ij[i][j], -instance.costsFollower[i][j]));
				SCIP_CALL(SCIPaddCoefLinear(scip, cnstr, z_ij[i][j], -1.0));
			}
			SCIP_CALL(SCIPaddCons(scip, cnstr));
		}
	}

	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			constraints.push_back(nullptr);
			auto& cnstr = constraints.back();
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &cnstr, "", 0, nullptr, nullptr, -instance.qUpperBound[i], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, cnstr, p_i[i], 1.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, cnstr, x_ij[i][j], -instance.qUpperBound[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, cnstr, z_ij[i][j], -1.0));
			SCIP_CALL(SCIPaddCons(scip, cnstr));
		}

		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			constraints.push_back(nullptr);
			auto& cnstr = constraints.back();
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &cnstr, "", 0, nullptr, nullptr, -instance.qUpperBound[i], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, cnstr, p_i[i], -1.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, cnstr, x_ij[i][j], -instance.qUpperBound[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, cnstr, z_ij[i][j], 1.0));
			SCIP_CALL(SCIPaddCons(scip, cnstr));
		}
	}

	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			constraints.push_back(nullptr);
			auto& cnstr = constraints.back();
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &cnstr, "", 0, nullptr, nullptr, -SCIPinfinity(scip), 0.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, cnstr, x_ij[i][j], -instance.qUpperBound[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, cnstr, z_ij[i][j], 1.0));
			SCIP_CALL(SCIPaddCons(scip, cnstr));
		}
	}

	for (int j = 0; j < instance.clientsCount; ++j)
	{
		constraints.push_back(nullptr);
		auto& cnstr = constraints.back();
		SCIP_CALL(SCIPcreateConsBasicLinear(scip, &cnstr, "", 0, nullptr, nullptr, -SCIPinfinity(scip), 1.0));
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			SCIP_CALL(SCIPaddCoefLinear(scip, cnstr, x_ij[i][j], 1.0));
		}
		SCIP_CALL(SCIPaddCons(scip, cnstr));
	}

	SCIP_CALL(SCIPsolve(scip));
	SCIP_SOL* sol = SCIPgetBestSol(scip);

	assert(sol != nullptr);

	//puck data
	output.followerIncome = std::lround(SCIPgetSolOrigObj(scip, sol));
	output.followerPrices.resize(instance.followerFacilityCount);
	for (int i = 0; i < instance.followerFacilityCount; ++i)
	{
		output.followerPrices[i] = std::lroundl(SCIPgetSolVal(scip, sol, p_i[i]));
	}

	if (false)
	{
		std::cout << "--------------------------------------------------" << std::endl;
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			for (int j = 0; j < instance.clientsCount; ++j)
			{
				double z = std::lround(SCIPgetSolVal(scip, sol, z_ij[i][j]));
				std::cout << std::setw(3) << z;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			for (int j = 0; j < instance.clientsCount; ++j)
			{
				double x = std::lround(SCIPgetSolVal(scip, sol, x_ij[i][j]));
				std::cout << std::setw(3) << x;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			int p = std::lround(SCIPgetSolVal(scip, sol, p_i[i]));
			std::cout << std::setw(3) << p;
		}
		std::cout << std::endl;
		std::cout << "--------------------------------------------------" << std::endl;
	}

	for (auto& l : constraints)
	{
		SCIP_CALL(SCIPreleaseCons(scip, &l));
	}
	for (auto& z_i : z_ij)
	{
		for (auto& z : z_i)
		{
			SCIP_CALL(SCIPreleaseVar(scip, &z));
		}
	}
	for (auto& x_i : x_ij)
	{
		for (auto& x : x_i)
		{
			SCIP_CALL(SCIPreleaseVar(scip, &x));
		}
	}
	for (auto& l : p_i)
	{
		SCIP_CALL(SCIPreleaseVar(scip, &l));
	}
	SCIP_CALL(SCIPfree(&scip));

	return SCIP_OKAY;
}*/