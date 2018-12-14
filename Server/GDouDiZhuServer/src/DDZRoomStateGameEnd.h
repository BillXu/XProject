#pragma once
#include "IGameRoomState.h"
#include "CommonDefine.h"
#include "GameRoom.h"
#include "DDZPlayer.h"
#include "DDZPlayerCard.h"
#include "DDZRoom.h"
class DDZRoomStateGameEnd
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_GameEnd; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		caculateResult();
		getRoom()->onGameEnd();
		setStateDuringTime(0.01);
	}

	void onStateTimeUp()
	{
		getRoom()->onGameDidEnd();
		getRoom()->goToState(eRoomSate_WaitReady);
	}
protected:
	void caculateResult()
	{
		auto pRoom = (DDZRoom*)getRoom();

		// find winer 
		uint8_t nWinerIdx = -1;
		for (uint8_t nIdx = 0; nIdx < pRoom->getSeatCnt(); ++nIdx)
		{
			auto p = (DDZPlayer*)pRoom->getPlayerByIdx(nIdx);
			if (p == nullptr)
			{
				LOGFMTE( "why room player is null room id = %u, idx = %u", pRoom->getRoomID(),nIdx );
				continue;
			}

			if (p->getPlayerCard()->getHoldCardCount() <= 0)
			{
				nWinerIdx = nIdx;
				break;
			}
		}

		if (((uint8_t)-1) == nWinerIdx )
		{
			LOGFMTE( "why no player is null ? room id = %u", pRoom->getRoomID() );
			return;
		}

		// check chun tian 
		bool isChunTian = true;
		auto pBanker = (DDZPlayer*)pRoom->getPlayerByIdx(pRoom->getBankerIdx());
		if ( nullptr == pBanker)
		{
			LOGFMTE( "why banker is null room id = %u", pRoom->getRoomID() );
			return;
		}

		bool isBankerWin = nWinerIdx == pBanker->getIdx();
		if ( isBankerWin ) // idle player do not chu card 
		{
			for (uint8_t nIdx = 0; nIdx < pRoom->getSeatCnt(); ++nIdx)
			{
				if (nIdx == pRoom->getBankerIdx())
				{
					continue;
				}

				auto p = (DDZPlayer*)pRoom->getPlayerByIdx(nIdx);
				if ( p == nullptr )
				{
					LOGFMTE("why room player is null room id = %u, idx = %u", pRoom->getRoomID(), nIdx);
					continue;
				}

				if ( p->getPlayerCard()->getChuedCardTimes() > 0 )
				{
					isChunTian = false;
					break;
				}
			}
		}
		else
		{
			isChunTian = pBanker->getPlayerCard()->getChuedCardTimes() == 1;
		}

		// check ming pai 
		bool isMingPai = pBanker->isMingPai();

		// do caculate 
		uint8_t nBombCnt = pRoom->getBombCount();
		//int32_t nOffset = pRoom->getBankTimes() * (isMingPai ? 2 : 1) * pow(2, nBombCnt) * (isChunTian ? 2 : 1);
		int32_t nMultiple = pRoom->getBankTimes() * (isMingPai ? 2 : 1) * pow(2, nBombCnt) * (isChunTian ? 2 : 1);
		for (uint8_t nIdx = 0; nIdx < pRoom->getSeatCnt(); ++nIdx)
		{
			auto pPlayer = (DDZPlayer*)pRoom->getPlayerByIdx(nIdx);
			if (pPlayer == pBanker)
			{
				continue;
			}

			if (nullptr == pPlayer)
			{
				LOGFMTE( "result player is null why , offset = %u , room id = %u", nMultiple, pRoom->getRoomID() );
				continue;
			}

			int32_t nBankerMultipleThisPlayer = nMultiple * ( pBanker->isChaoZhuang() ? 2 : 1 ) * (pPlayer->isChaoZhuang() ? 2 : 1) * ( ( pBanker->isTiLaChuai() && pPlayer->isTiLaChuai() ) ? 2 : 1) * (pPlayer->isTiLaChuai() ? 2 : 1) * (pPlayer->getDouble() ? pPlayer->getDouble() : 1) * (pBanker->getDouble() ? pBanker->getDouble() : 1);
			
			//±¶Êý·â¶¥
			if (pRoom->getFanLimit() && nBankerMultipleThisPlayer > (int32_t)pRoom->getFanLimit()) {
				nBankerMultipleThisPlayer = pRoom->getFanLimit();
			}

			//·ÖÊý·â¶¥
			int32_t nBankerOffsetThisPlayer = nBankerMultipleThisPlayer * (isBankerWin ? 1 : -1) * pRoom->getBaseScore();
			if (pRoom->fengDing() && abs(nBankerOffsetThisPlayer) > pRoom->fengDing())
			{
				nBankerOffsetThisPlayer = pRoom->fengDing() * (nBankerOffsetThisPlayer / abs(nBankerOffsetThisPlayer));
			}

			pBanker->addSingleOffset(nBankerOffsetThisPlayer);
			pPlayer->addSingleOffset(nBankerOffsetThisPlayer * -1 );
		}

		// send msg tell result ;
		Json::Value jsMsg;
		jsMsg["bombCnt"] = nBombCnt;
		jsMsg["isChunTian"] = isChunTian ? 1 : 0;
		jsMsg["isMingPai"] = isMingPai ? 1 : 0;
		jsMsg["bottom"] = pRoom->getBankTimes();

		Json::Value jsPlayersOffset;
		for (uint8_t nIdx = 0; nIdx < pRoom->getSeatCnt(); ++nIdx)
		{
			auto p = (DDZPlayer*)pRoom->getPlayerByIdx(nIdx);
			if (nullptr == p)
			{
				continue;
			}

			Json::Value jsPlayer;
			jsPlayer["idx"] = nIdx;
			jsPlayer["offset"] = p->getSingleOffset();

			Json::Value jsCards;
			p->getPlayerCard()->holdCardToJson(jsCards);
			if (jsCards.size() != 0)
			{
				jsPlayer["cards"] = jsCards;
			}

			jsPlayersOffset[jsPlayersOffset.size()] = jsPlayer;
		}
		jsMsg["players"] = jsPlayersOffset;

		// add frame 
		pRoom->addReplayFrame(DDZ_Frame_GameEnd, jsMsg);

		pRoom->sendRoomMsg(jsMsg,MSG_DDZ_ROOM_RESULT);
	}
};