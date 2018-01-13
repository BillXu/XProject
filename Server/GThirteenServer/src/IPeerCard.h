#pragma once
#include "NativeTypes.h"
#include <cassert>
#include "ThirteenDefine.h"
class IPeerCard
{
public:
	typedef std::vector<uint8_t> VEC_CARD;
public:
	virtual ~IPeerCard(){}
	virtual void reset() = 0;
	virtual void addCompositCardNum( uint8_t nCardCompositNum ) = 0;
	virtual bool setDao(uint8_t nIdx, VEC_CARD vCards) = 0;
	virtual bool autoSetDao() = 0;
	virtual const char* getNameString() = 0 ;
	virtual uint32_t getWeight() = 0;
	virtual uint8_t getType() = 0;
	//virtual IPeerCard* swap(IPeerCard* pTarget) = 0;
	virtual PK_RESULT pk(IPeerCard* pTarget) = 0;

	bool operator < (IPeerCard& refTarget )
	{
		return pk(&refTarget) == PK_RESULT_FAILED ;
	}

	bool operator > (IPeerCard& refTarget)
	{
		return pk(&refTarget) == PK_RESULT_WIN ;
	}

	bool operator == (IPeerCard& refTarget)
	{
		return pk(&refTarget) == PK_RESULT_EQUAL ;
	}

	void static addCardToVecAsc(VEC_CARD& vec, uint8_t nCard)
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

	void static eraseVector(uint8_t p, VEC_CARD& typeVec) {
		VEC_CARD::iterator it = find(typeVec.begin(), typeVec.end(), p);
		if (it != typeVec.end())
		{
			typeVec.erase(it);
		}
	}
};