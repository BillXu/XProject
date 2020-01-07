#pragma once
#include "IMJRoom.h"
#include "IMJPoker.h"
#include "OptsNJ.h"
class NJMJ
	:public IMJRoom
{
public:
	struct stSettle
	{
		std::map<uint8_t, uint16_t> vWinIdxs;
		std::map<uint8_t, uint16_t> vLoseIdx;
		eMJActType eSettleReason;
		void addWin(uint8_t nIdx, uint16_t nWinCoin)
		{
			vWinIdxs[nIdx] = nWinCoin;
		}

		void addLose(uint8_t nIdx, uint16_t nLoseCoin)
		{
			auto iter = vLoseIdx.find(nIdx);
			if (iter != vLoseIdx.end())
			{
				iter->second += nLoseCoin;
				return;
			}
			vLoseIdx[nIdx] = nLoseCoin;
		}
	};

	struct stChuedCards
	{
		uint8_t nCard;
		std::vector<uint8_t> vFollowedIdxs;
		void clear()
		{
			vFollowedIdxs.clear();
			nCard = 0;
		}

		void addChuedCard(uint8_t nChuCard, uint8_t nIdx)
		{
			if (nChuCard != nCard)
			{
				clear();
			}

			nCard = nChuCard;
			vFollowedIdxs.push_back(nIdx);
		}

		bool isInvokerFanQian(uint8_t& vTargetIdx)
		{
			if (vFollowedIdxs.size() == 4)
			{
				vTargetIdx = vFollowedIdxs.front();
			}

			return vFollowedIdxs.size() == 4;
		}
	};
public:
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	IGamePlayer* createGamePlayer()override;
	uint8_t getRoomType()override;
	IPoker* getPoker()override;

	bool isHaveLouPeng()override { return true; }
	bool isGameOver()override;
	void onPlayerChu(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	void onPlayerAnGang(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerBuGang(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
protected:
	bool canPlayerHuWithCard(IMJPlayer* p, uint8_t nCard, uint8_t nInvokerIdx)override;
	void addSettle(stSettle& tSettle);
	void doProcessChuPaiFanQian(uint8_t chuIdx, uint8_t chuCard);
	bool isBiXiaHu();
protected:
	IMJPoker m_tPoker;
	OptsNJ mOpts;
	stChuedCards m_tChuedCards;
	std::vector<stSettle> m_vSettle;
	bool m_isWillBiXiaHu;
};