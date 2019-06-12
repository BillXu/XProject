#include "SDMJRoom.h"
#include "SDMJPlayer.h"
#include "CommonDefine.h"
#include "SDMJRoomStateWaitPlayerChu.h"
#include "SDMJRoomStateWaitPlayerAct.h"
#include "SDMJRoomStateStartGame.h"
#include "MJRoomStateGameEnd.h"
#include "SDMJRoomStateAskForRobotGang.h"
#include "SDMJRoomStateAskForPengOrHu.h"
#include "MJRoomStateWaitReady.h"
#include "MJRoomStateDoPlayerAct.h"
#include "MJRoomStateAutoBuHua.h"
#include "FanxingChecker.h"
#include "Fanxing7Dui.h"
#include "FanxingDuiDuiHu.h"
#include "MJReplayFrameType.h"
#include "SDMJPoker.h"
#include "IGameRoomDelegate.h"
#include "SDMJOpts.h"

bool SDMJRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)
{
	IMJRoom::init(pRoomMgr,nSeialNum,nRoomID,ptrGameOpts);
	m_cFanxingChecker.init();
	m_vGainChip.resize(getSeatCnt());
	m_nDice = 0;
	m_bFanBei = false;
	m_bWillFanBei = false;
	m_bBankerHu = false;

	m_nLastHuIdx = -1;
	m_nYiPaoDuoXiang = -1;
	m_bHuangZhuang = true;
	// add room state ;
	IGameRoomState* p[] = { new CMJRoomStateWaitReady(), new SDMJRoomStateWaitPlayerChu(),new SDMJRoomStateWaitPlayerAct(),
		new SDMJRoomStateStartGame(),new MJRoomStateGameEnd(),new MJRoomStateDoPlayerAct(),new SDMJRoomStateAskForRobotGang(),
		new SDMJRoomStateAskForPengOrHu(), new MJRoomStateAutoBuHua()
	};
	for ( auto& pS : p )
	{
		addRoomState(pS);
	}
	setInitState(p[0]);
	return true;
}

IGamePlayer* SDMJRoom::createGamePlayer()
{
	auto pPlayer = new SDMJPlayer();
	((SDMJPlayerCard*)(pPlayer->getPlayerCard()))->setRuleMode(getRuleMode());
	return pPlayer;
}

void SDMJRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	IMJRoom::packRoomInfo(jsRoomInfo);
	jsRoomInfo["bankIdx"] = m_nBankerIdx;
	jsRoomInfo["curActIdex"] = getCurState()->getCurIdx();
	jsRoomInfo["waitTimer"] = getCurState()->getStateDuring();
	jsRoomInfo["dice"] = m_nDice;
	jsRoomInfo["isFanBei"] = m_bFanBei ? 1 : 0;
	jsRoomInfo["isWillFanBei"] = m_bWillFanBei ? 1 : 0;
}

void SDMJRoom::visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)
{
	if (!pPlayer)
	{
		return;
	}

	IMJRoom::visitPlayerInfo(pPlayer, jsPlayerInfo, nVisitorSessionID);
	jsPlayerInfo["extraTime"] = ((SDMJPlayer*)pPlayer)->getExtraTime();

	if (pPlayer->haveState(eRoomPeer_CanAct) == false)  // not join this round game ;
	{
		return;
	}

	((SDMJPlayer*)pPlayer)->getPlayerCard()->onVisitPlayerCardInfo(jsPlayerInfo, pPlayer->getSessionID() == nVisitorSessionID);
}

void SDMJRoom::onWillStartGame() {
	IMJRoom::onWillStartGame();
	m_vSettle.clear();
	clearGain();
	m_nDice = 0;

	doWillFanBei();
	doFanBei();
	doProduceNewBanker();
}

void SDMJRoom::onStartGame()
{
	IMJRoom::onStartGame();
	sendStartGameMsg();
}

void SDMJRoom::packStartGameReplyInfo(Json::Value& jsFrameArg) {
	IMJRoom::packStartGameReplyInfo(jsFrameArg);
	jsFrameArg["dice"] = m_nDice;
	jsFrameArg["isFanBei"] = m_bFanBei ? 1 : 0;
	jsFrameArg["isWillFanBei"] = m_bWillFanBei ? 1 : 0;
}

void SDMJRoom::sendStartGameMsg() {
	Json::Value jsMsg;
	jsMsg["dice"] = m_nDice;
	jsMsg["bankerIdx"] = getBankerIdx();
	jsMsg["isFanBei"] = m_bFanBei ? 1 : 0;
	jsMsg["isWillFanBei"] = m_bWillFanBei ? 1 : 0;
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
		auto pPlayerCard = ((SDMJPlayer*)pPlayer)->getPlayerCard();
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

uint8_t SDMJRoom::getRoomType()
{
	return eGame_SDMJ;
}

IPoker* SDMJRoom::getPoker()
{
	return &m_tPoker;
}

bool SDMJRoom::needChu() {
	return true;
}

bool SDMJRoom::isGameOver() {
	if (isCanGoOnMoPai() == false) {
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

bool SDMJRoom::isRoomOver() {
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
		if (nCnt + 1 >= getSeatCnt()) {
			return true;
		}
	}
	return IMJRoom::isRoomOver();
}

void SDMJRoom::onGameEnd() {
	if (m_bHuangZhuang && isDiLing()) {
		m_bWillFanBei = true;
	}

	Json::Value jsReal, jsPlayers;
	settleInfoToJson(jsReal);

	for (auto& ref : m_vPlayers) {
		if (ref) {
			Json::Value jsHoldCard, jsPlayer;
			IMJPlayerCard::VEC_CARD vHoldCard;
			((SDMJPlayer*)ref)->getPlayerCard()->getHoldCard(vHoldCard);
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

void SDMJRoom::doProduceNewBanker() {
	if ((uint8_t)-1 == m_nBankerIdx)
	{
		m_nBankerIdx = 0;
	}
	else
	{
		if ((uint8_t)-1 == m_nYiPaoDuoXiang) {
			if (m_bHuangZhuang) {
				m_nBankerIdx = (m_nBankerIdx + 1) % MAX_SEAT_CNT;
			}
			else {
				if ((uint8_t)-1 != m_nLastHuIdx) {
					m_nBankerIdx = m_nLastHuIdx;
				}
			}
		}
		else {
			m_nBankerIdx = m_nYiPaoDuoXiang;
		}
	}

	m_nLastHuIdx = -1;
	m_nYiPaoDuoXiang = -1;
	m_bHuangZhuang = true;
	m_bBankerHu = false;
}

void SDMJRoom::doFanBei() {
	m_nDice = 1 + rand() % 6;
	m_nDice = m_nDice * 10 + (1 + rand() % 6);
	if (isDiLing() && (41 == m_nDice || 14 == m_nDice || m_nDice % 11 == 0)) {
		if (m_bFanBei) {
			m_bWillFanBei = true;
		}
		m_bFanBei = true;
	}
}

void SDMJRoom::doWillFanBei() {
	m_bFanBei = false;
	if (m_bWillFanBei)
	{
		m_bFanBei = true;
	}
	m_bWillFanBei = false;
}

void SDMJRoom::onPlayerChu(uint8_t nIdx, uint8_t nCard) {
	IMJRoom::onPlayerChu(nIdx, nCard);

	auto pActPlayer = (SDMJPlayer*)getPlayerByIdx(nIdx);
	auto pActCard = (SDMJPlayerCard*)pActPlayer->getPlayerCard();
	//update by haodi 添加漏碰规则:如果有人手上有3张一样的牌，但他把这个牌打了出去，在他没摸牌之前，别人也打同样的牌，也是不允许碰的
	if (pActCard->canPengWithCard(nCard)) {
		onPlayerLouPeng(nIdx, nCard);
	}

	if (pActCard->canHuWitCard(nCard)) {
		onPlayerLouHu(nIdx, nIdx);
	}
}

void SDMJRoom::onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	IMJRoom::onPlayerMingGang(nIdx, nCard, nInvokeIdx);
	auto pActPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	pActPlayer->clearFlag(IMJPlayer::eMJActFlag_WaitCheckTianTing);
}

void SDMJRoom::onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	if (vHuIdx.empty())
	{
		LOGFMTE("why hu vec is empty ? room id = %u", getRoomID());
		return;
	}

	m_bHuangZhuang = false;
	Json::Value jsDetail, jsMsg;
	bool isZiMo = vHuIdx.front() == nInvokeIdx;
	jsMsg["isZiMo"] = isZiMo ? 1 : 0;
	jsMsg["huCard"] = nCard;
	jsMsg["isFanBei"] = m_bFanBei ? 1 : 0;
	stSettle st;
	st.eSettleReason = eMJAct_Hu;

	if (isZiMo) {
		auto pZiMoPlayer = (SDMJPlayer*)getPlayerByIdx(nInvokeIdx);
		if (pZiMoPlayer == nullptr)
		{
			LOGFMTE("room id = %u zi mo player is nullptr idx = %u ", getRoomID(), nInvokeIdx);
			return;
		}

		setLastHuIdx(nInvokeIdx);

		pZiMoPlayer->addZiMoCnt();
		pZiMoPlayer->setState(eRoomPeer_AlreadyHu);
		// svr :{ huIdx : 234 , baoPaiIdx : 2 , winCoin : 234,huardSoftHua : 23, isGangKai : 0 ,vhuTypes : [ eFanxing , ], LoseIdxs : [ {idx : 1 , loseCoin : 234 }, .... ]   }
		jsDetail["huIdx"] = nInvokeIdx;
		std::vector<eFanxingType> vType;
		uint32_t nFanCnt = 0;
		uint16_t nHoldHuaCnt = 0;
		uint16_t nHuHuaCnt = getBaseScore();
		auto pZiMoPlayerCard = (SDMJPlayerCard*)pZiMoPlayer->getPlayerCard();
		pZiMoPlayerCard->setHuCard(0);
		m_cFanxingChecker.checkFanxing(vType, pZiMoPlayer, nInvokeIdx, this);
		sortFanxing2FanCnt(vType, nHuHuaCnt);
		pZiMoPlayer->setBestCards(nHuHuaCnt);
		if (std::find(vType.begin(), vType.end(), eFanxing_DaMenQing) == vType.end()) {
			nHoldHuaCnt = pZiMoPlayerCard->getHuaCntWithoutHuTypeHuaCnt();
			pZiMoPlayerCard->getAnKeHuaType(vType);
		}
		nFanCnt = nHoldHuaCnt + nHuHuaCnt;
		if (m_bFanBei) {
			nFanCnt *= 2;
		}
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
		auto pLosePlayer = (SDMJPlayer*)getPlayerByIdx(nInvokeIdx);
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
			if (isDiLing()) {
				m_bWillFanBei = true;
			}
			setYiPaoDuoXiang(nInvokeIdx);
		}
		else {
			setLastHuIdx(vHuIdx.front());
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
			auto pHuPlayer = (SDMJPlayer*)getPlayerByIdx(nHuIdx);
			if (pHuPlayer == nullptr)
			{
				LOGFMTE("room id = %u hu player idx = %u , is nullptr", getRoomID(), nHuIdx);
				continue;
			}
			pHuPlayer->addHuCnt();

			Json::Value jsHuPlayer;
			jsHuPlayer["idx"] = nHuIdx;
			pHuPlayer->setState(eRoomPeer_AlreadyHu);

			auto pHuPlayerCard = (SDMJPlayerCard*)pHuPlayer->getPlayerCard();
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
				pHuPlayerCard->getAnKeHuaType(vType);
			}
			nFanCnt = nHoldHuaCnt + nHuHuaCnt;
			if (m_bFanBei) {
				nFanCnt *= 2;
			}
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

void SDMJRoom::onWaitPlayerAct(uint8_t nIdx, bool& isCanPass) {
	auto pPlayer = (SDMJPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer) {
		LOGFMTE("player idx = %u is null can not tell it wait act", nIdx);
		return;
	}
	
	if (pPlayer->haveState(eRoomPeer_AlreadyHu)) {
		LOGFMTE("player idx = %u is alreay hu, why wait him to act", nIdx);
		return;
	}

	auto pMJCard = (SDMJPlayerCard*)pPlayer->getPlayerCard();
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
	if (pMJCard->isHoldCardCanHu(nJiang, pPlayer->haveGangFlag() || pPlayer->haveFlag(IMJPlayer::eMJActFlag_BuHua)))
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

bool SDMJRoom::isAnyPlayerRobotGang(uint8_t nInvokeIdx, uint8_t nCard) {
	for (auto& ref : m_vPlayers)
	{
		if (ref == nullptr || nInvokeIdx == ref->getIdx())
		{
			continue;
		}

		if (ref->haveState(eRoomPeer_AlreadyHu)) {
			continue;
		}

		auto pMJCard = (SDMJPlayerCard*)((IMJPlayer*)ref)->getPlayerCard();
		auto isLouHu = ((IMJPlayer*)ref)->haveFlag(IMJPlayer::eMJActFlag_LouHu);
		if ((isLouHu == false) && pMJCard->canHuWitCard(nCard, true))
		{
			return true;
		}
	}

	return false;
}

void SDMJRoom::onAskForRobotGang(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates) {
	// send decalre gang msg ;
	Json::Value msg;
	msg["idx"] = nInvokeIdx;
	msg["actType"] = eMJAct_BuGang_Pre;
	msg["card"] = nCard;
	sendRoomMsg(msg, MSG_ROOM_ACT);

	// inform target player do this things 
	Json::Value jsFrameArg;
	for (auto& ref : m_vPlayers)
	{
		if (ref == nullptr || nInvokeIdx == ref->getIdx())
		{
			continue;
		}

		if (ref->haveState(eRoomPeer_AlreadyHu)) {
			continue;
		}

		Json::Value jsMsg;
		jsMsg["invokerIdx"] = nInvokeIdx;
		jsMsg["cardNum"] = nCard;

		Json::Value jsActs;
		auto pMJCard = (SDMJPlayerCard*)((IMJPlayer*)ref)->getPlayerCard();
		// check hu 
		auto isLouHu = ((IMJPlayer*)ref)->haveFlag(IMJPlayer::eMJActFlag_LouHu);
		if ((isLouHu == false) && pMJCard->canHuWitCard(nCard, true))
		{
			jsActs[jsActs.size()] = eMJAct_Hu;
			vOutCandinates.push_back(ref->getIdx());

			jsFrameArg[jsFrameArg.size()] = ref->getIdx();
		}

		if (jsActs.size() > 0)
		{
			jsActs[jsActs.size()] = eMJAct_Pass;
		}
		else
		{
			continue;
		}

		jsMsg["acts"] = jsActs;
		sendMsgToPlayer(jsMsg, MSG_PLAYER_WAIT_ACT_ABOUT_OTHER_CARD, ref->getSessionID());
		//LOGFMTD("inform uid = %u robot gang card = %u room id = %u ", ref->getUID(),nCard, getRoomID());
	}

	// add frame 
	addReplayFrame(eMJFrame_WaitRobotGang, jsFrameArg);
}

uint8_t SDMJRoom::getNextActPlayerIdx(uint8_t nCurActIdx) {
	return (nCurActIdx + 1) % getSeatCnt();
}

uint32_t SDMJRoom::getGuang() {
	return 0;
}

bool SDMJRoom::isDiLing() {
	auto pSDMJOpts = std::dynamic_pointer_cast<SDMJOpts>(getDelegate()->getOpts());
	return pSDMJOpts->isEnableDiLing();
}

bool SDMJRoom::isEanableHHQD() {
	auto pSDMJOpts = std::dynamic_pointer_cast<SDMJOpts>(getDelegate()->getOpts());
	return pSDMJOpts->isEnableHHQD();
}

uint8_t SDMJRoom::getRuleMode() {
	auto pSDMJOpts = std::dynamic_pointer_cast<SDMJOpts>(getDelegate()->getOpts());
	return pSDMJOpts->getRuleMode();
}

bool SDMJRoom::isEnableQiDui() {
	auto pSDMJOpts = std::dynamic_pointer_cast<SDMJOpts>(getDelegate()->getOpts());
	return pSDMJOpts->isEnableQiDui();
}

bool SDMJRoom::isEnableOnlyZM() {
	auto pSDMJOpts = std::dynamic_pointer_cast<SDMJOpts>(getDelegate()->getOpts());
	return pSDMJOpts->isEnableOnlyZiMo();
}

void SDMJRoom::addSettle(stSettle& tSettle) {
	m_vSettle.push_back(tSettle);

	Json::Value jsItem, jsRDetail;
	jsItem["actType"] = tSettle.eSettleReason;
	if (tSettle.eSettleReason == eMJAct_Hu) {
		jsItem["msg"] = tSettle.jsHuMsg;
	}

	uint32_t nTotalGain = 0;
	for (auto& refl : tSettle.vLoseIdx)
	{
		auto pPlayer = (SDMJPlayer*)getPlayerByIdx(refl.first);
		if (pPlayer) {
			auto nGain = pPlayer->addGuangSingleOffset(-1 * (int32_t)refl.second, getGuang());
			nTotalGain += nGain;
			if (nGain && tSettle.vWinIdxs.size()) {
				stSettleGain ssg;
				ssg.nGainChips = nGain;
				ssg.nTargetIdx = tSettle.vWinIdxs.begin()->first;
				addGain(refl.first, ssg);
			}
			Json::Value jsPlayer;
			jsPlayer["idx"] = refl.first;
			jsPlayer["chips"] = pPlayer->getChips();
			jsRDetail[jsRDetail.size()] = jsPlayer;
		}
	}

	for (auto& refl : tSettle.vWinIdxs)
	{
		auto pPlayer = getPlayerByIdx(refl.first);
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
			pPlayer->addSingleOffset(nTempWin);
			if (nTempWin) {
				backGain(refl.first);
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

void SDMJRoom::settleInfoToJson(Json::Value& jsRealTime) {
	for (auto& ref : m_vSettle) {
		Json::Value jsItem, jsRDetail;
		jsItem["actType"] = ref.eSettleReason;
		if (ref.eSettleReason == eMJAct_Hu) {
			jsItem["msg"] = ref.jsHuMsg;
		}

		for (auto& refl : ref.vLoseIdx)
		{
			auto pPlayer = (SDMJPlayer*)getPlayerByIdx(refl.first);
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

void SDMJRoom::sortFanxing2FanCnt(std::vector<eFanxingType>& vType, uint16_t& nFanCnt) {
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
		case eFanXing_SD_LuoDa:
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

void SDMJRoom::addGain(uint8_t nIdx, stSettleGain stGain) {
	if (nIdx < m_vGainChip.size()) {
		m_vGainChip[nIdx].push_back(stGain);
	}
}

void SDMJRoom::clearGain() {
	for (auto& ref : m_vGainChip) {
		ref.clear();
	}
}

void SDMJRoom::backGain(uint8_t nIdx) {
	if (nIdx < m_vGainChip.size()) {
		auto& vGain = m_vGainChip[nIdx];
		if (vGain.empty()) {
			return;
		}

		auto pLosePlayer = (SDMJPlayer*)getPlayerByIdx(nIdx);
		if (pLosePlayer == nullptr) {
			return;
		}
		for (auto& ref : vGain) {
			if (pLosePlayer->canBackGain(getGuang()) == false) {
				return;
			}
			auto pTargetPlayer = (SDMJPlayer*)getPlayerByIdx(ref.nTargetIdx);
			if (pTargetPlayer == nullptr) {
				continue;
			}
			if (ref.nGainChips == 0) {
				continue;
			}
			uint32_t nTempBack = ref.nGainChips - pLosePlayer->addGuangSingleOffset(-1 * (int32_t)ref.nGainChips, getGuang());
			ref.nGainChips -= nTempBack;
			pTargetPlayer->addSingleOffset(nTempBack);
			backGain(ref.nTargetIdx);
		}
	}
}

bool SDMJRoom::canStartGame() {
	for (auto ref : m_vPlayers) {
		if (ref && ref->haveState(eRoomPeer_Ready)) {
			continue;
		}
		return false;
	}

	return IMJRoom::canStartGame();
}

void SDMJRoom::doRandomChangeSeat() {
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

bool SDMJRoom::doChangeSeat(uint16_t nIdx, uint16_t nWithIdx) {
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

void SDMJRoom::setLastHuIdx(uint8_t nIdx) {
	m_nLastHuIdx = nIdx;
	m_bHuangZhuang = false;
}

void SDMJRoom::setYiPaoDuoXiang(uint8_t nIdx) {
	m_nYiPaoDuoXiang = nIdx;
	m_bHuangZhuang = false;
}