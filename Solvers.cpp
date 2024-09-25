#include "Solvers.h"

#include <iostream>


SCIP_RETCODE FollowerProblemSolver::SolveProblem(const ivector& leaderPrices, const Instance& instance)
{
	auto toIdx = [&instance](int i, int j)
	{
		return i * instance.clientsCount + j;
	};

	SCIP* scip = nullptr;
	SCIP_CALL(SCIPcreate(&scip));
	SCIP_CALL(SCIPincludeDefaultPlugins(scip)); //  include default plugins
	SCIPsetMessagehdlrQuiet(scip, true);

	SCIP_CALL(SCIPcreateProbBasic(scip, "lower"));
	SCIP_CALL(SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE));

	std::vector<SCIP_VAR*> z_ij(instance.followerFacilityCount * instance.clientsCount, nullptr);
	for (auto& z : z_ij)
	{
		SCIP_CALL(SCIPcreateVarBasic(scip, &z, "", 0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_INTEGER));
		SCIP_CALL(SCIPaddVar(scip, z));
	}
	std::vector<SCIP_VAR*> x_ij(instance.followerFacilityCount * instance.clientsCount, nullptr);
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
	
	std::vector<SCIP_CONS*> constr1(instance.clientsCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr1[j], "", 0, nullptr, nullptr, 0.0, SCIPinfinity(scip)));
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			SCIP_CALL(SCIPaddCoefLinear(scip, constr1[j], x_ij[toIdx(i, j)], instance.budgets[j] - instance.costsFollower[i][j]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr1[j], z_ij[toIdx(i, j)], -1.0));
		}
		SCIP_CALL(SCIPaddCons(scip, constr1[j]));
	}

	std::vector<SCIP_CONS*> constr2(instance.clientsCount * instance.followerFacilityCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int k = 0; k < instance.followerFacilityCount; ++k)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr2[toIdx(k, j)], "", 0, nullptr, nullptr, -instance.costsFollower[k][j], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr2[toIdx(k, j)], p_i[k], 1.0));
			for (int i = 0; i < instance.followerFacilityCount; ++i)
			{
				SCIP_CALL(SCIPaddCoefLinear(scip, constr2[toIdx(k, j)], x_ij[toIdx(i, j)], -instance.costsFollower[i][j]));
				SCIP_CALL(SCIPaddCoefLinear(scip, constr2[toIdx(k, j)], z_ij[toIdx(i, j)], -1.0));
			}
			SCIP_CALL(SCIPaddCons(scip, constr2[toIdx(k, j)]));
		}
	}

	std::vector<SCIP_CONS*> constr3(instance.clientsCount * instance.followerFacilityCount, nullptr);
	std::vector<SCIP_CONS*> constr4(instance.clientsCount * instance.followerFacilityCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr3[toIdx(i, j)], "", 0, nullptr, nullptr, -instance.qUpperBound[i], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], p_i[i], 1.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], x_ij[toIdx(i, j)], -instance.qUpperBound[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], z_ij[toIdx(i, j)], -1.0));
			SCIP_CALL(SCIPaddCons(scip, constr3[toIdx(i, j)]));

			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr4[toIdx(i, j)], "", 0, nullptr, nullptr, -instance.qUpperBound[i], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], p_i[i], -1.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], x_ij[toIdx(i, j)], -instance.qUpperBound[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], z_ij[toIdx(i, j)], 1.0));
			SCIP_CALL(SCIPaddCons(scip, constr4[toIdx(i, j)]));
		}
	}

	std::vector<SCIP_CONS*> constr5(instance.clientsCount * instance.followerFacilityCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr5[toIdx(i, j)], "", 0, nullptr, nullptr, -SCIPinfinity(scip), 0.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr5[toIdx(i, j)], x_ij[toIdx(i, j)], -instance.qUpperBound[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr5[toIdx(i, j)], z_ij[toIdx(i, j)], 1.0));
			SCIP_CALL(SCIPaddCons(scip, constr5[toIdx(i, j)]));
		}
	}

	std::vector<SCIP_CONS*> constr6(instance.clientsCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr6[j], "", 0, nullptr, nullptr, -SCIPinfinity(scip), 1.0));
		for (int i = 0; i < instance.followerFacilityCount; ++i)
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
				leaderBestOffer[i] = std::min(leaderBestOffer[i], instance.costsLeader[i][j] + leaderPrices[i]);
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
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			constr7.push_back(nullptr);
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr7.back(), "", 0, nullptr, nullptr, -SCIPinfinity(scip), leaderBestOffer[j]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr7.back(), z_ij[toIdx(i, j)], 1.0));
			SCIP_CALL(SCIPaddCons(scip, constr7.back()));
		}
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

FollowerProblemSolver::FollowerProblemSolver(const ivector& leaderPrices, const Instance& instance)
{
	SolveProblem(leaderPrices, instance);
}