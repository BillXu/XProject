#include "BJRoom.h"
#include "BJRoomStateWaitPlayerReady.h"
#include "BJRoomStateStartGame.h"
#include "BJRoomStateMakeGroupCard.h"
#include "BJRoomStateGameEnd.h"
#include "BJPlayer.h"
#include <algorithm>
#include "BJRoomStateGameEnd.h"
#include "BJRoomStateMakeGroupCard.h"
#include "BJRoomStateStartGame.h"
#include "BJRoomStateWaitPlayerReady.h"
bool BJRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	GameRoom::init(pRoomMgr,nSeialNum,nRoomID,nSeatCnt,vJsOpts);

	IGameRoomState* pState = new BJRoomStateWaitReady();
	addRoomState(pState);
	setInitState(pState);
	pState = new BJRoomStateGameEnd();
	addRoomState(pState);
	pState = new BJRoomStateMakeGroup();
	addRoomState(pState);
	pState = new BJRoomStateStartGame();
	addRoomState(pState);
	return true;
}

IGamePlayer* BJRoom::createGamePlayer()
{
	return new BJPlayer();
}

void BJRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	GameRoom::packRoomInfo(jsRoomInfo);
}

void BJRoom::visitPlayerInfo( IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID )
{
	GameRoom::visitPlayerInfo(pPlayer,jsPlayerInfo,nVisitorSessionID );
	if ( eRoomState_GameEnd == getCurState()->getStateID() || (getCurState()->getStateID() == eRoomState_BJ_Make_Group && nVisitorSessionID == pPlayer->getSessionID()))
	{
		auto p = (BJPlayer*)pPlayer;
		Json::Value jsHoldCards;
		p->getPlayerCard()->holdCardToJson(jsHoldCards);
		jsPlayerInfo["holdCards"] = jsHoldCards;
	}
}

uint8_t BJRoom::getRoomType()
{
	return eGame_BiJi;
}

void BJRoom::onStartGame()
{
	GameRoom::onStartGame();
	// distribute card 
	auto nSeatCnt = getSeatCnt();
	for (uint8_t nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto p = (BJPlayer*)getPlayerByIdx(nIdx);
		if (p == nullptr || (p->haveState(eRoomPeer_CanAct) == false))
		{
			continue;
		}

		uint8_t nCardCnt = 9;
		while (nCardCnt--)
		{
			p->getPlayerCard()->addCompositCardNum(getPoker()->distributeOneCard());
		}
	}

	// send start game msg
	Json::Value jsMsg;
	for (uint8_t nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto p = (BJPlayer*)getPlayerByIdx(nIdx);
		if (p == nullptr || (p->haveState(eRoomPeer_CanAct) == false))
		{
			continue;
		}

		Json::Value jsVHoldCard;
		if ( p->haveState(eRoomPeer_CanAct))
		{
			p->getPlayerCard()->holdCardToJson(jsVHoldCard);
		}
		jsMsg["vSelfCard"] = jsVHoldCard;
		sendMsgToPlayer(jsMsg, MSG_ROOM_BJ_START_GAME, p->getSessionID());
	}
}

void BJRoom::onGameEnd()
{
	// find playing players ;
	std::vector<BJPlayer*> vActivePlayers;
	auto nSeatCnt = getSeatCnt();
	for (uint8_t nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto p = getPlayerByIdx(nIdx);
		if ( p && p->haveState(eRoomPeer_DoMakedCardGroup))
		{
			vActivePlayers.push_back((BJPlayer*)p);
		}
	}

	// caculate per guo ,
	for (uint8_t nGuoIdx = 0; nGuoIdx < 3; ++nGuoIdx)
	{
		for ( auto& p : vActivePlayers)
		{
			p->getPlayerCard()->setCurGroupIdx(nGuoIdx);
		}
		
		std::sort(vActivePlayers.begin(), vActivePlayers.end(), []( BJPlayer* pLeft , BJPlayer* pRight )
		{
			return pLeft->getPlayerCard()->getWeight() < pRight->getPlayerCard()->getWeight();
		});

		// caculate fen shu ;
		int32_t nWinerPos = vActivePlayers.size() - 1;
		for ( int32_t nLoserPos = 0; nLoserPos < nWinerPos; ++nLoserPos)
		{
			int32_t nWin = nWinerPos - nLoserPos;
			vActivePlayers[nWinerPos]->addOffsetPerGuo(nGuoIdx, nWin);
			vActivePlayers[nLoserPos]->addOffsetPerGuo(nGuoIdx, nWin * -1 );
		}
	}
	// send game result msg ; 
	Json::Value jsArrayPlayers;
	for (auto& p : vActivePlayers)
	{
		Json::Value jsPlayerResult;
		jsPlayerResult["idx"] = p->getIdx();
		//jsPlayerResult["offset"] = p->getSingleOffset();

		Json::Value jsGuoArrays;
		for (uint8_t nIdx = 0; nIdx < 3; ++nIdx)
		{
			std::vector<uint8_t> vGuoCards;
			uint8_t nCardType = 0;
			p->getPlayerCard()->getGroupInfo(nIdx, nCardType, vGuoCards);

			Json::Value jsGuoInfo;
			//jsGuoInfo["idx"] = nIdx;
			jsGuoInfo["type"] = nCardType;
			jsGuoInfo["offset"] = p->getOffsetPerGuo(nIdx);

			Json::Value jsArrayCards;
			for (auto& ref : vGuoCards)
			{
				jsArrayCards[jsArrayCards.size()] = ref;
			}
			jsGuoInfo["cards"] = jsArrayCards;
			jsGuoArrays[jsGuoArrays.size()] = jsGuoInfo;
		}
		jsPlayerResult["vGuoInfo"] = jsGuoArrays;
		jsArrayPlayers[jsArrayPlayers.size()] = jsPlayerResult;
	}

	Json::Value jsMsg;
	jsMsg["players"] = jsArrayPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_BJ_GAME_END );
	GameRoom::onGameEnd();
}

bool BJRoom::canStartGame()
{
	if (false == GameRoom::canStartGame())
	{
		return false;
	}

	uint16_t nReadyCnt = 0;
	for (uint16_t nIdx = 0; nIdx < getSeatCnt(); ++nIdx)
	{
		auto p = getPlayerByIdx(nIdx);
		if (nullptr == p)
		{
			continue;
		}

		if (p->haveState(eRoomPeer_Ready) == false)
		{
			return false;
		}

		++nReadyCnt;
	}
	return nReadyCnt >= 2;
}

IPoker* BJRoom::getPoker()
{
	return &m_tPoker;
}

void BJRoom::onPlayerReady(uint16_t nIdx)
{
	auto pPlayer = getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("idx = %u target player is null ptr can not set ready", nIdx);
		return;
	}
	pPlayer->setState(eRoomPeer_Ready);
	// msg ;
	Json::Value jsMsg;
	jsMsg["idx"] = nIdx;
	sendRoomMsg(jsMsg, MSG_ROOM_PLAYER_READY);
	LOGFMTD("do send player set ready idx = %u room id = %u", nIdx, getRoomID())
}

bool BJRoom::isAllPlayerMakedGroupCard()
{
	auto nSeatCnt = getSeatCnt();
	for (auto nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto pPlayer = (BJPlayer*)getPlayerByIdx(nIdx);
		if (pPlayer == nullptr)
		{
			continue;
		}

		if (false == pPlayer->haveState(eRoomPeer_DoMakedCardGroup))
		{
			return false;
		}
	}
	return true;
}

uint8_t BJRoom::onPlayerDoMakeCardGroup(uint8_t nIdx, std::vector<uint8_t>& vGroupCards)
{
	auto pPlayer = (BJPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE( "player is null , how to make card group idx = %u , room id = %u",nIdx,getRoomID() );
		return 1;
	}

	auto pPlayerCard = pPlayer->getPlayerCard();
	if ( !pPlayerCard->setCardsGroup(vGroupCards))
	{
		return 2;
	}
	pPlayer->setState(eRoomPeer_DoMakedCardGroup);

	// send msg player maked card group 
	Json::Value jsmsg;
	jsmsg["idx"] = nIdx;
	sendRoomMsg(jsmsg, MSG_ROOM_BJ_MAKE_GROUP_CARD);
	return 0;
}

bool BJRoom::onPlayerAutoMakeCardGroupAllPlayerOk()
{
	auto nSeatCnt = getSeatCnt();
	for ( uint8_t nIdx = 0; nIdx < nSeatCnt; ++nIdx )
	{
		auto p = getPlayerByIdx(nIdx);
		if ( p == nullptr || (p->haveState(eRoomPeer_CanAct) == false) || p->haveState(eRoomPeer_DoMakedCardGroup) )
		{
			continue;
		}
		std::vector<uint8_t> vDefault;
		onPlayerDoMakeCardGroup(nIdx,vDefault);
	}
	return true;
}

bool BJRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
{
	if (GameRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID))
	{
		return true;
	}

	if (MSG_PLAYER_SET_READY == nMsgType)
	{
		auto pPlayer = getPlayerBySessionID(nSessionID);
		Json::Value jsRet;
		if (pPlayer == nullptr)
		{
			jsRet["ret"] = 1;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("you are not in this room how to set ready ? session id = %u", nSessionID);
			return true;
		}

		if (pPlayer->haveState(eRoomPeer_WaitNextGame) == false)
		{
			jsRet["ret"] = 2;
			jsRet["curState"] = pPlayer->getState();
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("player state error uid = %u , state = %u", pPlayer->getUserUID(), pPlayer->getState());
			return true;
		}

		onPlayerReady(pPlayer->getIdx());
		jsRet["ret"] = 0;
		sendMsgToPlayer(jsRet, nMsgType, nSessionID);
		return true;
	}
	return false;
}

bool BJRoom::addPlayerOneRoundOffsetToRecorder(IGamePlayer* pPlayer)
{

}