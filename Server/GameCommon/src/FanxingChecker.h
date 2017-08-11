#pragma once
#include <vector>
#include <map>
#include "IFanxing.h"
#include "log4z.h"
#include "IMJPlayer.h"
class FanxingChecker
{
public:
	virtual ~FanxingChecker()
	{
		for (auto& ref : m_vFanxing)
		{
			delete ref.second;
			ref.second = nullptr;
		}
		m_vFanxing.clear();
	}

	bool addFanxing(IFanxing* pFanxing)
	{
		auto iter = m_vFanxing.find(pFanxing->getFanxingType());
		if (iter != m_vFanxing.end())
		{
			LOGFMTE( "already add this fanxing = %u do not add again",pFanxing->getFanxingType() );
			return false;
		}
		m_vFanxing[pFanxing->getFanxingType()] = pFanxing;
		return true;
	}

	void checkFanxing( std::vector<eFanxingType>& vFanxing, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom )
	{
		for (auto& ref : m_vFanxing)
		{
			if (ref.second->checkFanxing(pPlayer->getPlayerCard(), pPlayer, nInvokerIdx, pmjRoom))
			{
				vFanxing.push_back((eFanxingType)ref.first);
			}
		}
	}

protected:
	std::map<uint16_t, IFanxing*> m_vFanxing;
};
