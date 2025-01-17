#include "FollowerExactSolver.h"

#include <iostream>
#include <iomanip>

#include "ClientProblemSolver.h"

FollowerSolution FollowerExactSolver::Solve(const ivector& leaderPrices, const Instance& instance)
{
	FollowerSolution output;
	Solve(leaderPrices, instance, output);
	return output;
}

SCIP_RETCODE FollowerExactSolver::Solve(const ivector& leaderPrices, const Instance& instance, FollowerSolution& output)
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
				assert(instance.costsLeader[i][j] + leaderPrices[i] > 0);
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

	SCIP_CALL(SCIPsolve(scip));
	SCIP_SOL* sol = SCIPgetBestSol(scip);

	//puck data
	output.followerIncome = std::lround(SCIPgetSolOrigObj(scip, sol));
	output.followerPrices.resize(instance.followerFacilityCount);
	for (int i = 0; i < instance.followerFacilityCount; ++i)
	{
		output.followerPrices[i] = std::lround(SCIPgetSolVal(scip, sol, p_i[i]));
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