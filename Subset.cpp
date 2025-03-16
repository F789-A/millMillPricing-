#include "Subset.h"

SubsetIterator::SubsetIterator(int setSize, int cardinality) : setSize(setSize), cardinality(cardinality), data(cardinality, 0)
{
	for (int i = 0; i < cardinality; ++i)
	{
		data[i] = i;
	}
}

SubsetIterator& SubsetIterator::operator++()
{
	int cur = cardinality;
	while (cur > 0) 
	{
		--cur;
		++data[cur];
		if (data[cur] < setSize - (cardinality - 1 - cur))
		{
			break;
		}
	}
	for (int j = cur; j < cardinality-1; ++j)
	{
		data[j+1] = data[j] + 1;
	}
	return *this;
}

const std::vector<int>& SubsetIterator::operator*() const
{
	return data;
}

bool SubsetIterator::End() const
{
	return data[0] > setSize - cardinality;
}