#include "FollowerExactSolver.h"

#include <iostream>
#include <iomanip>

#include "ClientProblemSolver.h"

FollowerSolution FollowerExactSolver::Solve(const ivector& leaderPrices, const Instance& instance,
	const std::optional<ivector>& hint)
{
	FollowerSolution output;
	Solve(leaderPrices, instance, output, hint);
	return output;
}

SCIP_RETCODE FollowerExactSolver::Solve(const ivector& leaderPrices, const Instance& instance, FollowerSolution& output,
	const std::optional<ivector>& hint)
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

	ivector reformulatedBudgets = instance.budgets;
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < instance.leaderFacilityCount; ++i)
		{
			if (instance.budgets[j] - (instance.costsLeader[i][j] + leaderPrices[i]) >= 0)
			{
				reformulatedBudgets[j] = std::min(reformulatedBudgets[j], instance.costsLeader[i][j] + leaderPrices[i] - 1);
			}
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

	if (hint.has_value())
	{
		SCIP_SOL* initSol = nullptr;
		SCIP_CALL(SCIPcreateSol(scip, &initSol, nullptr));
		ClientProblemSolver cps2;
		cps2.needAll = true;
		cps2.Solve(leaderPrices, *hint, instance);
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			for (int j = 0; j < instance.clientsCount; ++j)
			{
				if (cps2.xOutB[j] == -1 && cps2.xOut[j] == i)
				{
					SCIP_CALL(SCIPsetSolVal(scip, initSol, z_ij[i][j], (*hint)[i]));
					SCIP_CALL(SCIPsetSolVal(scip, initSol, x_ij[i][j], 1));
				}
				else
				{
					SCIP_CALL(SCIPsetSolVal(scip, initSol, z_ij[i][j], 0.0));
					SCIP_CALL(SCIPsetSolVal(scip, initSol, x_ij[i][j], 0));
				}
			}
		}
		for (int k = 0; k < p_i.size(); ++k)
		{
			SCIP_CALL(SCIPsetSolVal(scip, initSol, p_i[k], (*hint)[k]));
		}
		SCIP_Bool stored = false;
		SCIP_CALL(SCIPaddSolFree(scip, &initSol, &stored));
		assert(stored);
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
	if (needAll)
	{
		zOut.clear();
		zOut.reserve(instance.followerFacilityCount* instance.clientsCount);
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			for (int j = 0; j < instance.clientsCount; ++j)
			{
				zOut.push_back(std::lroundl(SCIPgetSolVal(scip, sol, z_ij[i][j])));
			}
		}
		xOut.clear();
		xOut.reserve(instance.followerFacilityCount* instance.clientsCount);
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			for (int j = 0; j < instance.clientsCount; ++j)
			{
				xOut.push_back(std::lroundl(SCIPgetSolVal(scip, sol, x_ij[i][j])));
			}
		}
	}

	if (debug)
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

	ClientProblemSolver cps;
	cps.Solve(leaderPrices, output.followerPrices, instance);
	assert(cps.followerIncome == output.followerIncome);

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
}