#include "HighPointRelaxation.h"

#include <iostream>
#include <iomanip>

#include "ClientProblemSolver.h"

SCIP_RETCODE HighPointRelaxation::Solve(const Instance& instance)
{
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
	ivector upperBounds;
	for (int i = 0; i < instance.leaderFacilityCount; ++i)
	{
		upperBounds.push_back(instance.pUpperBound[i]);
	}
	for (int i = 0; i < instance.followerFacilityCount; ++i)
	{
		upperBounds.push_back(instance.qUpperBound[i]);
	}

	SCIP* scip = nullptr;
	SCIP_CALL(SCIPcreate(&scip));
	SCIP_CALL(SCIPincludeDefaultPlugins(scip)); //  include default plugins
	SCIPsetMessagehdlrQuiet(scip, true);

	SCIP_CALL(SCIPcreateProbBasic(scip, "lower"));
	SCIP_CALL(SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE));

	std::vector<std::vector<SCIP_VAR*>> z_ij(facilityCount, std::vector<SCIP_VAR*>(instance.clientsCount, nullptr));
	for (int i = 0; i < instance.leaderFacilityCount; ++i)
	{
		for (auto& z : z_ij[i])
		{
			SCIP_CALL(SCIPcreateVarBasic(scip, &z, "", 0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_INTEGER));
			SCIP_CALL(SCIPaddVar(scip, z));
		}
	}
	for (int i = instance.leaderFacilityCount; i < facilityCount; ++i)
	{
		for (auto& z : z_ij[i])
		{
			SCIP_CALL(SCIPcreateVarBasic(scip, &z, "", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_INTEGER));
			SCIP_CALL(SCIPaddVar(scip, z));
		}
	}
	std::vector<std::vector<SCIP_VAR*>> x_ij(facilityCount, std::vector<SCIP_VAR*>(instance.clientsCount, nullptr));
	for (auto& x_i : x_ij)
	{
		for (auto& x : x_i)
		{
			SCIP_CALL(SCIPcreateVarBasic(scip, &x, "", 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY));
			SCIP_CALL(SCIPaddVar(scip, x));
		}
	}
	std::vector<SCIP_VAR*> p_i(facilityCount, nullptr);
	for (auto& p : p_i)
	{
		SCIP_CALL(SCIPcreateVarBasic(scip, &p, "", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_INTEGER));
		SCIP_CALL(SCIPaddVar(scip, p));
	}

	std::vector<SCIP_CONS*> constr1(instance.clientsCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr1[j], "", 0, nullptr, nullptr, 0.0, SCIPinfinity(scip)));
		for (int i = 0; i < facilityCount; ++i)
		{
			SCIP_CALL(SCIPaddCoefLinear(scip, constr1[j], x_ij[i][j], instance.budgets[j] - costs[i][j]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr1[j], z_ij[i][j], -1.0));
		}
		SCIP_CALL(SCIPaddCons(scip, constr1[j]));
	}

	std::vector<std::vector<SCIP_CONS*>> constr2(facilityCount, std::vector<SCIP_CONS*>(instance.clientsCount, nullptr));
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int k = 0; k < facilityCount; ++k)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr2[k][j], "", 0, nullptr, nullptr, -costs[k][j], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr2[k][j], p_i[k], 1.0));
			for (int i = 0; i < facilityCount; ++i)
			{
				SCIP_CALL(SCIPaddCoefLinear(scip, constr2[k][j], x_ij[i][j], -costs[i][j]));
				SCIP_CALL(SCIPaddCoefLinear(scip, constr2[k][j], z_ij[i][j], -1.0));
			}
			SCIP_CALL(SCIPaddCons(scip, constr2[k][j]));
		}
	}

	std::vector<std::vector<SCIP_CONS*>> constr3(facilityCount, std::vector<SCIP_CONS*>(instance.clientsCount, nullptr));
	std::vector<std::vector<SCIP_CONS*>> constr4(facilityCount, std::vector<SCIP_CONS*>(instance.clientsCount, nullptr));
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < facilityCount; ++i)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr3[i][j], "", 0, nullptr, nullptr, -upperBounds[i], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[i][j], p_i[i], 1.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[i][j], x_ij[i][j], -upperBounds[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[i][j], z_ij[i][j], -1.0));
			SCIP_CALL(SCIPaddCons(scip, constr3[i][j]));

			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr4[i][j], "", 0, nullptr, nullptr, -upperBounds[i], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[i][j], p_i[i], -1.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[i][j], x_ij[i][j], -upperBounds[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[i][j], z_ij[i][j], 1.0));
			SCIP_CALL(SCIPaddCons(scip, constr4[i][j]));
		}
	}

	std::vector<std::vector<SCIP_CONS*>> constr5(facilityCount, std::vector<SCIP_CONS*>(instance.clientsCount, nullptr));
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < facilityCount; ++i)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr5[i][j], "", 0, nullptr, nullptr, -SCIPinfinity(scip), 0.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr5[i][j], x_ij[i][j], -upperBounds[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr5[i][j], z_ij[i][j], 1.0));
			SCIP_CALL(SCIPaddCons(scip, constr5[i][j]));
		}
	}

	std::vector<SCIP_CONS*> constr6(instance.clientsCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr6[j], "", 0, nullptr, nullptr, -SCIPinfinity(scip), 1.0));
		for (int i = 0; i < facilityCount; ++i)
		{
			SCIP_CALL(SCIPaddCoefLinear(scip, constr6[j], x_ij[i][j], 1.0));
		}
		SCIP_CALL(SCIPaddCons(scip, constr6[j]));
	}

	SCIP_CALL(SCIPsolve(scip));
	SCIP_SOL* sol = SCIPgetBestSol(scip);

	//puck data
	income = std::round(SCIPgetSolOrigObj(scip, sol));
	prices.resize(facilityCount);
	for (int i = 0; i < facilityCount; ++i)
	{
		prices[i] = std::round(SCIPgetSolVal(scip, sol, p_i[i]));
	}

	if (debug)
	{
		std::cout << "--------------------------------------------------" << std::endl;
		for (int i = 0; i < facilityCount; ++i)
		{
			for (int j = 0; j < instance.clientsCount; ++j)
			{
				double z = std::round(SCIPgetSolVal(scip, sol, z_ij[i][j]));
				std::cout << std::setw(3) << z;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
		for (int i = 0; i < facilityCount; ++i)
		{
			for (int j = 0; j < instance.clientsCount; ++j)
			{
				double x = std::round(SCIPgetSolVal(scip, sol, x_ij[i][j]));
				std::cout << std::setw(3) << x;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
		for (int i = 0; i < facilityCount; ++i)
		{
			int p = std::round(SCIPgetSolVal(scip, sol, p_i[i]));
			std::cout << std::setw(3) << p;
		}
		std::cout << std::endl;
		std::cout << "--------------------------------------------------" << std::endl;
	}

	for (auto& l : constr1)
	{
		SCIP_CALL(SCIPreleaseCons(scip, &l));
	}
	for (auto& l : constr2)
	{
		for (auto& ll : l)
		{
			SCIP_CALL(SCIPreleaseCons(scip, &ll));
		}
	}
	for (auto& l : constr3)
	{
		for (auto& ll : l)
		{
			SCIP_CALL(SCIPreleaseCons(scip, &ll));
		}
	}
	for (auto& l : constr4)
	{
		for (auto& ll : l)
		{
			SCIP_CALL(SCIPreleaseCons(scip, &ll));
		}
	}
	for (auto& l : constr5)
	{
		for (auto& ll : l)
		{
			SCIP_CALL(SCIPreleaseCons(scip, &ll));
		}
	}
	for (auto& l : constr6)
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