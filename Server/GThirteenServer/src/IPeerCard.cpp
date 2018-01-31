#include "IPeerCard.h"

void IPeerCard::addCardToVecAsc(VEC_CARD& vec, uint8_t nCard)
{
	auto iter = vec.begin();
	for (; iter < vec.end(); ++iter)
	{
		if ((*iter) < nCard)
		{
			vec.insert(iter, nCard);
			return;
		}
	}
	vec.push_back(nCard);
}

void IPeerCard::eraseVector(uint8_t p, VEC_CARD& typeVec) {
	VEC_CARD::iterator it = find(typeVec.begin(), typeVec.end(), p);
	if (it != typeVec.end())
	{
		typeVec.erase(it);
	}
}