#include "Solvers.h"

#include <scip/scip.h>
#include <scip/scipdefplugins.h>

#include <iostream>

Instance::Instance(const table& Costs, const vector& Budgets)
	: costs(Costs), budgets(Budgets), facilityCount(Costs.size()), clientsCount(Budgets.size())
{
	pUpperBound.resize(facilityCount);
	for (int i = 0; i < facilityCount; ++i)
	{
		pUpperBound[i] = std::numeric_limits<float>::min();
		for (int j = 0; j < clientsCount; ++j)
		{
			pUpperBound[j] = std::max(pUpperBound[j], budgets[j] - costs[i][j]);
		}
	}
}

SCIP_RETCODE ProblemSolver::SolveProblem(const ivector& constraint, const Instance& instance, bool relax)
{
	auto toIdx = [&instance](int i, int j)
	{
		return i * instance.clientsCount + j;
	};

	SCIP_Vartype vartype = relax ? SCIP_VARTYPE_CONTINUOUS : SCIP_VARTYPE_BINARY;

	SCIP* scip = nullptr;
	SCIP_CALL(SCIPcreate(&scip));
	SCIP_CALL(SCIPincludeDefaultPlugins(scip)); //  include default plugins
	SCIPsetMessagehdlrQuiet(scip, true);

	SCIP_CALL(SCIPcreateProbBasic(scip, "relax"));
	SCIP_CALL(SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE));

	std::vector<SCIP_VAR*> z_ij(instance.facilityCount * instance.clientsCount, nullptr);
	for (auto& z : z_ij)
	{
		SCIP_CALL(SCIPcreateVarBasic(scip, &z, "", 0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_CONTINUOUS));
		SCIP_CALL(SCIPaddVar(scip, z));
	}
	std::vector<SCIP_VAR*> x_ij(instance.facilityCount * instance.clientsCount, nullptr);
	for (auto& x : x_ij)
	{
		SCIP_CALL(SCIPcreateVarBasic(scip, &x, "", 0.0, 1.0, 0.0, vartype));
		SCIP_CALL(SCIPaddVar(scip, x));
	}
	std::vector<SCIP_VAR*> p_i(instance.facilityCount, nullptr);
	for (auto& p : p_i)
	{
		SCIP_CALL(SCIPcreateVarBasic(scip, &p, "", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_CONTINUOUS));
		SCIP_CALL(SCIPaddVar(scip, p));
	}
	
	std::vector<SCIP_CONS*> constr1(instance.clientsCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr1[j], "", 0, nullptr, nullptr, 0.0, SCIPinfinity(scip)));
		for (int i = 0; i < instance.facilityCount; ++i)
		{
			SCIP_CALL(SCIPaddCoefLinear(scip, constr1[j], x_ij[toIdx(i, j)], instance.budgets[j] - instance.costs[i][j]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr1[j], z_ij[toIdx(i, j)], -1.0));
		}
		SCIP_CALL(SCIPaddCons(scip, constr1[j]));
	}

	std::vector<SCIP_CONS*> constr2(instance.clientsCount * instance.facilityCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int k = 0; k < instance.clientsCount; ++k)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr2[toIdx(k, j)], "", 0, nullptr, nullptr, -instance.costs[k][j], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr2[toIdx(k, j)], p_i[k], 1.0));
			for (int i = 0; i < instance.facilityCount; ++i)
			{
				SCIP_CALL(SCIPaddCoefLinear(scip, constr2[toIdx(k, j)], x_ij[toIdx(i, j)], instance.costs[i][j]));
				SCIP_CALL(SCIPaddCoefLinear(scip, constr2[toIdx(k, j)], z_ij[toIdx(i, j)], -1.0));
			}
			SCIP_CALL(SCIPaddCons(scip, constr2[toIdx(k, j)]));
		}
	}

	std::vector<SCIP_CONS*> constr3(instance.clientsCount * instance.facilityCount, nullptr);
	std::vector<SCIP_CONS*> constr4(instance.clientsCount * instance.facilityCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < instance.clientsCount; ++i)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr3[toIdx(i, j)], "", 0, nullptr, nullptr, -instance.pUpperBound[i], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], p_i[i], 1.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], x_ij[toIdx(i, j)], -instance.pUpperBound[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr3[toIdx(i, j)], z_ij[toIdx(i, j)], -1.0));
			SCIP_CALL(SCIPaddCons(scip, constr3[toIdx(i, j)]));

			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr4[toIdx(i, j)], "", 0, nullptr, nullptr, -instance.pUpperBound[i], SCIPinfinity(scip)));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], p_i[i], -1.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], x_ij[toIdx(i, j)], -instance.pUpperBound[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr4[toIdx(i, j)], z_ij[toIdx(i, j)], 1.0));
			SCIP_CALL(SCIPaddCons(scip, constr4[toIdx(i, j)]));
		}
	}

	std::vector<SCIP_CONS*> constr5(instance.clientsCount * instance.facilityCount, nullptr);
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < instance.clientsCount; ++i)
		{
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr5[toIdx(i, j)], "", 0, nullptr, nullptr, -SCIPinfinity(scip), 0.0));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr5[toIdx(i, j)], x_ij[toIdx(i, j)], -instance.pUpperBound[i]));
			SCIP_CALL(SCIPaddCoefLinear(scip, constr5[toIdx(i, j)], z_ij[toIdx(i, j)], 1.0));
			SCIP_CALL(SCIPaddCons(scip, constr5[toIdx(i, j)]));
		}
	}

	std::vector<SCIP_CONS*> constr6;
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		if (j < constraint.size())
		{
			for (int i = 0; i < instance.facilityCount; ++i)
			{
				double rb = 0.0;
				if (constraint[j] == i)
				{
					rb = 1.0;
				}
				constr6.push_back(nullptr);
				SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr6.back(), "", 0, nullptr, nullptr, rb, rb));
				SCIP_CALL(SCIPaddCoefLinear(scip, constr6.back(), x_ij[toIdx(i, j)], 1.0));
				SCIP_CALL(SCIPaddCons(scip, constr6.back()));
			}
		}
		else
		{
			constr6.push_back(nullptr);
			SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr6.back(), "", 0, nullptr, nullptr, -SCIPinfinity(scip), 1.0));
			for (int i = 0; i < instance.facilityCount; ++i)
			{
				SCIP_CALL(SCIPaddCoefLinear(scip, constr6.back(), x_ij[toIdx(i, j)], 1.0));
			}
			SCIP_CALL(SCIPaddCons(scip, constr6.back()));
		}
	}

	SCIP_CALL(SCIPsolve(scip));
	SCIP_SOL* sol = nullptr;
	sol = SCIPgetBestSol(scip);

	//puck data
	answer.income = SCIPgetSolOrigObj(scip, sol);
	answer.prices.resize(instance.facilityCount);
	for (int i = 0; i < instance.facilityCount; ++i)
	{
		answer.prices[i] = SCIPgetSolVal(scip, sol, p_i[i]);
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

ProblemSolver::ProblemSolver(const ivector& constraint, const Instance& instance, bool relax)
{
	SolveProblem(constraint, instance, relax);
}

SCIP_RETCODE FirstVectorSolver::SolveProblem(const ivector& constraint, const Instance& instance)
{
	SCIP* scip = nullptr;
	SCIP_CALL(SCIPcreate(&scip));
	SCIP_CALL(SCIPincludeDefaultPlugins(scip)); //  include default plugins
	SCIPsetMessagehdlrQuiet(scip, true);

	SCIP_CALL(SCIPcreateProbBasic(scip, "first"));
	SCIP_CALL(SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE));

	std::vector<SCIP_VAR*> p_i(instance.facilityCount, nullptr);
	for (auto& p : p_i)
	{
		SCIP_CALL(SCIPcreateVarBasic(scip, &p, "", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_CONTINUOUS));
		SCIP_CALL(SCIPaddVar(scip, p));
	}
	std::vector<SCIP_VAR*> x_i;

	double eps = 0.000000001f; // TODO remove this hack
	double D = 10000000000; // TODO remove this hack

	std::vector<SCIP_CONS*> constr1;
	for (int j = 0; j < constraint.size(); ++j)
	{
		if (constraint[j] == std::numeric_limits<int>::max())
		{
			for (int i = 0; i < instance.facilityCount; ++i)
			{
				constr1.push_back(nullptr);
				SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr1.back(), "", 0, nullptr, nullptr, -SCIPinfinity(scip), 
					-instance.budgets[j] + instance.costs[i][j]-eps));
				SCIP_CALL(SCIPaddCoefLinear(scip, constr1.back(), p_i[i], -1.0));
				SCIP_CALL(SCIPaddCons(scip, constr1.back()));
			}
		}
		else
		{
			for (int i = 0; i < instance.facilityCount; ++i)
			{
				x_i.push_back(nullptr);
				SCIP_CALL(SCIPcreateVarBasic(scip, &x_i.back(), "", 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY));
				SCIP_CALL(SCIPaddVar(scip, x_i.back()));
				if (i == constraint[j])
				{
					continue;
				}
				constr1.push_back(nullptr);
				SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr1.back(), "", 0, nullptr, nullptr, -SCIPinfinity(scip), 
					instance.costs[i][j] - instance.costs[constraint[j]][j]));
				SCIP_CALL(SCIPaddCoefLinear(scip, constr1.back(), p_i[i], -1.0));
				SCIP_CALL(SCIPaddCoefLinear(scip, constr1.back(), p_i[constraint[j]], 1.0));
				SCIP_CALL(SCIPaddCoefLinear(scip, constr1.back(), x_i.back(), -D));
				SCIP_CALL(SCIPaddCons(scip, constr1.back()));

				constr1.push_back(nullptr);
				SCIP_CALL(SCIPcreateConsBasicLinear(scip, &constr1.back(), "", 0, nullptr, nullptr, -SCIPinfinity(scip),
					-instance.budgets[j] + instance.costs[i][j] - eps + D));
				SCIP_CALL(SCIPaddCoefLinear(scip, constr1.back(), p_i[i], -1.0));
				SCIP_CALL(SCIPaddCoefLinear(scip, constr1.back(), p_i[constraint[j]], 1.0));
				SCIP_CALL(SCIPaddCoefLinear(scip, constr1.back(), x_i.back(), +D));
				SCIP_CALL(SCIPaddCons(scip, constr1.back()));
			}
		}
		
	}

	SCIP_CALL(SCIPsolve(scip));
	SCIP_SOL* sol = nullptr;
	sol = SCIPgetBestSol(scip);

	//puck data
	exist = sol != nullptr;
	prices.resize(instance.facilityCount);
	for (int i = 0; i < instance.facilityCount; ++i)
	{
		prices[i] = SCIPgetSolVal(scip, sol, p_i[i]);
	}

	for (auto& l : constr1)
	{
		SCIP_CALL(SCIPreleaseCons(scip, &l));
	}
	for (auto& l : p_i)
	{
		SCIP_CALL(SCIPreleaseVar(scip, &l));
	}
	for (auto& l : x_i)
	{
		SCIP_CALL(SCIPreleaseVar(scip, &l));
	}
	SCIP_CALL(SCIPfree(&scip));

	return SCIP_OKAY;
}

FirstVectorSolver::FirstVectorSolver(const ivector& constraint, const Instance& instance)
{
	SolveProblem(constraint, instance);
}