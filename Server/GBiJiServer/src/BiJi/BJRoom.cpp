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
#include "BJPlayerRecorder.h"
#include "BJOpts.h"
#include "IGameRoomDelegate.h"
bool BJRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)
{
	GameRoom::init(pRoomMgr,nSeialNum,nRoomID,ptrGameOpts);

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
		if ( p->haveState(eRoomPeer_DoMakedCardGroup))
		{
			p->getPlayerCard()->groupCardToJson(jsHoldCards);
		}
		else if ( p->haveState(eRoomPeer_CanAct) )
		{
			p->getPlayerCard()->holdCardToJson(jsHoldCards);
		}
		
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
		if ( p && p->haveState(eRoomPeer_StayThisRound) )
		{
			vActivePlayers.push_back((BJPlayer*)p);
		}
	}

	// caculate per guo ,
	bool isAllGiveUp = true;
	for (uint8_t nGuoIdx = 0; nGuoIdx < 3; ++nGuoIdx)
	{
		for ( auto& p : vActivePlayers)
		{
			if ( p->haveState(eRoomPeer_GiveUp) == false )
			{
				isAllGiveUp = false;
			}
			p->getPlayerCard()->setCurGroupIdx(nGuoIdx);
		}
		
		if (isAllGiveUp)
		{
			break;
		}

		std::sort(vActivePlayers.begin(), vActivePlayers.end(), []( BJPlayer* pLeft , BJPlayer* pRight )
		{
			auto nLeft = pLeft->getPlayerCard()->getWeight();
			auto nRight = pRight->getPlayerCard()->getWeight();
			if (pLeft->haveState(eRoomPeer_GiveUp))
			{
				nLeft = 0;
			}

			if (pRight->haveState(eRoomPeer_GiveUp))
			{
				nRight = 0;
			}
			return nLeft < nRight;
		});

		// caculate fen shu ;
		int32_t nWinerPos = vActivePlayers.size() - 1;
		for ( int32_t nLoserPos = 0; nLoserPos < nWinerPos; ++nLoserPos)
		{
			int32_t nWin = nWinerPos - nLoserPos;
			if (vActivePlayers[nLoserPos]->haveState(eRoomPeer_GiveUp))
			{
				nWin = vActivePlayers.size() - 1;
			}

			nWin *= getRoomRate();
			vActivePlayers[nWinerPos]->addOffsetPerGuo(nGuoIdx, nWin);
			vActivePlayers[nLoserPos]->addOffsetPerGuo(nGuoIdx, nWin * -1 );
		}
	}

	// caculate xi pai 
	for ( uint8_t nCheckIdx = 0; nCheckIdx < vActivePlayers.size() && ( false == isAllGiveUp ) ; ++nCheckIdx )
	{
		std::vector<eXiPaiType> vXiType;
		auto nXiPaiRate = vActivePlayers[nCheckIdx]->getPlayerCard()->getXiPaiType(isEnableSanQing(),isEnableShunQingDaTou() ,vXiType );
		if ( 0 == nXiPaiRate )
		{
			continue;
		}

		int32_t nWinPerLose = (vActivePlayers.size() - 1) * nXiPaiRate * getRoomRate();
		uint32_t nTotalWin = 0;
		// caculate xiPai 
		for (uint8_t nIdx = 0; nIdx < vActivePlayers.size(); ++nIdx)
		{
			if (nCheckIdx == nIdx || vActivePlayers[nIdx]->haveState(eRoomPeer_GiveUp) )
			{
				continue;
			}
			nTotalWin += nWinPerLose;
			vActivePlayers[nIdx]->addXiPaiOffset(nWinPerLose * -1 );
		}

		vActivePlayers[nCheckIdx]->addXiPaiOffset((int32_t)nTotalWin);
	}

	// caculate tong guan ;
	if ( vActivePlayers.back()->isTongGuan() )
	{
		int32_t nWinPerLose = (vActivePlayers.size() - 1) * getRoomRate();
		uint32_t nTotalWin = 0;
		// caculate xiPai 
		for (uint8_t nIdx = 0; nIdx < vActivePlayers.size() - 1 ; ++nIdx)
		{
			if ( vActivePlayers[nIdx]->haveState(eRoomPeer_GiveUp) )
			{
				continue;
			}
			nTotalWin += nWinPerLose;
			vActivePlayers[nIdx]->addTongGuanOffset(nWinPerLose * -1);
		}
		vActivePlayers.back()->addTongGuanOffset(nTotalWin);
	}

	// send game result msg ; 
	Json::Value jsArrayPlayers;
	for (auto& p : vActivePlayers)
	{
		Json::Value jsPlayerResult;
		jsPlayerResult["idx"] = p->getIdx();
		jsPlayerResult["offset"] = p->getSingleOffset();
		jsPlayerResult["xiPaiOffset"] = p->getXiPaiOffset();
		std::vector<eXiPaiType> vXiType;
		if ( (false == isAllGiveUp) && 0 != p->getPlayerCard()->getXiPaiType(isEnableSanQing(), isEnableShunQingDaTou(), vXiType))
		{
			Json::Value jsXiPais;
			for (auto& ref : vXiType)
			{
				jsXiPais[jsXiPais.size()] = ref;
			}

			jsPlayerResult["xiPaiTypes"] = jsXiPais;
		}
		
		jsPlayerResult["tongGuanOffset"] = p->getTongGuanOffset();

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

		if (false == pPlayer->haveState(eRoomPeer_DoMakedCardGroup) && false == pPlayer->haveState( eRoomPeer_GiveUp) )
		{
			return false;
		}
	}
	return true;
}

std::shared_ptr<IPlayerRecorder> BJRoom::createPlayerRecorderPtr()
{
	return std::make_shared<BJPlayerRecorder>();
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
	if ( !pPlayerCard->setCardsGroup(vGroupCards,isTianJiSaiMa()))
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
		LOGFMTE("we do not support auto make group room id = %u, uid = %u",getRoomID(),p->getUserUID() );
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

uint8_t BJRoom::getRoomRate() {
	auto pBJOpts = std::dynamic_pointer_cast<BJOpts>(getDelegate()->getOpts());
	return pBJOpts->getRate();
}

bool BJRoom::isEnableSanQing() {
	auto pBJOpts = std::dynamic_pointer_cast<BJOpts>(getDelegate()->getOpts());
	return pBJOpts->isEnableSanQing();
}

bool BJRoom::isEnableShunQingDaTou() {
	auto pBJOpts = std::dynamic_pointer_cast<BJOpts>(getDelegate()->getOpts());
	return pBJOpts->isEnableShunQingDaTou();
}

bool BJRoom::isTianJiSaiMa() {
	auto pBJOpts = std::dynamic_pointer_cast<BJOpts>(getDelegate()->getOpts());
	return pBJOpts->isTianJiSaiMa();
}

bool BJRoom::isEnableGiveUp() {
	auto pBJOpts = std::dynamic_pointer_cast<BJOpts>(getDelegate()->getOpts());
	return pBJOpts->isEnableGiveUp();
}