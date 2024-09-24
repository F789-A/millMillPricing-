#include "Instance.h"

Instance::Instance(const table& CostsLeader, const table& CostsFollower, const ivector& Budgets)
	: costsLeader(CostsLeader), costsFollower(CostsFollower), budgets(Budgets),
	leaderFacilityCount(CostsLeader.size()), followerFacilityCount(CostsFollower.size()), clientsCount(Budgets.size())
{
	pUpperBound.resize(leaderFacilityCount);
	for (int i = 0; i < leaderFacilityCount; ++i)
	{
		pUpperBound[i] = std::numeric_limits<float>::min();
		for (int j = 0; j < clientsCount; ++j)
		{
			pUpperBound[i] = std::max(pUpperBound[i], budgets[j] - costsLeader[i][j]);
		}
	}
	qUpperBound.resize(followerFacilityCount);
	for (int i = 0; i < followerFacilityCount; ++i)
	{
		qUpperBound[i] = std::numeric_limits<float>::min();
		for (int j = 0; j < clientsCount; ++j)
		{
			qUpperBound[i] = std::max(qUpperBound[i], budgets[j] - costsFollower[i][j]);
		}
	}
}
