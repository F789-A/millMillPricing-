#pragma once
#include <vector>

class SubsetIterator
{
public:
	SubsetIterator(int setSize, int cardinality);

	SubsetIterator& operator++();
	const std::vector<int>& operator*() const;

	bool End() const;
	int Cardinality() const;

private:
	int cardinality;
	int setSize;
	std::vector<int> data;
};