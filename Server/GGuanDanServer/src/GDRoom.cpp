#include "GDRoom.h"
#include "GDPlayer.h"
#include "GDRoomStateWaitReady.h"
#include "GDRoomStatePlayerChu.h"
#include "GDRoomStateStartGame.h"
#include "GDRoomStateGameEnd.h"
#include "GDPlayer.h"
#include "GDRoomStatePayTribute.h"
#include "GDRoomStateBackTribute.h"
#include "GDRoomManager.h"
#include "IGameRoomDelegate.h"
#include "GDOpts.h"
bool GDRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)
{
	GameRoom::init(pRoomMgr, nSeialNum, nRoomID, ptrGameOpts);
	m_stLastGameInfo.clear();
	m_nFirstChu = 0;
	m_vDaJi[0] = 2;
	m_vDaJi[1] = 2;
	m_nDaJi = 2;

	IGameRoomState* pState = nullptr;
	pState = new GDRoomStateWaitReady();
	addRoomState(pState);
	setInitState(pState);
	
	pState = new GDRoomStatePayTribute();
	addRoomState(pState);
	pState = new GDRoomStateBackTribute();
	addRoomState(pState);
	pState = new GDRoomStatePlayerChu();
	addRoomState(pState);
	pState = new GDRoomStateStartGame();
	addRoomState(pState);
	pState = new GDRoomStateGameEnd();
	addRoomState(pState);
	return true;
}

IGamePlayer* GDRoom::createGamePlayer()
{
	return new GDPlayer();
}

void GDRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	GameRoom::packRoomInfo(jsRoomInfo);
	/*Json::Value jsDiPai;
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
	jsRoomInfo["bottom"] = getBankTimes();*/
}

void GDRoom::visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)
{
	GameRoom::visitPlayerInfo(pPlayer,jsPlayerInfo,nVisitorSessionID);
	auto p = (GDPlayer*)pPlayer;
	jsPlayerInfo["holdCardCnt"] = p->getPlayerCard()->getHoldCardCount();
	if (nVisitorSessionID == pPlayer->getSessionID())
	{
		Json::Value jsHoldCards;
		p->getPlayerCard()->holdCardToJson(jsHoldCards);
		jsPlayerInfo["holdCards"] = jsHoldCards;
	}
	jsPlayerInfo["extraTime"] = p->getExtraTime();
}

uint8_t GDRoom::getRoomType()
{
	return getDelegate()->getOpts()->getGameType();
}

void GDRoom::onWillStartGame()
{
	GameRoom::onWillStartGame();
	confirmDaJi();
}

void GDRoom::onStartGame()
{
	GameRoom::onStartGame();

	// distribute card 
	auto nSeatCnt = getSeatCnt();
	for (uint8_t nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto p = (GDPlayer*)getPlayerByIdx(nIdx);
		if (p == nullptr || (p->haveState(eRoomPeer_CanAct) == false))
		{
			continue;
		}

		uint8_t nCardCnt = 27;
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
		auto p = (GDPlayer*)getPlayerByIdx(nIdx);
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
		auto p = (GDPlayer*)getPlayerByIdx(nIdx);
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
	addReplayFrame(GD_Frame_StartGame, jsFrame);
}

void GDRoom::onGameEnd()
{
	GameRoom::onGameEnd();
}

bool GDRoom::canStartGame()
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

IPoker* GDRoom::getPoker()
{
	return &m_tPoker;
}

bool GDRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
{
	if (GameRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID))
	{
		return true;
	}
	return false;
}

bool GDRoom::isCanDouble()
{
	auto pGDOpts = std::dynamic_pointer_cast<GDOpts>(getDelegate()->getOpts());
	return pGDOpts->isEnableDouble();
}

uint8_t GDRoom::getBaseScore()
{
	auto pGDOpts = std::dynamic_pointer_cast<GDOpts>(getDelegate()->getOpts());
	return pGDOpts->getBaseScore();
}

bool GDRoom::isEnableRandomSeat() {
	auto pGDOpts = std::dynamic_pointer_cast<GDOpts>(getDelegate()->getOpts());
	return pGDOpts->isEnableRandomSeat();
}

bool GDRoom::isEnableRandomDa() {
	auto pGDOpts = std::dynamic_pointer_cast<GDOpts>(getDelegate()->getOpts());
	return pGDOpts->isEnableRandomDa();
}

uint8_t GDRoom::checkPlayerCanSitDown(stEnterRoomData* pEnterRoomPlayer)
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

void GDRoom::doRandomChangeSeat() {
	std::vector<uint16_t> vChanged;
	for (uint16_t i = 0; i < getSeatCnt(); i++) {
		if (std::find(vChanged.begin(), vChanged.end(), i) == vChanged.end()) {
			vChanged.push_back(i);
			uint16_t nSwitchIdx = rand() % getSeatCnt();
			if (std::find(vChanged.begin(), vChanged.end(), nSwitchIdx) == vChanged.end()) {
				vChanged.push_back(nSwitchIdx);
				if (doChangeSeat(i, nSwitchIdx) == false) {
					break;
				}
			}
		}
	}
	Json::Value jsMsg, jsPlayersEnd, jsPlayerEnd;
	for (auto ref : m_vPlayers) {
		jsPlayerEnd["idx"] = ref->getIdx();
		jsPlayerEnd["uid"] = ref->getUserUID();
		jsPlayersEnd[jsPlayersEnd.size()] = jsPlayerEnd;
	}
	jsMsg["detail"] = jsPlayersEnd;
	sendRoomMsg(jsMsg, MSG_ROOM_PLAYER_EXCHANGE_SEAT);
}

bool GDRoom::doChangeSeat(uint16_t nIdx, uint16_t nWithIdx) {
	auto pPlayer = getPlayerByIdx(nIdx);
	auto sPlayer = getPlayerByIdx(nWithIdx);
	if (pPlayer == nullptr || sPlayer == nullptr) {
		return false;
	}
	sPlayer->setIdx(nIdx);
	pPlayer->setIdx(nWithIdx);
	m_vPlayers[nIdx] = sPlayer;
	m_vPlayers[nWithIdx] = pPlayer;
	return true;
}

void GDRoom::confirmDaJi() {
	if (isEnableRandomDa()) {
		m_stLastGameInfo.clear();
		if (isEnableRandomSeat()) {
			doRandomChangeSeat();
		}

		m_nDaJi = rand() % 13 + 1;
		if (m_nDaJi = 1) {
			m_nDaJi = 14;
		}
	}
	else {
		if (m_stLastGameInfo.isSet()) {
			auto n1UID = m_stLastGameInfo.get1You();
			auto pPlayer = getPlayerByUID(n1UID);
			if (pPlayer) {
				auto nIdx = pPlayer->getIdx();
				m_nDaJi = m_vDaJi[nIdx % 2];
			}
			else {
				LOGFMTE("GDRoom = %u is already set last game info but can not confirm tuo you = %u!", getRoomID(), n1UID);
				m_nDaJi = 2;
			}
		}
		else {
			m_nDaJi = 2;
		}
	}

	Json::Value jsMsg;
	jsMsg["daJi"] = m_nDaJi;
	sendRoomMsg(jsMsg, MSG_GD_CONFIRM_DAJI);

	//add replay frame
	addReplayFrame(GD_Frame_Daji, jsMsg);
}

bool GDRoom::onWaitPayTribute(std::vector<uint8_t>& vWaitIdx) {
	vWaitIdx.clear();
	if (m_stLastGameInfo.isSet()) {
		auto n1You = m_stLastGameInfo.get1You();
		auto n2You = m_stLastGameInfo.get2You();
		auto p1YouPlayer = getPlayerByUID(n1You);
		auto p2YouPlayer = getPlayerByUID(n2You);
		if (p1YouPlayer == nullptr || p2YouPlayer == nullptr) {
			return false;
		}
		auto n1YouIdx = p1YouPlayer->getIdx();
		auto n2YouIdx = p2YouPlayer->getIdx();
		if (n1YouIdx % 2 == n2YouIdx % 2) {
			auto nSeatCnt = getSeatCnt();
			for (uint8_t idx = 0; idx < nSeatCnt; idx++) {
				if (idx == n1YouIdx || idx == n2YouIdx) {
					continue;
				}
				vWaitIdx.push_back(idx);
			}
		}
		else {
			auto n4You = m_stLastGameInfo.get4You();
			auto p4YouPlayer = getPlayerByUID(n4You);
			if (p4YouPlayer == nullptr) {
				return false;
			}
			vWaitIdx.push_back(p4YouPlayer->getIdx());
		}
		Json::Value jsMsg, jsIdx;
		for (auto idx : vWaitIdx) {
			jsIdx[jsIdx.size()] = idx;
		}
		jsMsg["idx"] = jsIdx;
		sendRoomMsg(jsMsg, MSG_GD_WAIT_PAYTRIBUTE);
		return true;
	}
	return false;
}

void GDRoom::doPayTribute(std::map<uint8_t, uint8_t>& mDoIdx, std::map<uint8_t, uint8_t>& mBackInfo) {
	mBackInfo.clear();

	if (mDoIdx.size() == 0) {
		return;
	}

	if (mDoIdx.size() == 1) {
		auto pPayPlayer = (GDPlayer*)getPlayerByIdx(mDoIdx.begin()->first);
		auto n1You = m_stLastGameInfo.get1You();
		auto p1YouPlayer = (GDPlayer*)getPlayerByUID(n1You);
		if (pPayPlayer == nullptr || p1YouPlayer == nullptr) {
			return;
		}
		auto pPayCard = pPayPlayer->getPlayerCard();
		if (pPayCard->onPayCard(mDoIdx.begin()->second)) {
			auto p1YouCard = p1YouPlayer->getPlayerCard();
			p1YouCard->addHoldCard(mDoIdx.begin()->second);
			mBackInfo[p1YouPlayer->getIdx()] = mDoIdx.begin()->first;
			m_nFirstChu = mDoIdx.begin()->first;

			Json::Value jsMsg, jsInfos, jsInfo;
			jsInfo["idx"] = mDoIdx.begin()->first;
			jsInfo["target"] = p1YouPlayer->getIdx();
			jsInfo["card"] = mDoIdx.begin()->second;
			jsInfos[jsInfos.size()] = jsInfo;
			jsMsg["info"] = jsInfos;
			sendRoomMsg(jsMsg, MSG_GD_DO_PAYTRIBUTE);

			// add frame 
			addReplayFrame(GD_Frame_PayTribute, jsMsg);
		}
	}
	else if(mDoIdx.size() == 2) {
		std::sort(mDoIdx.begin(), mDoIdx.end(), [](const std::pair<uint8_t, uint8_t>& a, const std::pair<uint8_t, uint8_t>& b) {
			return a.second > b.second;
		});
		uint32_t nTargetUID = 0;
		Json::Value jsMsg, jsInfos, jsInfo;
		for (auto& ref : mDoIdx) {
			if (nTargetUID) {
				nTargetUID = m_stLastGameInfo.get2You();
			}
			else {
				nTargetUID = m_stLastGameInfo.get1You();
			}

			auto pPayPlayer = (GDPlayer*)getPlayerByIdx(ref.first);
			auto pTargetPlayer = (GDPlayer*)getPlayerByUID(nTargetUID);
			if (pTargetPlayer == nullptr || pPayPlayer == nullptr) {
				continue;
			}
			auto pPayCard = pPayPlayer->getPlayerCard();
			if (pPayCard->onPayCard(ref.second)) {
				auto pTargetCard = pTargetPlayer->getPlayerCard();
				pTargetCard->addHoldCard(ref.second);
				mBackInfo[pTargetPlayer->getIdx()] = ref.first;

				jsInfo["idx"] = ref.first;
				jsInfo["target"] = pTargetPlayer->getIdx();
				jsInfo["card"] = ref.second;
				jsInfos[jsInfos.size()] = jsInfo;
			}
		}

		if (mBackInfo.size() == 2) {
			m_nFirstChu = mDoIdx.begin()->first;
			jsMsg["info"] = jsInfos;
			sendRoomMsg(jsMsg, MSG_GD_DO_PAYTRIBUTE);

			// add frame 
			addReplayFrame(GD_Frame_PayTribute, jsMsg);
		}
	}
}