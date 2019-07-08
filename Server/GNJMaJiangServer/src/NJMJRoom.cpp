#include "NJMJRoom.h"
#include "NJMJPlayer.h"
#include "CommonDefine.h"
#include "NJMJRoomStateWaitPlayerChu.h"
#include "NJMJRoomStateWaitPlayerAct.h"
#include "NJMJRoomStateStartGame.h"
#include "MJRoomStateGameEnd.h"
#include "NJMJRoomStateAskForRobotGang.h"
#include "NJMJRoomStateAskForPengOrHu.h"
#include "MJRoomStateWaitReady.h"
#include "MJRoomStateDoPlayerAct.h"
#include "MJRoomStateAutoBuHua.h"
#include "FanxingChecker.h"
#include "Fanxing7Dui.h"
#include "FanxingDuiDuiHu.h"
#include "MJReplayFrameType.h"
#include "NJMJPoker.h"
#include "IGameRoomDelegate.h"
#include "NJMJOpts.h"

bool NJMJRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)
{
	IMJRoom::init(pRoomMgr,nSeialNum,nRoomID,ptrGameOpts);
	m_cFanxingChecker.init();
	clearOneCircleEnd();
	m_cFollowCards.setSeatCnt(ptrGameOpts->getSeatCnt());
	m_bWillFanBei = false;
	m_bFanBei = false;
	m_nLeiBaTaCnt = 0;
	m_nGangCnt = 0;
	// add room state ;
	IGameRoomState* p[] = { new CMJRoomStateWaitReady(), new NJMJRoomStateWaitPlayerChu(),new NJMJRoomStateWaitPlayerAct(),
		new NJMJRoomStateStartGame(),new MJRoomStateGameEnd(),new MJRoomStateDoPlayerAct(),new NJMJRoomStateAskForRobotGang(),
		new NJMJRoomStateAskForPengOrHu(), new MJRoomStateAutoBuHua()
	};
	for ( auto& pS : p )
	{
		addRoomState(pS);
	}
	setInitState(p[0]);
	return true;
}

IGamePlayer* NJMJRoom::createGamePlayer()
{
	return new NJMJPlayer();
}

void NJMJRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	IMJRoom::packRoomInfo(jsRoomInfo);
	jsRoomInfo["bankIdx"] = m_nBankerIdx;
	jsRoomInfo["curActIdex"] = getCurState()->getCurIdx();
	jsRoomInfo["waitTimer"] = getCurState()->getStateDuring();
	if (isFanBei()) {
		jsRoomInfo["fanBei"] = 1;
		if (isEnableLeiBaTa()) {
			jsRoomInfo["LBTCnt"] = m_nLeiBaTaCnt;
		}
	}
}

void NJMJRoom::visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)
{
	if (!pPlayer)
	{
		return;
	}

	IMJRoom::visitPlayerInfo(pPlayer, jsPlayerInfo, nVisitorSessionID);
	jsPlayerInfo["extraTime"] = ((NJMJPlayer*)pPlayer)->getExtraTime();
	if (isEnableWaiBao()) {
		jsPlayerInfo["extraOffset"] = ((NJMJPlayer*)pPlayer)->getExtraOffset();
	}

	if (pPlayer->haveState(eRoomPeer_CanAct) == false)  // not join this round game ;
	{
		return;
	}

	((NJMJPlayer*)pPlayer)->getPlayerCard()->onVisitPlayerCardInfo(jsPlayerInfo, pPlayer->getSessionID() == nVisitorSessionID);
}

void NJMJRoom::onWillStartGame() {
	IMJRoom::onWillStartGame();
	m_vSettle.clear();
	m_cFollowCards.reset();
	m_cCheckHuCard.reset();
	m_nGangCnt = 0;

	doProduceNewBanker();
	doWillFanBei();
	doLeiBaTa();
}

void NJMJRoom::onStartGame()
{
	IMJRoom::onStartGame();
	sendStartGameMsg();
}

void NJMJRoom::packStartGameReplyInfo(Json::Value& jsFrameArg) {
	IMJRoom::packStartGameReplyInfo(jsFrameArg);
	if (isFanBei()) {
		jsFrameArg["fanBei"] = 1;
		if (isEnableLeiBaTa()) {
			jsFrameArg["LBTCnt"] = m_nLeiBaTaCnt;
		}
	}
}

void NJMJRoom::sendStartGameMsg() {
	Json::Value jsMsg;
	jsMsg["bankerIdx"] = getBankerIdx();
	if (isFanBei()) {
		jsMsg["fanBei"] = 1;
		if (isEnableLeiBaTa()) {
			jsMsg["LBTCnt"] = m_nLeiBaTaCnt;
		}
	}
	//给围观者发送开始消息
	sendRoomMsgToStander(jsMsg, MSG_ROOM_MQMJ_GAME_START);

	//Json::Value arrPeerCards;
	for (auto& pPlayer : m_vPlayers)
	{
		if (!pPlayer)
		{
			LOGFMTE("why player is null hz mj must all player is not null");
			continue;
		}
		Json::Value peer;
		auto pPlayerCard = ((NJMJPlayer*)pPlayer)->getPlayerCard();
		IMJPlayerCard::VEC_CARD vCard;
		pPlayerCard->getHoldCard(vCard);
		for (auto& vC : vCard)
		{
			peer[peer.size()] = vC;
		}
		jsMsg["cards"] = peer;
		sendMsgToPlayer(jsMsg, MSG_ROOM_MQMJ_GAME_START, pPlayer->getSessionID());
	}
}

uint8_t NJMJRoom::getRoomType()
{
	return eGame_NJMJ;
}

IPoker* NJMJRoom::getPoker()
{
	return &m_tPoker;
}

bool NJMJRoom::needChu() {
	return true;
}

bool NJMJRoom::isGameOver() {
	if (isCanGoOnMoPai() == false) {
		return true;
	}
	else if (isInternalShouldCloseAll()) {
		return true;
	}
	for (auto& ref : m_vPlayers) {
		if (ref) {
			if (ref->haveState(eRoomPeer_CanAct) == false) {
				continue;
			}
			if (ref->haveState(eRoomPeer_AlreadyHu)) {
				return true;
			}
		}
	}
	return false;
}

bool NJMJRoom::isRoomOver() {
	if (isInternalShouldCloseAll()) {
		return true;
	}
	return IMJRoom::isRoomOver();
}

void NJMJRoom::onGameEnd() {
	Json::Value jsReal, jsPlayers;
	settleInfoToJson(jsReal);

	for (auto& ref : m_vPlayers) {
		if (ref) {
			Json::Value jsHoldCard, jsPlayer;
			IMJPlayerCard::VEC_CARD vHoldCard;
			((NJMJPlayer*)ref)->getPlayerCard()->getHoldCard(vHoldCard);
			for (auto& refCard : vHoldCard) {
				jsHoldCard[jsHoldCard.size()] = refCard;
			}
			jsPlayer["idx"] = ref->getIdx();
			jsPlayer["offset"] = ref->getSingleOffset();
			jsPlayer["chips"] = ref->getChips();
			jsPlayer["holdCard"] = jsHoldCard;
			jsPlayers[jsPlayers.size()] = jsPlayer;
		}
		//jsHoldCards[jsHoldCards.size()] = jsHoldCard;
	}

	Json::Value jsMsg;
	jsMsg["realTimeCal"] = jsReal;
	jsMsg["players"] = jsPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_SCMJ_GAME_END);

	IMJRoom::onGameEnd();
}

void NJMJRoom::doProduceNewBanker() {
	if ((uint8_t)-1 == getBankerIdx()) {
		setBankIdx(0);
	}
	else {
		if ((uint8_t)-1 == m_nNextBankerIdx) {
			//荒庄庄不变
		}
		else {
			setBankIdx(m_nNextBankerIdx);
		}
	}

	//保险机制
	if ((uint8_t)-1 == getBankerIdx()) {
		setBankIdx(0);
	}

	m_nNextBankerIdx = getBankerIdx();

	auto pPlayer = (NJMJPlayer*)getPlayerByIdx(getBankerIdx());
	if (pPlayer) {
		pPlayer->addBankerCnt();
	}
}

void NJMJRoom::setNextBankerIdx(std::vector<uint8_t>& vHuIdx) {
	if ((uint8_t)-1 == getBankerIdx()) {
		m_nNextBankerIdx = 0;
	}
	else {
		if (vHuIdx.empty()) {
			m_nNextBankerIdx = getBankerIdx();
		}
		else {
			auto iter = std::find(vHuIdx.begin(), vHuIdx.end(), getBankerIdx());
			if (iter == vHuIdx.end()) {
				m_nNextBankerIdx = getNextActPlayerIdx(getBankerIdx());
				//接庄比下胡
				if (isEnableJieZhuangBi()) {
					if (std::find(vHuIdx.begin(), vHuIdx.end(), m_nNextBankerIdx) != vHuIdx.end()) {
						m_bWillFanBei = true;
					}
				}
			}
			else {
				m_nNextBankerIdx = getBankerIdx();
			}
			//m_nNextBankerIdx = iter == vHuIdx.end() ? getNextActPlayerIdx(getBankerIdx()) : getBankerIdx();
			//m_nNextBankerIdx = nHuIdx == getBankerIdx() ? nHuIdx : getNextActPlayerIdx(getBankerIdx());
			if (m_nNextBankerIdx == 0 && getBankerIdx()) {
				signOneCircleEnd();
			}
		}
	}

	if ((uint8_t)-1 == m_nNextBankerIdx) {
		m_nNextBankerIdx = 0;
	}
}

void NJMJRoom::onPlayerChu(uint8_t nIdx, uint8_t nCard) {
	IMJRoom::onPlayerChu(nIdx, nCard);

	auto pActPlayer = (NJMJPlayer*)getPlayerByIdx(nIdx);
	pActPlayer->setSongGangIdx();

	auto pActCard = (NJMJPlayerCard*)pActPlayer->getPlayerCard();
	if (isEnableSiLianFeng()) {
		if (pActCard->isSiLianFeng()) {
			m_bWillFanBei = true;
			//TODO
			stSettle st;
			st.eSettleReason = eMJAct_4Feng;
			uint16_t nWinCoin = 0;
			uint16_t nLoseCoin = 5 * getBaseScore();
			for (auto& pp : m_vPlayers) {
				if (pp) {
					if (pp->getIdx() == nIdx) {
						continue;
					}
					else {
						st.addLose(pp->getIdx(), nLoseCoin);
						nWinCoin += nLoseCoin;
					}
				}
			}
			st.addWin(nIdx, nWinCoin);
			addSettle(st);
		}
	}

	if (pActCard->isZiDaAnGang(nCard)) {
		m_bWillFanBei = true;
		//TODO
		stSettle st;
		st.eSettleReason = eMJAct_ZiDaAnGang;
		uint16_t nWinCoin = 0;
		uint16_t nLoseCoin = 5 * getBaseScore();
		for (auto& pp : m_vPlayers) {
			if (pp) {
				if (pp->getIdx() == nIdx) {
					continue;
				}
				else {
					st.addLose(pp->getIdx(), nLoseCoin);
					nWinCoin += nLoseCoin;
				}
			}
		}
		st.addWin(nIdx, nWinCoin);
		addSettle(st);
	}

	if (m_cFollowCards.onChuCard(nIdx, nCard)) {
		auto pLosePlayer = (NJMJPlayer*)getPlayerByIdx(m_cFollowCards.m_nFirstIdx);
		if (pLosePlayer) {
			m_bWillFanBei = true;
			//TODO
			stSettle st;
			st.eSettleReason = eMJAct_Followed;
			uint16_t nWinCoin = 5 * getBaseScore();
			uint16_t nLoseCoin = 5 * getBaseScore() * (getSeatCnt() - 1);
			if (pLosePlayer->canPayOffset(nLoseCoin, getGuang()) == false) {
				nWinCoin = 0;
				nLoseCoin = 0;
			}

			for (auto& pp : m_vPlayers) {
				if (pp) {
					if (pp->getIdx() == pLosePlayer->getIdx()) {
						st.addLose(pp->getIdx(), nLoseCoin);
					}
					else {
						st.addWin(pp->getIdx(), nWinCoin);
					}
				}
			}
			addSettle(st);
		}
	}
}

void NJMJRoom::onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	IMJRoom::onPlayerMingGang(nIdx, nCard, nInvokeIdx);

	m_nGangCnt++;
	if (isEnableShuangGang() && m_nGangCnt > 1) {
		m_bWillFanBei = true;
	}

	auto pActPlayer = (NJMJPlayer*)getPlayerByIdx(nIdx);
	pActPlayer->setSongGangIdx(nInvokeIdx);
	pActPlayer->clearFlag(IMJPlayer::eMJActFlag_WaitCheckTianTing);
}

void NJMJRoom::onPlayerBuGang(uint8_t nIdx, uint8_t nCard) {
	IMJRoom::onPlayerBuGang(nIdx, nCard);

	m_nGangCnt++;
	if (isEnableShuangGang() && m_nGangCnt > 1) {
		m_bWillFanBei = true;
	}

	auto pActPlayer = (NJMJPlayer*)getPlayerByIdx(nIdx);
	auto pActCard = (NJMJPlayerCard*)pActPlayer->getPlayerCard();
	auto nInvokerIdx = pActCard->getMingCardInvokerIdx(nCard, eMJAct_BuGang);
	if ((uint8_t)-1 != nInvokerIdx) {
		pActPlayer->setSongGangIdx(nInvokerIdx);
	}
}

void NJMJRoom::onPlayerAnGang(uint8_t nIdx, uint8_t nCard) {
	IMJRoom::onPlayerAnGang(nIdx, nCard);

	if (isEnableShuangGang()) {
		m_bWillFanBei = true;
	}
}

void NJMJRoom::onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	if (vHuIdx.empty())
	{
		LOGFMTE("why hu vec is empty ? room id = %u", getRoomID());
		return;
	}

	setNextBankerIdx(vHuIdx);

	Json::Value jsDetail, jsMsg;
	bool isZiMo = vHuIdx.front() == nInvokeIdx;
	jsMsg["isZiMo"] = isZiMo ? 1 : 0;
	jsMsg["huCard"] = nCard;
	stSettle st;
	st.eSettleReason = eMJAct_Hu;

	if (isZiMo) {
		auto pZiMoPlayer = (NJMJPlayer*)getPlayerByIdx(nInvokeIdx);
		if (pZiMoPlayer == nullptr)
		{
			LOGFMTE("room id = %u zi mo player is nullptr idx = %u ", getRoomID(), nInvokeIdx);
			return;
		}

		pZiMoPlayer->addZiMoCnt();
		pZiMoPlayer->setState(eRoomPeer_AlreadyHu);
		// svr :{ huIdx : 234 , baoPaiIdx : 2 , winCoin : 234,huardSoftHua : 23, isGangKai : 0 ,vhuTypes : [ eFanxing , ], LoseIdxs : [ {idx : 1 , loseCoin : 234 }, .... ]   }
		jsDetail["huIdx"] = nInvokeIdx;
		std::vector<eFanxingType> vType;
		uint32_t nFanCnt = 0;
		uint16_t nHoldHuaCnt = 0;
		uint16_t nHuHuaCnt = getBaseScore();
		auto pZiMoPlayerCard = (NJMJPlayerCard*)pZiMoPlayer->getPlayerCard();
		pZiMoPlayerCard->setHuCard(0);
		m_cFanxingChecker.checkFanxing(vType, pZiMoPlayer, nInvokeIdx, this);
		sortFanxing2FanCnt(vType, nHuHuaCnt);
		pZiMoPlayer->setBestCards(nHuHuaCnt);
		if (std::find(vType.begin(), vType.end(), eFanxing_DaMenQing) == vType.end()) {
			nHoldHuaCnt = pZiMoPlayerCard->getHuaCntWithoutHuTypeHuaCnt();
		}

		//花砸
		if (isEnableHuaZa()) {
			nHoldHuaCnt *= 2;
		}

		nFanCnt = nHoldHuaCnt + nHuHuaCnt;
		/*if (m_bFanBei) {
			nFanCnt *= 2;
		}*/
		Json::Value jsHuTyps;
		for (auto& refHu : vType)
		{
			jsHuTyps[jsHuTyps.size()] = refHu;
		}
		jsDetail["vhuTypes"] = jsHuTyps;

		uint32_t nTotalWin = 0;
		for (auto& pLosePlayer : m_vPlayers)
		{
			if (pLosePlayer) {
				if (pLosePlayer == pZiMoPlayer)
				{
					continue;
				}
				st.addLose(pLosePlayer->getIdx(), nFanCnt);
				nTotalWin += nFanCnt;
			}
		}
		st.addWin(nInvokeIdx, nTotalWin);
		LOGFMTD("room id = %u player = %u zimo", getRoomID(), nInvokeIdx);
	}
	else {
		auto pLosePlayer = (NJMJPlayer*)getPlayerByIdx(nInvokeIdx);
		if (!pLosePlayer)
		{
			LOGFMTE("room id = %u lose but player idx = %u is nullptr", getRoomID(), nInvokeIdx);
			return;
		}
		jsDetail["dianPaoIdx"] = pLosePlayer->getIdx();
		pLosePlayer->addDianPaoCnt();

		if (vHuIdx.size() > 1)  // yi pao duo xiang 
		{
			//update by haodi 增加滴零处理，不用且操作避免影响一炮多响后期增加其他规则
			/*if (isDiLing()) {
				m_bWillFanBei = true;
			}*/

		}

		std::vector<uint8_t> vOrderHu;
		if (vHuIdx.size() > 1)
		{
			for (uint8_t offset = 1; offset < getSeatCnt(); ++offset)
			{
				auto nCheckIdx = nInvokeIdx + offset;
				nCheckIdx = nCheckIdx % getSeatCnt();
				auto iter = std::find(vHuIdx.begin(), vHuIdx.end(), nCheckIdx);
				if (iter != vHuIdx.end())
				{
					vOrderHu.push_back(nCheckIdx);
				}
			}
		}
		else
		{
			vOrderHu.swap(vHuIdx);
		}

		Json::Value jsHuPlayers;
		uint32_t nTotalLose = 0;
		for (auto& nHuIdx : vOrderHu)
		{
			auto pHuPlayer = (NJMJPlayer*)getPlayerByIdx(nHuIdx);
			if (pHuPlayer == nullptr)
			{
				LOGFMTE("room id = %u hu player idx = %u , is nullptr", getRoomID(), nHuIdx);
				continue;
			}
			pHuPlayer->addHuCnt();

			Json::Value jsHuPlayer;
			jsHuPlayer["idx"] = nHuIdx;
			pHuPlayer->setState(eRoomPeer_AlreadyHu);

			auto pHuPlayerCard = (NJMJPlayerCard*)pHuPlayer->getPlayerCard();
			pHuPlayerCard->setHuCard(nCard);

			std::vector<eFanxingType> vType;
			uint32_t nFanCnt = 0;
			uint16_t nHoldHuaCnt = 0;
			uint16_t nHuHuaCnt = getBaseScore();
			pHuPlayerCard->onDoHu(nInvokeIdx, nCard, pLosePlayer->haveGangFlag());
			m_cFanxingChecker.checkFanxing(vType, pHuPlayer, nInvokeIdx, this);
			sortFanxing2FanCnt(vType, nHuHuaCnt);
			pHuPlayer->setBestCards(nHuHuaCnt);
			if (std::find(vType.begin(), vType.end(), eFanxing_DaMenQing) == vType.end()) {
				nHoldHuaCnt = pHuPlayerCard->getHuaCntWithoutHuTypeHuaCnt();
			}

			//花砸
			if (isEnableHuaZa()) {
				nHoldHuaCnt *= 2;
			}

			nFanCnt = nHoldHuaCnt + nHuHuaCnt;
			/*if (m_bFanBei) {
				nFanCnt *= 2;
			}*/
			if (std::find(vType.begin(), vType.end(), eFanxing_QiangGang) != vType.end()) {
				nFanCnt *= getSeatCnt() - 1;
			}

			Json::Value jsHuTyps;
			for (auto& refHu : vType)
			{
				jsHuTyps[jsHuTyps.size()] = refHu;
			}
			jsHuPlayer["vhuTypes"] = jsHuTyps;

			st.addLose(nInvokeIdx, nFanCnt);
			st.addWin(nHuIdx, nFanCnt);

			jsHuPlayers[jsHuPlayers.size()] = jsHuPlayer;

			LOGFMTD("room id = %u player = %u hu", getRoomID(), nHuIdx);
		}
		jsDetail["huPlayers"] = jsHuPlayers;
	}
	jsMsg["detail"] = jsDetail;
	st.jsHuMsg = jsMsg;
	sendRoomMsg(jsMsg, MSG_ROOM_MQMJ_PLAYER_HU);
	addSettle(st);
}

void NJMJRoom::onWaitPlayerAct(uint8_t nIdx, bool& isCanPass) {
	auto pPlayer = (NJMJPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer) {
		LOGFMTE("player idx = %u is null can not tell it wait act", nIdx);
		return;
	}
	
	if (pPlayer->haveState(eRoomPeer_AlreadyHu)) {
		LOGFMTE("player idx = %u is alreay hu, why wait him to act", nIdx);
		return;
	}

	auto pMJCard = (NJMJPlayerCard*)pPlayer->getPlayerCard();
	// send msg to tell player do act 
	Json::Value jsArrayActs;
	Json::Value jsFrameActs;
	if (/*isCanGoOnMoPai() && */canGang())
	{
		// check bu gang .
		IMJPlayerCard::VEC_CARD vCards;
		pMJCard->getHoldCardThatCanBuGang(vCards);
		for (auto& ref : vCards)
		{
			Json::Value jsAct;
			jsAct["act"] = eMJAct_BuGang;
			jsAct["cardNum"] = ref;
			jsArrayActs[jsArrayActs.size()] = jsAct;
			jsFrameActs[jsFrameActs.size()] = eMJAct_BuGang;
		}
		vCards.clear();
		// check an gang .
		pMJCard->getHoldCardThatCanAnGang(vCards);
		for (auto& ref : vCards)
		{
			Json::Value jsAct;
			jsAct["act"] = eMJAct_AnGang;
			jsAct["cardNum"] = ref;
			jsArrayActs[jsArrayActs.size()] = jsAct;
			jsFrameActs[jsFrameActs.size()] = eMJAct_AnGang;
		}
		vCards.clear();
	}

	// check hu .
	uint8_t nJiang = 0;
	if (pMJCard->isHoldCardCanHu(nJiang))
	{
		Json::Value jsAct;
		jsAct["act"] = eMJAct_Hu;
		jsAct["cardNum"] = pMJCard->getNewestFetchedCard();
		jsArrayActs[jsArrayActs.size()] = jsAct;
		jsFrameActs[jsFrameActs.size()] = eMJAct_Hu;
	}

	isCanPass = jsArrayActs.empty() == false;
	jsFrameActs[jsFrameActs.size()] = eMJAct_Chu;

	// add default alwasy chu , infact need not add , becaust it alwasy in ,but compatable with current client ;
	Json::Value jsAct;
	jsAct["act"] = eMJAct_Chu;
	jsAct["cardNum"] = getAutoChuCardWhenWaitActTimeout(nIdx);
	jsArrayActs[jsArrayActs.size()] = jsAct;

	Json::Value jsMsg;
	jsMsg["acts"] = jsArrayActs;
	sendMsgToPlayer(jsMsg, MSG_PLAYER_WAIT_ACT_AFTER_RECEIVED_CARD, pPlayer->getSessionID());

	if (isCanPass)  // player do have option do select or need not give frame ;
	{
		Json::Value jsFrameArg;
		jsFrameArg["idx"] = nIdx;
		jsFrameArg["act"] = jsFrameActs;
		addReplayFrame(eMJFrame_WaitPlayerAct, jsFrameActs);
	}

	//LOGFMTD("tell player idx = %u do act size = %u",nIdx,jsArrayActs.size());
}

bool NJMJRoom::isAnyPlayerRobotGang(uint8_t nInvokeIdx, uint8_t nCard) {
	m_cCheckHuCard.setHuCard(nCard, nInvokeIdx, eMJAct_BuGang);
	return IMJRoom::isAnyPlayerRobotGang(nInvokeIdx, nCard);
}

bool NJMJRoom::isAnyPlayerPengOrHuThisCard(uint8_t nInvokeIdx, uint8_t nCard) {
	m_cCheckHuCard.setHuCard(nCard, nInvokeIdx, eMJAct_Chu);
	return IMJRoom::isAnyPlayerPengOrHuThisCard(nInvokeIdx, nCard);
}

uint8_t NJMJRoom::getNextActPlayerIdx(uint8_t nCurActIdx) {
	return (nCurActIdx + 1) % getSeatCnt();
}

uint8_t NJMJRoom::getBaseScore() {
	return IMJRoom::getBaseScore() * (isFanBei() ? 2 : 1);
}

uint32_t NJMJRoom::getGuang() {
	auto pNJMJOpts = std::dynamic_pointer_cast<NJMJOpts>(getDelegate()->getOpts());
	return pNJMJOpts->getGuang();
}

bool NJMJRoom::isEnableJieZhuangBi() {
	auto pNJMJOpts = std::dynamic_pointer_cast<NJMJOpts>(getDelegate()->getOpts());
	return pNJMJOpts->isEnableJieZhuangBi();
}

bool NJMJRoom::isEnableHuaZa() {
	auto pNJMJOpts = std::dynamic_pointer_cast<NJMJOpts>(getDelegate()->getOpts());
	return pNJMJOpts->isEnableHuaZa();
}

bool NJMJRoom::isEnableSiLianFeng() {
	auto pNJMJOpts = std::dynamic_pointer_cast<NJMJOpts>(getDelegate()->getOpts());
	return pNJMJOpts->isEnableSiLianFeng();
}

bool NJMJRoom::isEnableWaiBao() {
	auto pNJMJOpts = std::dynamic_pointer_cast<NJMJOpts>(getDelegate()->getOpts());
	return pNJMJOpts->isEnableWaiBao();
}

bool NJMJRoom::isEnableBiXiaHu() {
	auto pNJMJOpts = std::dynamic_pointer_cast<NJMJOpts>(getDelegate()->getOpts());
	return pNJMJOpts->isEnableBixiaHu();
}

bool NJMJRoom::isEnableLeiBaTa() {
	auto pNJMJOpts = std::dynamic_pointer_cast<NJMJOpts>(getDelegate()->getOpts());
	return pNJMJOpts->isEnableLeiBaoTa();
}

bool NJMJRoom::isEnableShuangGang() {
	auto pNJMJOpts = std::dynamic_pointer_cast<NJMJOpts>(getDelegate()->getOpts());
	return pNJMJOpts->isEnableShuangGang();
}

bool NJMJRoom::isEnableYiDuiDaoDi() {
	auto pNJMJOpts = std::dynamic_pointer_cast<NJMJOpts>(getDelegate()->getOpts());
	return pNJMJOpts->isEnableYiDuiDaoDi();
}

void NJMJRoom::addSettle(stSettle& tSettle) {
	m_vSettle.push_back(tSettle);

	Json::Value jsItem, jsRDetail;
	jsItem["actType"] = tSettle.eSettleReason;
	if (tSettle.eSettleReason == eMJAct_Hu) {
		jsItem["msg"] = tSettle.jsHuMsg;
	}

	uint32_t nTotalGain = 0;
	for (auto& refl : tSettle.vLoseIdx)
	{
		auto pPlayer = (NJMJPlayer*)getPlayerByIdx(refl.first);
		if (pPlayer) {
			if (tSettle.bWaiBao) {
				pPlayer->addExtraOffset(-1 * (int32_t)refl.second);
			}
			else {
				auto nGain = pPlayer->addGuangSingleOffset(-1 * (int32_t)refl.second, getGuang());
				nTotalGain += nGain;
			}
			Json::Value jsPlayer;
			jsPlayer["idx"] = refl.first;
			jsPlayer["chips"] = pPlayer->getChips();
			jsRDetail[jsRDetail.size()] = jsPlayer;
		}
	}

	for (auto& refl : tSettle.vWinIdxs)
	{
		auto pPlayer = (NJMJPlayer*)getPlayerByIdx(refl.first);
		uint32_t nTempWin = 0;
		if (nTotalGain > refl.second) {
			nTempWin = 0;
			nTotalGain -= refl.second;
		}
		else {
			nTempWin = refl.second - nTotalGain;
			nTotalGain = 0;
		}
		if (pPlayer) {
			if (tSettle.bWaiBao) {
				pPlayer->addExtraOffset(nTempWin);
			}
			else {
				pPlayer->addSingleOffset(nTempWin);
			}
			Json::Value jsPlayer;
			jsPlayer["idx"] = refl.first;
			jsPlayer["chips"] = pPlayer->getChips();
			jsRDetail[jsRDetail.size()] = jsPlayer;
		}
	}
	jsItem["detial"] = jsRDetail;

	sendRoomMsg(jsItem, MSG_ROOM_FXMJ_REAL_TIME_CELL);

	// add frame 
	addReplayFrame(eMJFrame_Settle, jsItem);
}

void NJMJRoom::settleInfoToJson(Json::Value& jsRealTime) {
	for (auto& ref : m_vSettle) {
		Json::Value jsItem, jsRDetail;
		jsItem["actType"] = ref.eSettleReason;
		if (ref.eSettleReason == eMJAct_Hu) {
			jsItem["msg"] = ref.jsHuMsg;
		}

		for (auto& refl : ref.vLoseIdx)
		{
			auto pPlayer = (NJMJPlayer*)getPlayerByIdx(refl.first);
			if (pPlayer) {
				Json::Value jsPlayer;
				jsPlayer["idx"] = refl.first;
				jsPlayer["offset"] = -1 * refl.second;
				jsRDetail[jsRDetail.size()] = jsPlayer;
			}
		}

		for (auto& refl : ref.vWinIdxs)
		{
			auto pPlayer = getPlayerByIdx(refl.first);
			if (pPlayer) {
				Json::Value jsPlayer;
				jsPlayer["idx"] = refl.first;
				jsPlayer["offset"] = refl.second;
				jsRDetail[jsRDetail.size()] = jsPlayer;
			}
		}
		jsItem["detial"] = jsRDetail;
		jsRealTime[jsRealTime.size()] = jsItem;
	}
}

void NJMJRoom::sortFanxing2FanCnt(std::vector<eFanxingType>& vType, uint16_t& nFanCnt) {
	for (auto& ref : vType) {
		switch (ref)
		{
		case eFanxing_TianHu:
		case eFanxing_DiHu:
		{
			nFanCnt += 50;
		}
		break;
		case eFanxing_HaiDiLaoYue:
		case eFanxing_XiaoMenQing:
		case eFanxing_GangKai:
		{
			nFanCnt += 5;
		}
		break;
		case eFanxing_HunYiSe:
		case eFanxing_DuiDuiHu:
		case eFanxing_DaMenQing:
		case eFanxing_QuanQiuDuDiao:
		{
			nFanCnt += 10;
		}
		break;
		case eFanxing_QingYiSe:
		{
			nFanCnt += 30;
		}
		break;
		case eFanxing_ShuangQiDui:
		{
			nFanCnt += 40;
		}
		break;
		case eFanxing_QiDui:
		{
			nFanCnt += 20;
		}
		break;
		}
	}
}

bool NJMJRoom::canStartGame() {
	for (auto ref : m_vPlayers) {
		if (ref && ref->haveState(eRoomPeer_Ready)) {
			continue;
		}
		return false;
	}

	return IMJRoom::canStartGame();
}

void NJMJRoom::doRandomChangeSeat() {
	/*Json::Value jsMsg, jsPlayerPre, jsPlayersPre;
	for (auto ref : m_vPlayers) {
		jsPlayerPre["idx"] = ref->getIdx();
		jsPlayerPre["uid"] = ref->getUserUID();
		jsPlayersPre[jsPlayersPre.size()] = jsPlayerPre;
	}
	jsMsg["detailPre"] = jsPlayersPre;*/

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

bool NJMJRoom::doChangeSeat(uint16_t nIdx, uint16_t nWithIdx) {
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

bool NJMJRoom::isOneCircleEnd() {
	if (isCircle()) {
		bool isEnd = false;
		if (m_bOneCircleEnd) {
			isEnd = true;
			clearOneCircleEnd();
		}
		return isEnd;
	}
	return true;
}

void NJMJRoom::doWillFanBei() {
	m_bFanBei = false;

	if (m_bWillFanBei) {
		m_bFanBei = true;
	}

	m_bWillFanBei = false;
}

void NJMJRoom::doLeiBaTa() {
	if (isFanBei() && isEnableLeiBaTa()) {
		m_nLeiBaTaCnt++;
	}
	else {
		m_nLeiBaTaCnt = 0;
	}
}

bool NJMJRoom::checkBaoGuang(uint8_t& nIdx) {
	if ((uint8_t)-1 == nIdx) {
		nIdx = m_cCheckHuCard.m_nIdx;
	}

	if ((uint8_t)-1 == nIdx) {
		return true;
	}

	if (getGuang() && isEnableWaiBao() == false) {
		auto pPlayer = (NJMJPlayer*)getPlayerByIdx(nIdx);
		if (pPlayer && pPlayer->canBackGain(getGuang())) {
			return false;
		}
		return true;
	}

	return false;
}

bool NJMJRoom::checkHuGuang(uint8_t& nIdx) {
	nIdx = m_cCheckHuCard.m_nIdx;

	if ((uint8_t)-1 == nIdx) {
		return true;
	}

	if (getGuang()) {
		auto pPlayer = (NJMJPlayer*)getPlayerByIdx(nIdx);
		if (pPlayer && pPlayer->canBackGain(getGuang())) {
			return false;
		}
		return true;
	}

	return false;
}

bool NJMJRoom::checkGuang(uint8_t nIdx) {
	if ((uint8_t)-1 == nIdx) {
		return true;
	}

	if (getGuang()) {
		auto pPlayer = (NJMJPlayer*)getPlayerByIdx(nIdx);
		if (pPlayer && pPlayer->canBackGain(getGuang())) {
			return false;
		}
		return true;
	}

	return false;
}

bool NJMJRoom::isInternalShouldCloseAll() {
	if (getGuang()) {
		uint16_t nCnt = 0;
		for (auto& ref : m_vPlayers) {
			if (ref->getChips() < 0) {
				uint32_t loseChip = abs(ref->getChips());
				if (loseChip < getGuang()) {
					continue;
				}
				nCnt++;
			}
		}
		if (nCnt > 1) {
			return true;
		}
	}
	return false;
}