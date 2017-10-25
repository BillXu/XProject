#include "NNRoom.h"
#include "NiuNiu\NNPlayer.h"
#include <algorithm>
#include "NiuNiu\NNRoomStateWaitReady.h"
#include "NiuNiu\NNRoomStateStartGame.h"
#include "NiuNiu\NNRoomStateGameEnd.h"
#include "NiuNiu\NNRoomPlayerCaculateNiu.h"
#include "NiuNiu\NNRoomStateBet.h"
#include "NiuNiu\NNRoomStateDecideBanker.h"
#include "NiuNiu\NNRoomStateDistributeCard.h"
#include "NiuNiu\NNRoomStateLRBStartGame.h"
#include "NiuNiu\NNRoomStateLRBRobotBanker.h"
#include "NiuNiu\NNRoomStateLRBDistributeFristCard.h"
#include "NiuNiu\NNRoomStateLRBDistributeFinalCard.h"
#include "NiuNiu\NiuNiuPlayerRecorder.h"
bool NNRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	GameRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
	m_nLastNiuNiuIdx = -1;
	m_nBankerIdx = -1;
	m_nBottomTimes = 1;
	m_eDecideBankerType = (eDecideBankerType)vJsOpts["bankType"].asUInt();
	m_eResultType = eResult_Max; // (eResultType)vJsOpts["resultType"].asUInt();
	
	IGameRoomState* pState = new NNRoomStateWaitReady();
	addRoomState(pState);
	setInitState(pState);
	pState = new NNRoomStateGameEnd();
	addRoomState(pState);
	pState = new NNRoomStateCaculateNiu();
	addRoomState(pState);
	pState = new NNRoomStateDoBet();
	addRoomState(pState);
	if ( eDecideBank_LookCardThenRobot != m_eDecideBankerType )
	{
		pState = new NNRoomStateStartGame();
		addRoomState(pState);

		pState = new NNRoomStateDecideBanker();
		addRoomState(pState);

		pState = new NNRoomStateDistributeCard();
		addRoomState(pState);
	}
	else
	{
		pState = new NNRoomStateLRBDistributeFinalCard();
		addRoomState(pState);

		pState = new NNRoomStateLRBDistributeFristCard();
		addRoomState(pState);

		pState = new NNRoomStateLRBRobotBanker();
		addRoomState(pState);

		pState = new NNRoomStateLRBStartGame();
		addRoomState(pState);
	}
	return true;
}

IGamePlayer* NNRoom::createGamePlayer()
{
	auto p = new NNPlayer();
	p->getPlayerCard()->setOpts(m_jsOpts["wuHua"].asUInt() == 1 , m_jsOpts["zhaDan"].asUInt() == 1, m_jsOpts["wuXiao"].asUInt() == 1, m_jsOpts["shunKan"].asUInt() == 1, m_jsOpts["fengKuang"].asUInt() == 1 );
	return p;
}

void NNRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	GameRoom::packRoomInfo(jsRoomInfo);
	jsRoomInfo["bankIdx"] = m_nBankerIdx;
	jsRoomInfo["curActIdex "] = getCurState()->getCurIdx();
	jsRoomInfo["waitTimer "] = getCurState()->getStateDuring();
}

void NNRoom::visitPlayerInfo( IGamePlayer* pPlayer, Json::Value& jsPlayerInfo,uint32_t nVisitorSessionID)
{
	if (!pPlayer)
	{
		return ;
	}

	GameRoom::visitPlayerInfo(pPlayer, jsPlayerInfo,nVisitorSessionID );
	
	if ( pPlayer->haveState(eRoomPeer_CanAct) == false )  // not join this round game ;
	{
		return;
	}

	if (nVisitorSessionID == pPlayer->getSessionID())
	{
		jsPlayerInfo["lastOffset"] = ((NNPlayer*)pPlayer)->getLastOffset();
		jsPlayerInfo["canTuiZhuang"] = ((NNPlayer*)pPlayer)->getRobotBankerFailedTimes() >= 3;
	}
	jsPlayerInfo["betTimes"] = ((NNPlayer*)pPlayer)->getBetTimes();
	bool isSendHoldCard = false;
	auto nCurState = getCurState()->getStateID();
	isSendHoldCard = ( nCurState == eRoomState_CaculateNiu || nCurState == eRoomState_GameEnd || nCurState == eRoomState_DistributeCard );
	if ( m_eDecideBankerType == eDecideBank_LookCardThenRobot )
	{
		isSendHoldCard = nCurState != eRoomSate_WaitReady;
	}
 
	if ( isSendHoldCard )
	{
		Json::Value jsHoldCards;
		auto playerCard = ((NNPlayer*)pPlayer)->getPlayerCard();
		playerCard->toJson(jsHoldCards);
		jsPlayerInfo["cards"] = jsHoldCards;
	}
}

uint8_t NNRoom::getRoomType()
{
	return eGame_NiuNiu;
}

void NNRoom::onStartGame()
{
	GameRoom::onStartGame();
	m_nBottomTimes = 1;
	// add frame ;
}

bool sortPlayerByCard(NNPlayer* pLeft, NNPlayer* pRight)
{
	if (pLeft->getPlayerCard()->pk(pRight->getPlayerCard()) == IPeerCard::PK_RESULT_FAILED)
	{
		return true;
	}
	return false;
}

void NNRoom::onGameEnd()
{
	// caculate result ;
	std::vector<NNPlayer*> vPlayerCardAsc;
	auto nSeatCnt = getSeatCnt();
	for (uint16_t nIdx = 0;  nIdx < nSeatCnt; ++nIdx)
	{
		auto p = (NNPlayer*)getPlayerByIdx(nIdx);
		if (p && p->haveState(eRoomPeer_CanAct))
		{
			vPlayerCardAsc.push_back(p);
		}
	}

	if (vPlayerCardAsc.empty() || vPlayerCardAsc.front()->getPlayerCard()->getHoldCardCnt() != NIUNIU_HOLD_CARD_COUNT)
	{
		LOGFMTW("room id = %u invoker on game end but do not finish distribute card in game end state ,dismissed ? can not do caculate", getRoomID());  // do not caculate result ;
		GameRoom::onGameEnd();
		return;
	}

	std::sort(vPlayerCardAsc.begin(), vPlayerCardAsc.end(), sortPlayerByCard);
	// do caculate ;
	auto pBanker = (NNPlayer*)getPlayerByIdx(m_nBankerIdx);
	if (!pBanker)
	{
		LOGFMTE( "why banker is null ? bug idx = %u , room id = %u",m_nBankerIdx,getRoomID() );
	}
	else
	{
		bool isBankerWin = true;
		for ( auto& ref : vPlayerCardAsc )
		{
			if ( ref->getIdx() == m_nBankerIdx)
			{
				isBankerWin = false;
				continue;
			}
			auto pPlayerCard = ref->getPlayerCard();
			if ( isBankerWin )
			{
				pPlayerCard = pBanker->getPlayerCard();
			}
			auto nBeishu = getBeiShuByCardType(pPlayerCard->getType(),pPlayerCard->getPoint() );
			nBeishu = nBeishu * max(m_nBottomTimes , 1 ) * max(ref->getBetTimes(), 1);
			pBanker->addSingleOffset(nBeishu * ( isBankerWin ? 1 : -1 ) );
			ref->addSingleOffset( nBeishu * ( (false == isBankerWin) ? 1 : -1) );
		}
	}

	// check niuniu be banker ;
	auto pBiggistPlayer = vPlayerCardAsc.back();
	if ( pBiggistPlayer->getIdx() != m_nBankerIdx && pBiggistPlayer->getPlayerCard()->getType() >= CNiuNiuPeerCard::Niu_Niu)
	{
		m_nLastNiuNiuIdx = pBiggistPlayer->getIdx();
	}

	// send game end msg ;
	Json::Value jsResult;
	for (auto& ref : vPlayerCardAsc)
	{
		Json::Value jsPlayer;
		jsPlayer["uid"] = ref->getUserUID();
		jsPlayer["final"] = ref->getChips();
		jsPlayer["offset"] = ref->getSingleOffset();
		jsResult[jsResult.size()] = jsPlayer;
	}

	Json::Value jsMsg;
	jsMsg["bankerIdx"] = m_nBankerIdx;
	jsMsg["result"] = jsResult;
	sendRoomMsg(jsMsg, MSG_ROOM_NIUNIU_GAME_END );

	GameRoom::onGameEnd();
}

bool NNRoom::canStartGame()
{
	if ( false == GameRoom::canStartGame())
	{
		return false;
	}

	uint16_t nReadyCnt = 0;
	for (uint16_t nIdx = 0; nIdx < getSeatCnt(); ++nIdx)
	{
		auto p = getPlayerByIdx(nIdx);
		if ( nullptr == p )
		{
			continue;
		}

		if ( p->haveState(eRoomPeer_Ready) == false )
		{
			return false;
		}

		++nReadyCnt;
	}
	return nReadyCnt >= 2;
}

IPoker* NNRoom::getPoker()
{
	return (IPoker*)&m_tPoker;
}

void NNRoom::onPlayerReady(uint16_t nIdx)
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
	sendRoomMsg(jsMsg, MSG_ROOM_PLAYER_READY );
	LOGFMTD( "do send player set ready idx = %u room id = %u",nIdx,getRoomID() )
}

uint8_t NNRoom::doProduceNewBanker()
{
	switch (m_eDecideBankerType)
	{
	case eDecideBank_Rand:
	{
		m_nBankerIdx = rand() % getSeatCnt();
		for (uint16_t nIdx = m_nBankerIdx; nIdx < (getSeatCnt() * 2); ++nIdx)
		{
			auto nReal = nIdx % getSeatCnt();
			auto p = getPlayerByIdx(nReal);
			if (p)
			{
				m_nBankerIdx = nReal;
				break;
			}
		}
	}
	break;
	case eDecideBank_NiuNiu:
	{
		if ((uint16_t)-1 == m_nBankerIdx)
		{
			m_nBankerIdx = rand() % getSeatCnt();
			for (uint16_t nIdx = m_nBankerIdx; nIdx < (getSeatCnt() * 2); ++nIdx)
			{
				auto nReal = nIdx % getSeatCnt();
				auto p = getPlayerByIdx(nReal);
				if (p)
				{
					m_nBankerIdx = nReal;
					break;
				}
			}
		}

		if (m_nLastNiuNiuIdx != (uint16_t)-1 )
		{
			m_nBankerIdx = m_nLastNiuNiuIdx;
		}
	}
	break;
	case eDecideBank_OneByOne:
	{
		if ((uint16_t)-1 == m_nBankerIdx)
		{
			m_nBankerIdx = rand() % getSeatCnt();
		}
		else
		{
			++m_nBankerIdx ;
		}
		
		for (uint16_t nIdx = m_nBankerIdx; nIdx < (getSeatCnt() * 2); ++nIdx )
		{
			auto nReal = nIdx % getSeatCnt();
			auto p = getPlayerByIdx(nReal);
			if ( p )
			{
				m_nBankerIdx = nReal;
				break;
			}
		}
	}
	break;
	case eDecideBank_LookCardThenRobot:
	{
		auto nSeatCnt = getSeatCnt();
		std::vector<uint16_t> vBankerCandinates;
		uint16_t nBiggistRobotTimes = 0;
		for (auto nIdx = 0; nIdx < nSeatCnt; ++nIdx)
		{
			auto pPlayer = (NNPlayer*)getPlayerByIdx(nIdx);
			if (pPlayer == nullptr || (false == pPlayer->haveState(eRoomPeer_CanAct)) )
			{
				continue;
			}

			auto nRobotTimes = pPlayer->getRobotBankerTimes();
			if (nRobotTimes < nBiggistRobotTimes)
			{
				continue;
			}

			if ( nRobotTimes > nBiggistRobotTimes)
			{
				nBiggistRobotTimes = nRobotTimes;
				vBankerCandinates.clear();
			}
			vBankerCandinates.push_back(nIdx);
		}
		if ( nBiggistRobotTimes != 0 )
		{
			m_nBottomTimes = nBiggistRobotTimes ;
		}
		
		if (vBankerCandinates.empty())
		{
			LOGFMTE("why look card robot banker candinates is empty room id = %u",getRoomID() );
			m_nBankerIdx = 0;
			break;
		}
		m_nBankerIdx = vBankerCandinates[rand()%vBankerCandinates.size()];

		// decide robot banker failed
		for (auto nIdx = 0; nIdx < nSeatCnt && nBiggistRobotTimes > 0 ; ++nIdx )
		{
			auto pPlayer = (NNPlayer*)getPlayerByIdx(nIdx);
			if (pPlayer == nullptr || (false == pPlayer->haveState(eRoomPeer_CanAct)))
			{
				continue;
			}

			auto nRobotTimes = pPlayer->getRobotBankerTimes();
			if ( nRobotTimes != nBiggistRobotTimes || nIdx == m_nBankerIdx )  // must biggest times robot banker , and failed ;
			{
				continue;
			}

			pPlayer->onRobotBankerFailed();
		}
	}
	break;
	default:
		LOGFMTE("invalid decide banker type = %u",m_eDecideBankerType);
		return 0;
	}

	// send msg tell new banker ;
	Json::Value jsMsg;
	jsMsg["bankerIdx"] = m_nBankerIdx;
	sendRoomMsg(jsMsg, MSG_ROOM_PRODUCED_BANKER);
	return (uint8_t)m_nBankerIdx;
}

void NNRoom::doStartBet()
{
	Json::Value js, jsPlayers;
	for (uint8_t nIdx = 0; nIdx < getSeatCnt(); ++nIdx)
	{
		auto p = (NNPlayer*)getPlayerByIdx(nIdx);
		if ( p == nullptr || p->haveState(eRoomPeer_CanAct) == false)
		{
			continue;
		}

		Json::Value jsPlayer;
		jsPlayer["idx"] = p->getIdx();
		jsPlayer["lastOffset"] = p->getLastOffset();
		jsPlayer["canTuiZhuang"] = p->getRobotBankerFailedTimes() >= 3 ? 1 : 0;
		jsPlayers[jsPlayers.size()] = jsPlayer;
	}
	js["players"] = jsPlayers;
	sendRoomMsg(js, MSG_ROOM_START_BET );
}

uint8_t NNRoom::onPlayerDoBet( uint16_t nIdx, uint8_t nBetTimes )
{
	auto p = (NNPlayer*)getPlayerByIdx(nIdx);
	if ( p == nullptr )
	{
		LOGFMTE( "player is null how to do bet ? room id = %u idx = %u",getRoomID(),nIdx );
		return 2;
	}

	if ( p->haveState(eRoomPeer_CanAct) == false )
	{
		LOGFMTE("you are not play this round , can not bet");
		return 3;
	}

	nBetTimes = p->doBet(nBetTimes);

	Json::Value jsRoomBet;
	jsRoomBet["idx"] = nIdx;
	jsRoomBet["betTimes"] = nBetTimes;
	sendRoomMsg(jsRoomBet, MSG_ROOM_DO_BET);
	return 0;
}

bool NNRoom::isAllPlayerDoneBet()
{
	auto nCnt = getSeatCnt();
	for ( uint16_t nIdx = 0; nIdx < nCnt; ++nIdx )
	{
		auto pPlayer = (NNPlayer*)getPlayerByIdx(nIdx);
		if ( nIdx == m_nBankerIdx || pPlayer == nullptr || (false == pPlayer->haveState(eRoomPeer_CanAct)))
		{
			continue;
		}
		if ( pPlayer->getBetTimes() <= 0 )
		{
			return false;
		}
	}
	return true;
}

void NNRoom::doDistributeCard(uint8_t nCardCnt)
{
	Json::Value jsVecPlayers;
	auto nCnt = getSeatCnt();
	for ( auto nIdx = 0; nIdx < nCnt; ++nIdx )
	{
		auto pPlayer = (NNPlayer*)getPlayerByIdx(nIdx);
		if (pPlayer == nullptr || (pPlayer->haveState(eRoomPeer_CanAct) == false))
		{
			continue;
		}
		auto playerCard = pPlayer->getPlayerCard();
		
		// distribute card ;
		for ( auto nCardIdx = 0; nCardIdx < nCardCnt; ++nCardIdx )
		{
			playerCard->addCompositCardNum(getPoker()->distributeOneCard());
		}

		// make disribute msg ;
		Json::Value jsPlayer;
		Json::Value jsHoldCards;
		playerCard->toJson(jsHoldCards);
		jsPlayer["idx"] = nIdx;
		jsPlayer["cards"] = jsHoldCards;
		jsVecPlayers[jsVecPlayers.size()] = jsPlayer;
	}

	Json::Value jsMsg;
	jsMsg["info"] = jsVecPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_DISTRIBUTE_CARD );
}

uint8_t NNRoom::onPlayerDoCacuateNiu( uint16_t nIdx )
{
	auto p = (NNPlayer*)getPlayerByIdx(nIdx);
	if (!p)
	{
		return 1;
	}

	if ( p->haveState(eRoomPeer_CanAct) == false )
	{
		return 2;
	}

	p->doCaculatedNiu();

	Json::Value js;
	js["idx"] = nIdx;
	sendRoomMsg(js, MSG_ROOM_CACULATE_NIU);
	return 0;
}

bool NNRoom::isAllPlayerCacualtedNiu()
{
	auto nSeatCnt = getSeatCnt();
	for (auto nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto pPlayer = (NNPlayer*)getPlayerByIdx(nIdx);
		if ( pPlayer == nullptr)
		{
			continue;
		}

		if (false == pPlayer->haveState(eRoomPeer_CanAct))
		{
			continue;
		}

		if ( false == pPlayer->isCaculatedNiu() )
		{
			return false;
		}
	}

	return true;
}

void NNRoom::doStartRobotBanker()
{
	// send msg infor player robot banker ; 
	Json::Value js;
	sendRoomMsg(js, MSG_ROOM_START_ROBOT_BANKER );
}

uint8_t NNRoom::onPlayerRobotBanker(uint16_t nIdx, uint8_t nRobotTimes )
{
	auto p = (NNPlayer*)getPlayerByIdx(nIdx);
	if (p == nullptr)
	{
		LOGFMTE("player is null how to do bet ? room id = %u idx = %u", getRoomID(), nIdx);
		return 2;
	}

	if (p->haveState(eRoomPeer_CanAct) == false)
	{
		LOGFMTE("you are not play this round , can not bet");
		return 3;
	}

	if ( p->getRobotBankerTimes() > 0 )
	{
		LOGFMTE( "already robot times uid = %u times = %u room id = %u ",p->getUserUID(),p->getRobotBankerTimes(),getRoomID() );
		return 4;
	}

	p->doRobotBanker(nRobotTimes);

	Json::Value jsRoomBet;
	jsRoomBet["idx"] = nIdx;
	jsRoomBet["robotTimes"] = nRobotTimes;
	sendRoomMsg(jsRoomBet, MSG_ROOM_ROBOT_BANKER);
	return 0;
}

bool NNRoom::isAllPlayerRobotedBanker()
{
	auto nSeatCnt = getSeatCnt();
	for (auto nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto pPlayer = (NNPlayer*)getPlayerByIdx(nIdx);
		if (pPlayer == nullptr)
		{
			continue;
		}

		if (false == pPlayer->haveState(eRoomPeer_CanAct))
		{
			continue;
		}

		if ( pPlayer->getRobotBankerTimes() <= 0 )
		{
			return false;
		}
	}

	return true;
}

int16_t NNRoom::getBeiShuByCardType( uint16_t nType, uint16_t nPoint)
{
	switch (nType)
	{
	case CNiuNiuPeerCard::Niu_Single:
	{
		if (nPoint == 7 || 8 == nPoint) { return 2; }
		if (nPoint == 9) { return 3; }
		return 1;
	}
	break;
	case CNiuNiuPeerCard::Niu_Niu:
	{
		return 4;
	};
	break;
	case CNiuNiuPeerCard::Niu_ShunZiNiu:
	case CNiuNiuPeerCard::Niu_TongHuaNiu:
	case CNiuNiuPeerCard::Niu_Hulu:
	case CNiuNiuPeerCard::Niu_FiveFlower:
	{
		return 5;
	}
	break;
	case CNiuNiuPeerCard::Niu_Boom:
	{
		return 6;
	}
	break;
	case CNiuNiuPeerCard::Niu_FiveSmall:
	{
		return 8;
	}
	break;
	case CNiuNiuPeerCard::Niu_TongHuaShun:
	{
		return 10;
	}
	break;
	default:
		//LOGFMTE( "should come here , roomid = %u , type = %u , npoint = %u",getRoomID(),nType,nPoint );
		return 1;
	}

	// old code below ;
	// return ;
	if ( m_eResultType == eResult_NiuNiu4)
	{
		if (nType == CNiuNiuPeerCard::Niu_None)
		{
			return 1;
		}

		if (nType == CNiuNiuPeerCard::Niu_Single)
		{
			if (nPoint >= 1 && nPoint <= 6)
			{
				return 1;
			}

			if (nPoint <= 8)
			{
				return 2;
			}

			if (nPoint == 9)
			{
				return 3;
			}
		}
		else if (CNiuNiuPeerCard::Niu_Niu == nType)
		{
			return 4;
		}
		else if (CNiuNiuPeerCard::Niu_FiveFlower == nType)
		{
			return 5;
		}
		else if (CNiuNiuPeerCard::Niu_Boom == nType)
		{
			return 6;
		}
		else if (CNiuNiuPeerCard::Niu_FiveSmall == nType)
		{
			return 8;
		}
		LOGFMTE( "niu 4 result type : unknown : %u , p = %u",nType,nPoint );
		return 1;
	}
	else if (eReulst_NiuNiu3 == m_eResultType)
	{
		if (nType == CNiuNiuPeerCard::Niu_None)
		{
			return 1;
		}

		if (nType == CNiuNiuPeerCard::Niu_Single)
		{
			if (nPoint >= 1 && nPoint <= 7)
			{
				return 1;
			}

			if (nPoint <= 9)
			{
				return 2;
			}
		}
		else if (CNiuNiuPeerCard::Niu_Niu == nType)
		{
			return 3;
		}
		else if (CNiuNiuPeerCard::Niu_FiveFlower == nType)
		{
			return 5;
		}
		else if (CNiuNiuPeerCard::Niu_Boom == nType)
		{
			return 6;
		}
		else if (CNiuNiuPeerCard::Niu_FiveSmall == nType)
		{
			return 8;
		}
		LOGFMTE("niu 3 result type : unknown : %u , p = %u", nType, nPoint);
		return 1;
	}
	LOGFMTE( "unknow result = %u , room id = %u",m_eResultType,getRoomID() );
	return 1;
}

bool NNRoom::addPlayerOneRoundOffsetToRecorder(IGamePlayer* pPlayer)
{
	if (pPlayer == nullptr || pPlayer->haveState( eRoomPeer_StayThisRound) == false )
	{
		LOGFMTE("player is null how to add recorder room id = %u", getRoomID());
		return false;
	}

	auto pCurRecorder = getCurRoundRecorder();
	if (pCurRecorder == nullptr)
	{
		LOGFMTE("why this room id = %u single recorder is null ?  can not add uid = %u offset = %d", getRoomID(), pPlayer->getUserUID(), pPlayer->getSingleOffset());
	}
	else
	{
		auto pNiu = (NNPlayer*)pPlayer;
		auto ptr = std::make_shared<NiuNiuPlayerRecorder>(pPlayer->getUserUID(), pPlayer->getSingleOffset());
		ptr->nBetTimes = pNiu->getBetTimes();
		if (pNiu->getIdx() == getBankerIdx())
		{
			ptr->nBetTimes = 0;
		}
		auto pCards = pNiu->getPlayerCard();
		for (uint8_t nIdx = 0; nIdx < NIUNIU_HOLD_CARD_COUNT; ++nIdx)
		{
			ptr->vCards[nIdx] = pCards->getCardByIdx(nIdx);
		}
		pCurRecorder->addPlayerRecorderInfo(ptr);
	}
	return true;
}

bool NNRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
{
	if ( GameRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID) )
	{
		return true;
	}

	if ( MSG_PLAYER_SET_READY == nMsgType )
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

bool NNRoom::isEnableTuiZhu()
{
	return m_jsOpts["tuiZhu"].isNull() == false && m_jsOpts["tuiZhu"].asUInt() == 1;
}

bool NNRoom::isEnableTuiZhuang()
{
	if (m_eDecideBankerType != eDecideBank_LookCardThenRobot)
	{
		return true;
	}

	return m_jsOpts["tuiZhuang"].isNull() == false && m_jsOpts["tuiZhuang"].asUInt() == 1;
}

void NNRoom::onTimeOutPlayerAutoBet()
{
	for (uint8_t nIdx = 0; nIdx < getSeatCnt(); ++nIdx)
	{
		auto pPlayer = (NNPlayer*)getPlayerByIdx(nIdx);
		if ( getBankerIdx() == nIdx || nullptr == pPlayer || pPlayer->haveState(eRoomPeer_CanAct) != false)
		{
			continue;
		}

		if ( pPlayer->getBetTimes() == 0)
		{
			onPlayerDoBet(nIdx, 1);
		}
	}
}