#include "DDZRoom.h"
#include "DDZPlayer.h"
#include "DDZRoomStateWaitReady.h"
#include "DDZRoomStatePlayerChu.h"
#include "DDZRoomStateStartGame.h"
#include "DDZRoomStateRobotBanker.h"
#include "DDZRoomStateGameEnd.h"
#include "DDZPlayer.h"
#include "JJDDZRoomStateChaoZhuang.h"
#include "JJDDZRoomStateTuiLaChuai.h"
#include "JJDDZRoomStateWaitReadyChaoZhuangMode.h"
#include "DDZRoomStateDouble.h"
#include "DDZRoomManager.h"
#include "IGameRoomDelegate.h"
bool DDZRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	GameRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
	m_nFirstRobotBankerIdx = rand() % getSeatCnt();
	m_vDiPai.clear();
	m_nBankerIdx = 0;
	m_nBankerTimes = 0;
	m_nBombCnt = 0;

	IGameRoomState* pState = nullptr;
	if (vJsOpts["isChaoZhuang"].isNull() == false && vJsOpts["isChaoZhuang"].asUInt() == 1)
	{
		pState = new JJDDZRoomStateWaitReadyChaoZhuangMode();
		addRoomState(pState);
		setInitState(pState);

		pState = new JJDDZRoomStateChaoZhuang();
		addRoomState(pState);
	}
	else
	{
		pState = new DDZRoomStateWaitReady();
		addRoomState(pState);
		setInitState(pState);
	}
	
	pState = new DDZRoomStatePlayerChu();
	addRoomState(pState);
	pState = new DDZRoomStateStartGame();
	addRoomState(pState);
	pState = new DDZRoomStateRobotBanker();
	addRoomState(pState);
	pState = new DDZRoomStateGameEnd();
	addRoomState(pState);
	pState = new JJDDZRoomStateTiLaChuai();
	addRoomState(pState);
	pState = new DDZRoomStateDouble();
	addRoomState(pState);
	return true;
}

IGamePlayer* DDZRoom::createGamePlayer()
{
	return new DDZPlayer();
}

void DDZRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	GameRoom::packRoomInfo(jsRoomInfo);
	Json::Value jsDiPai;
	for (auto& ref : m_vDiPai)
	{
		jsDiPai[jsDiPai.size()] = ref;
	}

	if (jsDiPai.size() > 0)
	{
		jsRoomInfo["diPai"] = jsDiPai;
	}
	jsRoomInfo["dzIdx"] = getBankerIdx();
	jsRoomInfo["bombCnt"] = getBombCount();
	jsRoomInfo["bottom"] = getBankTimes();
}

void DDZRoom::visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)
{
	GameRoom::visitPlayerInfo(pPlayer,jsPlayerInfo,nVisitorSessionID);
	bool isStateShowCards = eRoomState_DDZ_Chu == getCurState()->getStateID() || eRoomState_JJ_DDZ_Ti_La_Chuai == getCurState()->getStateID() || eRoomState_RobotBanker == getCurState()->getStateID() || eRoomState_DDZ_Double == getCurState()->getStateID();
	auto p = (DDZPlayer*)pPlayer;
	if ( isStateShowCards )
	{
		jsPlayerInfo["holdCardCnt"] = p->getPlayerCard()->getHoldCardCount();
		if ( nVisitorSessionID == pPlayer->getSessionID())
		{
			Json::Value jsHoldCards;
			p->getPlayerCard()->holdCardToJson(jsHoldCards);
			jsPlayerInfo["holdCards"] = jsHoldCards;
		}
	}

	if (p->isChaoZhuang())
	{
		jsPlayerInfo["nJiaBei"] = 1;
	}

	if (p->isTiLaChuai())
	{
		jsPlayerInfo["isTiLaChuai"] = 1;
	}

	jsPlayerInfo["double"] = p->getDouble();
}

uint8_t DDZRoom::getRoomType()
{
	if (m_jsOpts["gameType"].isNull())
	{
		LOGFMTE("do not have game type key ");
		return eGame_Max;
	}
	return m_jsOpts["gameType"].asUInt();
}

void DDZRoom::onWillStartGame()
{
	m_vNotShuffleCards.clear();
	GameRoom::onWillStartGame();
}

void DDZRoom::onStartGame()
{
	GameRoom::onStartGame();
	initCardsWithNotShuffleCards();
	m_vDiPai.clear();
	m_nBankerIdx = 0;
	m_nBankerTimes = 0;
	m_nBombCnt = 0;

	// distribute card 
	auto nSeatCnt = getSeatCnt();
	for (uint8_t nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto p = (DDZPlayer*)getPlayerByIdx(nIdx);
		if (p == nullptr || (p->haveState(eRoomPeer_CanAct) == false))
		{
			continue;
		}

		uint8_t nCardCnt = 17;
		while (nCardCnt--)
		{
			p->getPlayerCard()->addHoldCard(getPoker()->distributeOneCard());
		}
	}

	// send start game msg
	Json::Value jsMsg, jsMsgStand, jsStandCards;
	for (uint8_t nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		Json::Value jsStandCardsInfo;
		auto p = (DDZPlayer*)getPlayerByIdx(nIdx);
		if (p == nullptr || (p->haveState(eRoomPeer_CanAct) == false))
		{
			continue;
		}

		Json::Value jsVHoldCard;
		if (p->haveState(eRoomPeer_CanAct))
		{
			p->getPlayerCard()->holdCardToJson(jsVHoldCard);
		}
		jsMsg["vSelfCard"] = jsVHoldCard;
		jsStandCardsInfo["idx"] = p->getIdx();
		jsStandCardsInfo["cnt"] = jsVHoldCard.size();
		jsStandCards[jsStandCards.size()] = jsStandCardsInfo;
		sendMsgToPlayer(jsMsg, MSG_ROOM_DDZ_START_GAME, p->getSessionID());
	}
	jsMsgStand["playerCards"] = jsStandCards;
	sendRoomMsgToStander(jsMsgStand, MSG_ROOM_DDZ_START_GAME);

	// add replay frame 
	Json::Value jsFrame;
	for (uint8_t nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto p = (DDZPlayer*)getPlayerByIdx(nIdx);
		if (p == nullptr || (p->haveState(eRoomPeer_CanAct) == false))
		{
			continue;
		}

		Json::Value jsVHoldCard;
		if (p->haveState(eRoomPeer_CanAct))
		{
			p->getPlayerCard()->holdCardToJson(jsVHoldCard);
		}

		Json::Value jsPlayers;
		jsPlayers["idx"] = nIdx;
		jsPlayers["uid"] = p->getUserUID();
		jsPlayers["cards"] = jsVHoldCard;
		jsFrame[jsFrame.size()] = jsPlayers;
	}
	addReplayFrame(DDZ_Frame_StartGame, jsFrame);
}

void DDZRoom::onGameEnd()
{
	for (auto ref : m_vPlayers) {
		if (ref && ref->haveState(eRoomPeer_StayThisRound)) {
			auto pCard = ((DDZPlayer*)ref)->getPlayerCard();
			if (pCard->getHoldCardCount()) {
				std::vector<uint8_t> vHoldCards;
				pCard->getHoldCard(vHoldCards);
				m_vNotShuffleCards.insert(m_vNotShuffleCards.end(), vHoldCards.begin(), vHoldCards.end());
			}
		}
	}

	((DDZRoomManager*)getRoomMgr())->addNotShuffleCards(m_vNotShuffleCards);

	GameRoom::onGameEnd();
}

bool DDZRoom::canStartGame()
{
	if (GameRoom::canStartGame() == false)
	{
		return false;
	}

	uint8_t nReadyCnt = 0;
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
	return nReadyCnt >= getSeatCnt();
}

IPoker* DDZRoom::getPoker()
{
	return &m_tPoker;
}

bool DDZRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
{
	if (GameRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID))
	{
		return true;
	}
	return false;
}

uint8_t DDZRoom::getFirstRobotBankerIdx()
{
	if (isCYDDZ()) {
		return m_nFirstRobotBankerIdx;
	}
	return m_nFirstRobotBankerIdx = (++m_nFirstRobotBankerIdx) % getSeatCnt();
}

bool DDZRoom::setFirstRotBankerIdx(uint8_t nIdx) {
	if (isCYDDZ()) {
		if (getPlayerByIdx(nIdx) == false) {
			return false;
		}
		m_nFirstRobotBankerIdx = nIdx;
	}
	return true;
}

void DDZRoom::setNewBankerInfo(uint8_t nBankerIdx, uint8_t nBankerTimes, std::vector<uint8_t>& vDiPai)
{
	m_nBankerIdx = nBankerIdx;
	m_nBankerTimes = nBankerTimes;
	m_vDiPai = vDiPai;
}

uint8_t DDZRoom::getBankTimes()
{
	return m_nBankerTimes;
}

uint8_t DDZRoom::getBombCount()
{
	return m_nBombCnt;
}

void DDZRoom::increaseBombCount()
{
	++m_nBombCnt;
}

uint32_t DDZRoom::fengDing()
{
	if (m_jsOpts["maxBet"].isNull() == false && m_jsOpts["maxBet"].isUInt() == false )
	{
		return 0; // not limit 
	}
	return m_jsOpts["maxBet"].asUInt();
}

DDZ_RotLandlordType DDZRoom::getRLT()
{
	if (m_jsOpts["rlt"].isUInt())
	{
		uint8_t nRLT = m_jsOpts["rlt"].asUInt();
		if (nRLT < eFALRLT_Max) {
			return (DDZ_RotLandlordType)nRLT;
		}
	}
	return eFALRLT_Call;
}

bool DDZRoom::isCYDDZ()
{
	return eGame_CYDouDiZhu == getRoomType();
}

bool DDZRoom::isCanDouble()
{
	return m_jsOpts["double"].asBool();
}

uint8_t DDZRoom::getBaseScore()
{
	if (m_jsOpts["baseScore"].isUInt()) {
		uint8_t nBaseScore = m_jsOpts["baseScore"].asUInt();
		if (nBaseScore == 0 || nBaseScore > 100) {
			nBaseScore = 1;
		}
		return nBaseScore;
	}
	return 1;
}

uint32_t DDZRoom::getFanLimit()
{
	return m_jsOpts["fanLimit"].isUInt() ? m_jsOpts["fanLimit"].asUInt() : 0;
}

bool DDZRoom::isNotShuffle()
{
	return m_jsOpts["notShuffle"].asBool();
}

void DDZRoom::addNotShuffleCards(std::vector<uint8_t>& vCards)
{
	m_vNotShuffleCards.insert(m_vNotShuffleCards.end(), vCards.begin(), vCards.end());
}

void DDZRoom::getNotShuffleCards(std::vector<uint8_t>& vCards)
{
	vCards.insert(vCards.end(), m_vNotShuffleCards.begin(), m_vNotShuffleCards.end());
}

void DDZRoom::initCardsWithNotShuffleCards()
{
	if (isNotShuffle()) {
		std::vector<uint8_t> vCards;
		((DDZRoomManager*)getRoomMgr())->getNotShuffleCards(vCards);
		if (vCards.size() == 54) {
			((CDouDiZhuPoker*)getPoker())->initAllCardWithCards(vCards);
		}
	}
}

uint8_t DDZRoom::checkPlayerCanSitDown(stEnterRoomData* pEnterRoomPlayer)
{
	auto p = getPlayerByUID(pEnterRoomPlayer->nUserUID);
	if (p)
	{
		LOGFMTD("already in this room id = %u , uid = %u , so can not sit down", getRoomID(), p->getUserUID());
		return 1;
	}

	if (pEnterRoomPlayer->nDiamond < getDelegate()->getSitDownDiamondConsume())
	{
		// diamond is not enough 
		return 8;
	}

	return 0;
}