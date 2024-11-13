#include "ClientProblemSolver.h"

#include <iostream>

void ClientProblemSolver::Solve(const ivector& leaderPrice, const ivector& followerPrice, const Instance& instance)
{
	leaderIncome = 0;
	followerIncome = 0;
	clientChose.clear();
	for (int j = 0; j < instance.clientsCount; ++j)
	{
		int leaderBestFacility = -1;
		int minLeaderCost = std::numeric_limits<int>::max();
		for (int i = 0; i < instance.leaderFacilityCount; ++i)
		{
			int tmpMinLeaderCost = instance.costsLeader[i][j] + leaderPrice[i];
			if (instance.budgets[j] - tmpMinLeaderCost >= 0 
				&& (minLeaderCost > tmpMinLeaderCost || 
					(minLeaderCost == tmpMinLeaderCost && (leaderBestFacility == -1 || leaderPrice[i] > leaderPrice[leaderBestFacility]))))
			{
				minLeaderCost = tmpMinLeaderCost;
				leaderBestFacility = i;
			}
		}
		int followerBestFacility = -1;
		int minFollowerCost = std::numeric_limits<int>::max();
		for (int i = 0; i < instance.followerFacilityCount; ++i)
		{
			int tmpMinFollowerCost = instance.costsFollower[i][j] + followerPrice[i];
			if (instance.budgets[j] - tmpMinFollowerCost >= 0 
				&& (minFollowerCost > tmpMinFollowerCost || 
					(minFollowerCost == tmpMinFollowerCost && (followerBestFacility == -1 || followerPrice[i] > followerPrice[followerBestFacility]))))
			{
				minFollowerCost = tmpMinFollowerCost;
				followerBestFacility = i;
			}
		}
		if (minLeaderCost < minFollowerCost && leaderBestFacility >= 0)
		{
			leaderIncome += leaderPrice[leaderBestFacility];
			clientChose.push_back('l');
			_clientChose.push_back(9);
		}
		else if (minLeaderCost >= minFollowerCost && followerBestFacility >= 0)
		{
			followerIncome += followerPrice[followerBestFacility];
			clientChose.push_back('f');
			_clientChose.push_back(followerBestFacility);
		}
		else
		{
			clientChose.push_back('u');
			_clientChose.push_back(9);
		}
	}
}