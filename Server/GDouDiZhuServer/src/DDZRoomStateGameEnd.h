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
		// find winer 
		uint8_t nWinerIdx = -1;
		for (uint8_t nIdx = 0; nIdx < getRoom()->getSeatCnt(); ++nIdx)
		{
			auto p = (DDZPlayer*)getRoom()->getPlayerByIdx(nIdx);
			if (p == nullptr)
			{
				LOGFMTE( "why room player is null room id = %u, idx = %u",getRoom()->getRoomID(),nIdx );
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
			LOGFMTE( "why no player is null ? room id = %u",getRoom()->getRoomID() );
			return;
		}

		// check chun tian 
		bool isChunTian = true;
		auto pBanker = (DDZPlayer*)getRoom()->getPlayerByIdx(((DDZRoom*)getRoom())->getBankerIdx());
		if ( nullptr == pBanker)
		{
			LOGFMTE( "why banker is null room id = %u",getRoom()->getRoomID() );
			return;
		}

		bool isBankerWin = nWinerIdx == pBanker->getIdx();
		if ( isBankerWin ) // idle player do not chu card 
		{
			for (uint8_t nIdx = 0; nIdx < getRoom()->getSeatCnt(); ++nIdx)
			{
				if (nIdx == ((DDZRoom*)getRoom())->getBankerIdx())
				{
					continue;
				}

				auto p = (DDZPlayer*)getRoom()->getPlayerByIdx(nIdx);
				if ( p == nullptr )
				{
					LOGFMTE("why room player is null room id = %u, idx = %u", getRoom()->getRoomID(), nIdx);
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
		uint8_t nBombCnt = ((DDZRoom*)getRoom())->getBombCount();
		int32_t nOffset = ((DDZRoom*)getRoom())->getBankTimes() * (isMingPai ? 2 : 1) * pow(2, nBombCnt) * (isChunTian ? 2 : 1);

		// do caculate 
		for (uint8_t nIdx = 0; nIdx < getRoom()->getSeatCnt(); ++nIdx)
		{
			auto pPlayer = getRoom()->getPlayerByIdx(nIdx);
			if (pPlayer == pBanker)
			{
				continue;
			}

			if (nullptr == pPlayer)
			{
				LOGFMTE( "result player is null why , offset = %u , room id = %u",nOffset,getRoom()->getRoomID() );
				continue;
			}

			int32_t nBankerOffsetThisPlayer = nOffset * (isBankerWin ? 1 : -1);
			pBanker->addSingleOffset(nBankerOffsetThisPlayer);
			pPlayer->addSingleOffset(nBankerOffsetThisPlayer * -1 );
		}

		// send msg tell result ;
		Json::Value jsMsg;
		jsMsg["bombCnt"] = nBombCnt;
		jsMsg["isChunTian"] = isChunTian ? 1 : 0;
		jsMsg["isMingPai"] = isMingPai ? 1 : 0;
		jsMsg["bottom"] = ((DDZRoom*)getRoom())->getBankTimes();

		Json::Value jsPlayersOffset;
		for (uint8_t nIdx = 0; nIdx < getRoom()->getSeatCnt(); ++nIdx)
		{
			auto p = (DDZPlayer*)getRoom()->getPlayerByIdx(nIdx);
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
		getRoom()->sendRoomMsg(jsMsg,MSG_DDZ_ROOM_RESULT);
	}
};