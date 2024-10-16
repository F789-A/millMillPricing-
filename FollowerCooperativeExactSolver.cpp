#include "FollowerCooperativeExactSolver.h"
#include "FollowerExactSolver.h"


SCIP_RETCODE FollowerCooperativeExactSolver::SolveProblem(const ivector& leaderPrices, const Instance& instance)
{
	auto toIdx = [&instance](int i, int j)
	{
		return i * instance.clientsCount + j;
	};

	FollowerExactSolver solver(leaderPrices, instance);
	int targetIncome = solver.income;

	int facilityCount = instance.leaderFacilityCount + instance.followerFacilityCount;
	table costs;
	for (int i = 0; i < instance.leaderFacilityCount; ++i)
	{
		costs.push_back(instance.costsLeader[i]);
	}
	for (int i = 0; i < instance.followerFacilityCount; ++i)
	{
		costs.push_back(instance.costsFollower[i]);
	}

	SCIP* scip = nullptr;
	SCIP_CALL(SCIPcreate(&scip));
	SCIP_CALL(SCIPincludeDefaultPlugins(scip)); //  include default plugins
	SCIPsetMessagehdlrQuiet(scip, true);

	SCIP_CALL(SCIPcreateProbBasic(scip, "lowerCooperative"));
	SCIP_CALL(SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE));

	std::vector<SCIP_VAR*> z_ij(facilityCount * instance.clientsCount, nullptr);
	for (int i = 0; i < facilityCount * instance.clientsCount; ++i)
	{
		SCIP_CALL(SCIPcreateVarBasic(scip, &z_ij[i], "", 0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_IMPLINT));
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
		SCIP_CALL(SCIPcreateVarBasic(scip, &p, "", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_IMPLINT));
		SCIP_CALL(SCIPaddVar(scip, p));
	}

	SCIP_CONS* mainConstr;
	SCIP_CALL(SCIPcreateConsBasicLinear(scip, &mainConstr, "", 0, nullptr, nullptr, targetIncome, targetIncome));
	for (int i = instance.followerFacilityCount * instance.clientsCount; 
		i < facilityCount * instance.clientsCount; ++i)
	{
		SCIP_CALL(SCIPaddCoefLinear(scip, mainConstr, z_ij[i], 1));
	}

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
		for (int k = instance.leaderFacilityCount; k < facilityCount; ++k)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr2[toIdx(k, j)], "", 0, nullptr, nullptr, -costs[k][j], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr2[toIdx(k, j)], p_i[k - instance.leaderFacilityCount], 1.0));
			for (int i = 0; i < instance.followerFacilityCount; ++i)
			{
				SCIP_CALL(SCIPaddCoefLinear(scip, constr2[toIdx(k, j)], x_ij[toIdx(i, j)], -costs[i][j]));
				SCIP_CALL(SCIPaddCoefLinear(scip, constr2[toIdx(k, j)], z_ij[toIdx(i, j)], -1.0));
			}
			SCIP_CALL(SCIPaddCons(scip, constr2[toIdx(k, j)]));
		}
		for (int k = 0; k < instance.leaderFacilityCount; ++k)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr2[toIdx(k, j)], "", 0, nullptr, nullptr, -costs[k][j] - leaderPrices[k], SCIPinfinity(scip)));
			for (int i = 0; i < instance.followerFacilityCount; ++i)
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
		int offset = instance.leaderFacilityCount;
		for (int i = instance.leaderFacilityCount; i < facilityCount; ++i)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr3[toIdx(i, j)], "", 0, nullptr, nullptr, -instance.qUpperBound[i- offset], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], p_i[i- offset], 1.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], x_ij[toIdx(i, j)], -instance.qUpperBound[i- offset]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], z_ij[toIdx(i, j)], -1.0));
			SCIP_CALL(SCIPaddCons(scip, constr3[toIdx(i, j)]));

			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr4[toIdx(i, j)], "", 0, nullptr, nullptr, -instance.qUpperBound[i- offset], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], p_i[i- offset], -1.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], x_ij[toIdx(i, j)], -instance.qUpperBound[i- offset]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], z_ij[toIdx(i, j)], 1.0));
			SCIP_CALL(SCIPaddCons(scip, constr4[toIdx(i, j)]));
		}


		for (int i = 0; i < instance.leaderFacilityCount; ++i)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr3[toIdx(i, j)], "", 0, nullptr, nullptr, -instance.pUpperBound[i] - leaderPrices[i], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], x_ij[toIdx(i, j)], -instance.pUpperBound[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], z_ij[toIdx(i, j)], -1.0));
			SCIP_CALL(SCIPaddCons(scip, constr3[toIdx(i, j)]));

			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr4[toIdx(i, j)], "", 0, nullptr, nullptr, -instance.pUpperBound[i] + leaderPrices[i], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], x_ij[toIdx(i, j)], -instance.pUpperBound[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], z_ij[toIdx(i, j)], 1.0));
			SCIP_CALL(SCIPaddCons(scip, constr4[toIdx(i, j)]));
		}
	}

	std::vector<SCIP_CONS*> constr5(instance.clientsCount * facilityCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		int offset = instance.leaderFacilityCount;
		for (int i = instance.leaderFacilityCount; i < facilityCount; ++i)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr5[toIdx(i, j)], "", 0, nullptr, nullptr, -SCIPinfinity(scip), 0.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr5[toIdx(i, j)], x_ij[toIdx(i, j)], -instance.qUpperBound[i- offset]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr5[toIdx(i, j)], z_ij[toIdx(i, j)], 1.0));
			SCIP_CALL(SCIPaddCons(scip, constr5[toIdx(i, j)]));
		}

		for (int i = 0; i < instance.leaderFacilityCount; ++i)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr5[toIdx(i, j)], "", 0, nullptr, nullptr, -SCIPinfinity(scip), 0.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr5[toIdx(i, j)], x_ij[toIdx(i, j)], -instance.pUpperBound[i]));
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

	SCIP_CALL(SCIPsolve(scip));
	SCIP_SOL* sol = nullptr;
	sol = SCIPgetBestSol(scip);

	//puck data
	income = SCIPgetSolOrigObj(scip, sol);
	prices.resize(instance.followerFacilityCount);
	for (int i = 0; i < instance.followerFacilityCount; ++i)
	{
		prices[i] = SCIPgetSolVal(scip, sol, p_i[i]);
	}

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

FollowerCooperativeExactSolver::FollowerCooperativeExactSolver(const ivector& leaderPrices, const Instance& instance)
{
	SolveProblem(leaderPrices, instance);
}