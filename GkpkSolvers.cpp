#include "GlpkSolvers.h"

#include <iostream>

Instance::Instance(const table& CostsLeader, const table& CostsFollower, const vector& Budgets)
	: costsLeader(CostsLeader), costsFollower(CostsFollower), budgets(Budgets),
	leaderFacilityCount(CostsLeader.size()), followerFacilityCount(CostsFollower.size()), clientsCount(Budgets.size())
{
	pUpperBound.resize(leaderFacilityCount);
	for (int i = 0; i < leaderFacilityCount; ++i)
	{
		pUpperBound[i] = std::numeric_limits<float>::min();
		for (int j = 0; j < clientsCount; ++j)
		{
			pUpperBound[j] = std::max(pUpperBound[j], budgets[j] - costsLeader[i][j]);
		}
	}

	qUpperBound.resize(followerFacilityCount);
	for (int i = 0; i < followerFacilityCount; ++i)
	{
		qUpperBound[i] = std::numeric_limits<float>::min();
		for (int j = 0; j < clientsCount; ++j)
		{
			qUpperBound[j] = std::max(qUpperBound[j], budgets[j] - costsFollower[i][j]);
		}
	}
}

RelaxSolver::RelaxSolver(const ivector& constraint, const Instance& instance)
{
	const int p_offset = 1;
	const int z_offset = p_offset + instance.clientsCount;
	const int x_offset = z_offset + instance.clientsCount * instance.followerFacilityCount;

	std::vector<int> ia(1);
	std::vector<int> ja(1);
	std::vector<double> ar(1);

	glp_prob* lp = glp_create_prob();
	glp_set_prob_name(lp, "sample");
	glp_set_obj_dir(lp, GLP_MAX);

	glp_add_rows(lp, 3 * instance.clientsCount + 4 * instance.clientsCount * instance.followerFacilityCount);
	int offset = 1;
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		glp_set_row_bnds(lp, offset, GLP_LO, 0.0, 0.0);
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			ia.push_back(offset); ja.push_back(z_offset + i * instance.followerFacilityCount + j); ar.push_back(-1.0);
			ia.push_back(offset); ja.push_back(x_offset + i * instance.followerFacilityCount + j); ar.push_back(
				instance.budgets[j] - instance.costsFollower[i][j]);
		}
		++offset;
	}
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int k = 0; k < instance.followerFacilityCount; ++k)
		{
			glp_set_row_bnds(lp, offset, GLP_LO, -instance.costsFollower[k][j], 0.0);
			for (int i = 0; i < instance.followerFacilityCount; ++i)
			{
				ia.push_back(offset); ja.push_back(z_offset + i * instance.followerFacilityCount + j); ar.push_back(-1.0);
				ia.push_back(offset); ja.push_back(x_offset + i * instance.followerFacilityCount + j); ar.push_back(
					- instance.costsFollower[i][j]);
			}
			ia.push_back(offset); ja.push_back(p_offset + k); ar.push_back(1.0);
			++offset;
		}
	}

	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			glp_set_row_bnds(lp, offset, GLP_LO, -instance.qUpperBound[i], 0.0);
			ia.push_back(offset); ja.push_back(z_offset + i * instance.followerFacilityCount + j); ar.push_back(-1.0);
			ia.push_back(offset); ja.push_back(x_offset + i * instance.followerFacilityCount + j); ar.push_back(-instance.qUpperBound[i]);
			ia.push_back(offset); ja.push_back(p_offset + i); ar.push_back(1.0);
			++offset;
		}
	}
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			glp_set_row_bnds(lp, offset, GLP_LO, -instance.qUpperBound[i], 0.0);
			ia.push_back(offset); ja.push_back(z_offset + i * instance.followerFacilityCount + j); ar.push_back(-1.0);
			ia.push_back(offset); ja.push_back(x_offset + i * instance.followerFacilityCount + j); ar.push_back(instance.qUpperBound[i]);
			ia.push_back(offset); ja.push_back(p_offset + i); ar.push_back(-1.0);
			++offset;
		}
	}
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			glp_set_row_bnds(lp, offset, GLP_UP, 0.0, 0.0);
			ia.push_back(offset); ja.push_back(z_offset + i * instance.followerFacilityCount + j); ar.push_back(-1.0);
			ia.push_back(offset); ja.push_back(x_offset + i * instance.followerFacilityCount + j); ar.push_back(-instance.qUpperBound[i]);
			++offset;
		}
	}
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		glp_set_row_bnds(lp, offset, GLP_UP, 0.0, 1.0);
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			ia.push_back(offset); ja.push_back(x_offset + i * instance.followerFacilityCount + j); ar.push_back(instance.qUpperBound[i]);
		}
		++offset;
	}

	// add additional constraints
	for (int j = 0; j < constraint.size(); ++j)
	{
		if (constraint[j] != std::numeric_limits<int>::max())
		{ 
			glp_set_row_bnds(lp, offset, GLP_FX, 1.0, 1.0);
			ia.push_back(offset); ja.push_back(x_offset + constraint[j] * instance.followerFacilityCount + j); ar.push_back(1.0);
			++offset;
		}
		else
		{
			for (int i = 0; i < instance.followerFacilityCount; ++i)
			{
				glp_set_row_bnds(lp, offset, GLP_FX, 0.0, 0.0);
				ia.push_back(offset); ja.push_back(x_offset + i * instance.followerFacilityCount + j); ar.push_back(1.0);
				++offset;
			}
		}
	}

	glp_add_cols(lp, (2 * instance.clientsCount + 1) * instance.followerFacilityCount);
	for (int i = 0; i < instance.followerFacilityCount; ++i) // p_i
	{
		glp_set_col_bnds(lp, p_offset + i, GLP_LO, 0.0, 0.0);
		glp_set_obj_coef(lp, p_offset + i, 0.0);
	}
	for (int k = 0; k < instance.clientsCount * instance.followerFacilityCount; ++k) // z_ij
	{
		glp_set_col_bnds(lp, z_offset + k, GLP_LO, 0.0, 0.0);
		glp_set_obj_coef(lp, z_offset + k, 1.0);
	}
	for (int k = 0; k < instance.clientsCount * instance.followerFacilityCount; ++k) // x_ij
	{
		glp_set_col_bnds(lp, x_offset+k, GLP_DB, 0.0, 1.0);
		glp_set_obj_coef(lp, x_offset + k, 0.0);
	}

	glp_load_matrix(lp, ar.size() - 1, ia.data(), ja.data(), ar.data());
	glp_simplex(lp, NULL);

	answer.followerPrices.resize(instance.clientsCount);
	for (int i = 0; i < answer.followerPrices.size(); ++i)
	{
		answer.followerPrices[i] = glp_get_col_prim(lp, p_offset + i);
	}
	answer.followerIncome = glp_get_obj_val(lp);

	glp_delete_prob(lp);
}

FirstVectorSolver::FirstVectorSolver(const ivector& constraint, const Instance& instance)
{
	const int q_offset = 1;
	const int x_offset = q_offset + instance.followerFacilityCount;
	const float D = 100000; //TODO

	std::vector<int> ia(1);
	std::vector<int> ja(1);
	std::vector<double> ar(1);

	glp_prob* mip = glp_create_prob();
	glp_set_prob_name(mip, "sample");
	glp_set_obj_dir(mip, GLP_MAX);

	int countOfX = 0;
	int constCount = 0;
	for (int j = 0; j < constraint.size(); ++j)
	{
		if (constraint[j] == std::numeric_limits<int>::max())
		{
			constCount += instance.followerFacilityCount;
		}
		else
		{
			constCount += (instance.followerFacilityCount-1) * 2;
			++countOfX;
		}
	}

	glp_add_rows(mip, constCount);
	int x_counter = 0;
	int offset = 1;
	for (int j = 0; j < constraint.size(); ++j)
	{
		if (constraint[j] == std::numeric_limits<int>::max())
		{
			for (int i = 0; i < instance.followerFacilityCount; ++i)
			{
				glp_set_row_bnds(mip, offset, GLP_UP, 0.0, -instance.budgets[j] + instance.costsFollower[q_offset + i][j]);
				ia.push_back(offset); ja.push_back(q_offset + i); ar.push_back(-1.0);
				++offset;
			}
		}
		else
		{
			for (int i = 0; i < instance.followerFacilityCount; ++i)
			{
				if (i == constraint[j])
				{
					continue;
				}
				glp_set_row_bnds(mip, offset, GLP_UP, 0.0, D + instance.costsFollower[q_offset + i][j] - instance.costsFollower[q_offset + constraint[j]][j]);
				ia.push_back(offset); ja.push_back(q_offset + constraint[j]); ar.push_back(1.0);
				++offset;
				ia.push_back(offset); ja.push_back(q_offset + i); ar.push_back(-1.0);
				++offset;
				ia.push_back(offset); ja.push_back(x_offset + x_counter); ar.push_back(D);
				++offset;

				glp_set_row_bnds(mip, offset, GLP_UP, 0.0, -instance.budgets[j] + instance.costsFollower[q_offset + i][j]);
				ia.push_back(offset); ja.push_back(q_offset + i); ar.push_back(-1.0);
				++offset;
				ia.push_back(offset); ja.push_back(x_offset + x_counter); ar.push_back(-D);
				++offset;

				++x_counter;
			}
		}
	}

	glp_add_cols(mip, countOfX + instance.followerFacilityCount);
	for (int i = 0; i < instance.followerFacilityCount; ++i) // p_i
	{
		glp_set_col_bnds(mip, q_offset + i, GLP_LO, 0.0, 0.0);
		glp_set_obj_coef(mip, q_offset + i, 0.0);
	}
	for (int k = 0; k < countOfX; ++k) // x_i
	{
		glp_set_col_bnds(mip, x_offset + k, GLP_DB, 0.0, 1.0);
		glp_mip_col_val(mip, x_offset + k);
	}

	glp_load_matrix(mip, ar.size() - 1, ia.data(), ja.data(), ar.data());
	glp_intopt(mip, NULL);
	int status = glp_mip_status(mip);
	exist = status == GLP_FEAS || status == GLP_OPT;

	if (exist)
	{
		answer.followerPrices.resize(instance.clientsCount);
		for (int i = 0; i < answer.followerPrices.size(); ++i)
		{
			answer.followerPrices[i] = glp_get_col_prim(mip, q_offset + i);
		}
	}
}