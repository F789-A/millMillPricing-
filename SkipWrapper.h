#pragma once
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include <vector>

class SkipWrapper
{
	void Init(SCIP_Objsense Objsense);
	SCIP_VAR* AddVar(double lb, double rb, double obj, SCIP_Vartype Vartype);
	void AddConstr(const std::vector<std::pair<SCIP_VAR*, double>>& vars, double lb, double rb);
	void Destroy();

	void Solve();
	double GetSolObj();
	double GetSolVar(SCIP_VAR* var);

private:
	SCIP* scip = nullptr;
	std::vector<SCIP_VAR*> vars;
	std::vector<SCIP_CONS*> cons;
};