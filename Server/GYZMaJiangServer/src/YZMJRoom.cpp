#include "YZMJRoom.h"
#include "YZMJPlayer.h"
#include "CommonDefine.h"
#include "YZMJRoomStateWaitReady.h"
#include "YZMJRoomStateWaitPlayerChu.h"
#include "YZMJRoomStateWaitPlayerAct.h"
#include "MJRoomStateStartGame.h"
#include "MJRoomStateGameEnd.h"
#include "YZMJRoomStateDoPlayerAct.h"
#include "YZMJRoomStateAskForRobotGang.h"
#include "YZMJRoomStateAskForPengOrHu.h"
#include "FanxingChecker.h"
#include "Fanxing7Dui.h"
#include "FanxingDuiDuiHu.h"
#include "MJReplayFrameType.h"
#include "YZMJPoker.h"
#include "IGameRoomDelegate.h"
#include "YZMJOpts.h"

bool YZMJRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)
{
	IMJRoom::init(pRoomMgr,nSeialNum,nRoomID,ptrGameOpts);
	m_vGainChip.resize(getSeatCnt());
	m_nDice = 0;
	m_nPeiZiCard = 0;
	// add room state ;
	IGameRoomState* p[] = { new YZMJRoomStateWaitReady(), new YZMJRoomStateWaitPlayerChu(),new YZMJRoomStateWaitPlayerAct(),
		new MJRoomStateStartGame(),new MJRoomStateGameEnd(),new YZMJRoomStateDoPlayerAct(),new YZMJRoomStateAskForRobotGang(),
		new YZMJRoomStateAskForPengOrHu()
	};
	for ( auto& pS : p )
	{
		addRoomState(pS);
	}
	setInitState(p[0]);
	return true;
}

IGamePlayer* YZMJRoom::createGamePlayer()
{
	return new YZMJPlayer();
}

void YZMJRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	IMJRoom::packRoomInfo(jsRoomInfo);
	jsRoomInfo["bankIdx"] = m_nBankerIdx;
	jsRoomInfo["curActIdex"] = getCurState()->getCurIdx();
	jsRoomInfo["waitTimer"] = getCurState()->getStateDuring();
	jsRoomInfo["dice"] = m_nDice;
	jsRoomInfo["da"] = getDa();
}

void YZMJRoom::visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)
{
	if (!pPlayer)
	{
		return;
	}

	IMJRoom::visitPlayerInfo(pPlayer, jsPlayerInfo, nVisitorSessionID);
	jsPlayerInfo["extraTime"] = ((YZMJPlayer*)pPlayer)->getExtraTime();

	if (pPlayer->haveState(eRoomPeer_CanAct) == false)  // not join this round game ;
	{
		return;
	}

	((YZMJPlayer*)pPlayer)->getPlayerCard()->onVisitPlayerCardInfo(jsPlayerInfo, pPlayer->getSessionID() == nVisitorSessionID);
}

void YZMJRoom::onWillStartGame() {
	IMJRoom::onWillStartGame();
	m_vSettle.clear();
	m_nDice = 0;

	doProduceNewBanker();
}

void YZMJRoom::confirmDa() {
	m_nPeiZiCard = 0;
	if (isPeiZi()) {
		if (isBaiBanPeiZi()) {
			m_nPeiZiCard = make_Card_Num(eCT_Jian, 3);
		}
		else {
			m_nPeiZiCard = getPoker()->distributeOneCard();
		}
	}
}

void YZMJRoom::onStartGame()
{
	IMJRoom::onStartGame();
	confirmDa();
	sendStartGameMsg();
}

void YZMJRoom::sendStartGameMsg() {
	Json::Value jsMsg;
	m_nDice = 1 + rand() % 6;
	m_nDice = m_nDice * 10 + (1 + rand() % 6);
	jsMsg["dice"] = m_nDice;
	jsMsg["bankerIdx"] = getBankerIdx();
	jsMsg["da"] = getDa();
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
		auto pPlayerCard = ((YZMJPlayer*)pPlayer)->getPlayerCard();
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

void YZMJRoom::packStartGameReplyInfo(Json::Value& jsFrameArg) {
	IMJRoom::packStartGameReplyInfo(jsFrameArg);
	jsFrameArg["da"] = getDa();
}

uint8_t YZMJRoom::getRoomType()
{
	return eGame_YZMJ;
}

IPoker* YZMJRoom::getPoker()
{
	return &m_tPoker;
}

void YZMJRoom::onPlayerLouHu(uint8_t nIdx, uint8_t nInvokerIdx) {
	IMJRoom::onPlayerLouHu(nIdx, nInvokerIdx);

	((YZMJPlayerCard*)((YZMJPlayer*)getPlayerByIdx(nIdx))->getPlayerCard())->onLouHu();
}

bool YZMJRoom::needChu() {
	return true;
}

bool YZMJRoom::isGameOver() {
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

bool YZMJRoom::isRoomOver() {
	if (isInternalShouldCloseAll()) {
		return true;
	}
	return IMJRoom::isRoomOver();
}

bool YZMJRoom::isInternalShouldCloseAll() {
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

void YZMJRoom::onGameEnd() {
	Json::Value jsReal, jsPlayers;
	settleInfoToJson(jsReal);

	for (auto& ref : m_vPlayers) {
		if (ref) {
			Json::Value jsHoldCard, jsPlayer;
			IMJPlayerCard::VEC_CARD vHoldCard;
			((YZMJPlayer*)ref)->getPlayerCard()->getHoldCard(vHoldCard);
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

void YZMJRoom::doProduceNewBanker() {
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
}

void YZMJRoom::setNextBankerIdx(uint8_t nHuIdx) {
	if ((uint8_t)-1 == getBankerIdx()) {
		m_nNextBankerIdx = 0;
	}
	else {
		if ((uint8_t)-1 == nHuIdx) {
			m_nNextBankerIdx = getBankerIdx();
		}
		else {
			m_nNextBankerIdx = nHuIdx == getBankerIdx() ? nHuIdx : getNextActPlayerIdx(getBankerIdx());
		}
	}

	if ((uint8_t)-1 == m_nNextBankerIdx) {
		m_nNextBankerIdx = 0;
	}
}

void YZMJRoom::onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	IMJRoom::onPlayerMingGang(nIdx, nCard, nInvokeIdx);

	stSettle st;
	st.eSettleReason = eMJAct_MingGang;
	uint16_t nWinCoin = 2 * getBaseScore();
	//uint16_t nLoseCoin = 2 * getBaseScore();
	/*for (auto& pp : m_vPlayers) {
		if (pp) {
			if (pp->getIdx() == nIdx) {
				continue;
			}
			else {
				st.addLose(pp->getIdx(), nLoseCoin);
				nWinCoin += nLoseCoin;
			}
		}
	}*/
	st.addLose(nInvokeIdx, nWinCoin);
	st.addWin(nIdx, nWinCoin);
	addSettle(st);
}

void YZMJRoom::onPlayerAnGang(uint8_t nIdx, uint8_t nCard) {
	IMJRoom::onPlayerAnGang(nIdx, nCard);

	stSettle st;
	st.eSettleReason = eMJAct_AnGang;
	uint16_t nWinCoin = 0;
	uint16_t nLoseCoin = 2 * getBaseScore();
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

	auto pPlayer = (YZMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_NeedClearCanCyclone);
}

void YZMJRoom::onPlayerBuGang(uint8_t nIdx, uint8_t nCard) {
	auto pPlayer = (YZMJPlayer*)getPlayerByIdx(nIdx);
	if (pPlayer) {
		auto pCard = pPlayer->getPlayerCard();
		if (pCard->getNewestFetchedCard() == nCard) {
			stSettle st;
			st.eSettleReason = eMJAct_BuGang;
			uint16_t nWinCoin = 0;
			uint16_t nLoseCoin = getBaseScore();
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
	IMJRoom::onPlayerBuGang(nIdx, nCard);
}

void YZMJRoom::onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	if (vHuIdx.empty())
	{
		LOGFMTE("why hu vec is empty ? room id = %u", getRoomID());
		return;
	}

	//三个搬子算暗杠
	for (auto pCheckPlayer : m_vPlayers)
	{
		auto pCheckPlayerCard = (YZMJPlayerCard*)((YZMJPlayer*)pCheckPlayer)->getPlayerCard();

		if (isPeiZi() && !isBaiBanPeiZi() && pCheckPlayerCard->isHaveDa() >= 3)
		{
			// do caculate ;
			stSettle st;
			st.eSettleReason = eMJAct_AnGang;
			uint16_t nWinCoin = 0;
			uint16_t nLoseCoin = 2 * getBaseScore();
			for (auto& pp : m_vPlayers) {
				if (pp) {
					if (pp->getIdx() == pCheckPlayer->getIdx()) {
						continue;
					}
					else {
						st.addLose(pp->getIdx(), nLoseCoin);
						nWinCoin += nLoseCoin;
					}
				}
			}
			st.addWin(pCheckPlayer->getIdx(), nWinCoin);
			addSettle(st);
		}
	}

	Json::Value jsDetail, jsMsg;
	bool isZiMo = vHuIdx.front() == nInvokeIdx;
	jsMsg["isZiMo"] = isZiMo ? 1 : 0;
	jsMsg["huCard"] = nCard;
	stSettle st;
	st.eSettleReason = eMJAct_Hu;

	if (isZiMo) {
		setNextBankerIdx(nInvokeIdx);
		auto pZiMoPlayer = (YZMJPlayer*)getPlayerByIdx(nInvokeIdx);
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
		auto pZiMoPlayerCard = (YZMJPlayerCard*)pZiMoPlayer->getPlayerCard();
		pZiMoPlayerCard->setHuCard(nCard);
		pZiMoPlayerCard->getLastHuFanxing(vType);
		auto nFanCnt = pZiMoPlayerCard->getLastHuCnt();
		pZiMoPlayer->setBestCards(nFanCnt);
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
		auto pLosePlayer = (YZMJPlayer*)getPlayerByIdx(nInvokeIdx);
		if (!pLosePlayer)
		{
			LOGFMTE("room id = %u lose but player idx = %u is nullptr", getRoomID(), nInvokeIdx);
			return;
		}
		jsDetail["dianPaoIdx"] = pLosePlayer->getIdx();
		pLosePlayer->addDianPaoCnt();
		std::vector<uint8_t> vOrderHu;
		if (vHuIdx.size() > 0)
		{
			auto iterBankWin = std::find(vHuIdx.begin(), vHuIdx.end(), getBankerIdx());
			if (iterBankWin == vHuIdx.end()) {
				setNextBankerIdx(vHuIdx.front());
			}
			else {
				setNextBankerIdx(getBankerIdx());
			}

			for (uint8_t offset = 1; offset <= 3; ++offset)
			{
				auto nCheckIdx = nInvokeIdx + offset;
				nCheckIdx = nCheckIdx % 4;
				auto iter = std::find(vHuIdx.begin(), vHuIdx.end(), nCheckIdx);
				if (iter != vHuIdx.end())
				{
					vOrderHu.push_back(nCheckIdx);
				}
			}
		}

		Json::Value jsHuPlayers;
		uint32_t nTotalLose = 0;
		for (auto& nHuIdx : vOrderHu)
		{
			auto pHuPlayer = (YZMJPlayer*)getPlayerByIdx(nHuIdx);
			if (pHuPlayer == nullptr)
			{
				LOGFMTE("room id = %u hu player idx = %u , is nullptr", getRoomID(), nHuIdx);
				continue;
			}
			pHuPlayer->addHuCnt();

			Json::Value jsHuPlayer;
			jsHuPlayer["idx"] = nHuIdx;
			pHuPlayer->setState(eRoomPeer_AlreadyHu);

			auto pHuPlayerCard = (YZMJPlayerCard*)pHuPlayer->getPlayerCard();
			pHuPlayerCard->setHuCard(nCard);

			pHuPlayerCard->onDoHu(nInvokeIdx, nCard, pLosePlayer->haveGangFlag());
			std::vector<eFanxingType> vType;
			pHuPlayerCard->getLastHuFanxing(vType);
			auto nFanCnt = pHuPlayerCard->getLastHuCnt();
			pHuPlayer->setBestCards(nFanCnt);

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

uint8_t YZMJRoom::getNextActPlayerIdx(uint8_t nCurActIdx) {
	return (nCurActIdx + 1) % getSeatCnt();
}

bool YZMJRoom::isPeiZi() {
	auto pYZMJOpts = std::dynamic_pointer_cast<YZMJOpts>(getDelegate()->getOpts());
	return pYZMJOpts->isPeiZi();
}

bool YZMJRoom::isBaiBanPeiZi() {
	auto pYZMJOpts = std::dynamic_pointer_cast<YZMJOpts>(getDelegate()->getOpts());
	return pYZMJOpts->isBaiBanPeiZi();
}

bool YZMJRoom::isYiTiaoLong() {
	auto pYZMJOpts = std::dynamic_pointer_cast<YZMJOpts>(getDelegate()->getOpts());
	return pYZMJOpts->isYiTiaoLong();
}

bool YZMJRoom::isQiDui() {
	auto pYZMJOpts = std::dynamic_pointer_cast<YZMJOpts>(getDelegate()->getOpts());
	return pYZMJOpts->isQiDui();
}

uint32_t YZMJRoom::getGuang() {
	auto pYZMJOpts = std::dynamic_pointer_cast<YZMJOpts>(getDelegate()->getOpts());
	return pYZMJOpts->getGuang();
}

void YZMJRoom::addSettle(stSettle& tSettle) {
	m_vSettle.push_back(tSettle);

	Json::Value jsItem, jsRDetail;
	jsItem["actType"] = tSettle.eSettleReason;
	if (tSettle.eSettleReason == eMJAct_Hu) {
		jsItem["msg"] = tSettle.jsHuMsg;
	}

	uint32_t nTotalGain = 0;
	for (auto& refl : tSettle.vLoseIdx)
	{
		auto pPlayer = (YZMJPlayer*)getPlayerByIdx(refl.first);
		if (pPlayer) {
			auto nGain = pPlayer->addGuangSingleOffset(-1 * (int32_t)refl.second, getGuang());
			nTotalGain += nGain;
			/*if (nGain && tSettle.vWinIdxs.size()) {
				stSettleGain ssg;
				ssg.nGainChips = nGain;
				ssg.nTargetIdx = tSettle.vWinIdxs.begin()->first;
				addGain(refl.first, ssg);
			}*/
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
			/*if (nTempWin) {
				backGain(refl.first);
			}*/
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

void YZMJRoom::settleInfoToJson(Json::Value& jsRealTime) {
	for (auto& ref : m_vSettle) {
		Json::Value jsItem, jsRDetail;
		jsItem["actType"] = ref.eSettleReason;
		if (ref.eSettleReason == eMJAct_Hu) {
			jsItem["msg"] = ref.jsHuMsg;
		}

		//uint32_t nTotalGain = 0;
		for (auto& refl : ref.vLoseIdx)
		{
			auto pPlayer = (YZMJPlayer*)getPlayerByIdx(refl.first);
			if (pPlayer) {
				/*auto nGain = pPlayer->addGuangSingleOffset(-1 * (int32_t)refl.second, getGuang());
				nTotalGain += nGain;
				if (nGain && ref.vWinIdxs.size()) {
					stSettleGain ssg;
					ssg.nGainChips = nGain;
					ssg.nTargetIdx = ref.vWinIdxs[0];
					addGain(refl.first, ssg);
				}*/
				Json::Value jsPlayer;
				jsPlayer["idx"] = refl.first;
				jsPlayer["offset"] = -1 * refl.second;
				jsRDetail[jsRDetail.size()] = jsPlayer;
			}
		}

		for (auto& refl : ref.vWinIdxs)
		{
			auto pPlayer = getPlayerByIdx(refl.first);
			/*uint32_t nTempWin = 0;
			if (nTotalGain > refl.second) {
				nTempWin = 0;
				nTotalGain -= refl.second;
			}
			else {
				nTempWin = refl.second - nTotalGain;
				nTotalGain = 0;
			}*/
			if (pPlayer) {
				/*pPlayer->addSingleOffset(nTempWin);
				if (nTempWin) {
					backGain(refl.first);
				}*/
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

void YZMJRoom::sortFanxing2FanCnt(std::vector<eFanxingType>& vType, uint16_t& nFanCnt) {
	for (auto& ref : vType) {
		switch (ref)
		{
		case eFanxing_GangKai:
		case eFanxing_JiaHu:
		//case eFanxing_QiangGang:
		//case eFanxing_GangHouPao:
		case eFanxing_HaiDiLaoYue:
		case eFanxing_MengQing:
		{
			nFanCnt += 1;
			break;
		}
		case eFanxing_BianHu:
		{
			nFanCnt += 2;
			break;
		}
		case eFanxing_DuiDuiHu:
		{
			nFanCnt += 3;
			break;
		}
		}
	}
}

bool YZMJRoom::canStartGame() {
	for (auto ref : m_vPlayers) {
		if (ref && ref->haveState(eRoomPeer_Ready)) {
			continue;
		}
		return false;
	}

	return IMJRoom::canStartGame();
}

void YZMJRoom::doRandomChangeSeat() {
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

bool YZMJRoom::doChangeSeat(uint16_t nIdx, uint16_t nWithIdx) {
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