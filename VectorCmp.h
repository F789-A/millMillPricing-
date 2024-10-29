#pragma once

class VectorCmp
{
public:
	bool operator()(const ivector& vec1, const ivector& vec2) const
	{
		assert(vec1.size() == vec2.size());
		for (int i = 0; i < vec1.size(); ++i)
		{
			if (vec1[i] < vec2[i])
			{
				return true;
			}
			if (vec1[i] > vec2[i])
			{
				return false;
			}
		}
		return false;
	}
};