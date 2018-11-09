#include "CFMJRoom.h"
#include "CFMJPlayer.h"
#include "CommonDefine.h"
#include "MJRoomStateAutoStartWaitReady.h"
#include "MJRoomStateWaitPlayerChu.h"
#include "CFMJRoomStateWaitPlayerAct.h"
#include "CFMJRoomStateStartGame.h"
#include "MJRoomStateGameEnd.h"
#include "CFMJRoomStateDoPlayerAct.h"
#include "CFMJRoomStateAskForRobotGang.h"
#include "CFMJRoomStateAskForPengOrHu.h"
#include "CFMJRoomStateAfterChiOrPeng.h"
#include "MJReplayFrameType.h"
bool CFMJRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	IMJRoom::init(pRoomMgr,nSeialNum,nRoomID,nSeatCnt,vJsOpts);
	clearOneCircleEnd();
	// add room state ;
	IGameRoomState* p[] = { new MJRoomStateAutoStartWaitReady(), new MJRoomStateWaitPlayerChu(),new CFMJRoomStateWaitPlayerAct(),
		new CFMJRoomStateStartGame(),new MJRoomStateGameEnd(),new CFMJRoomStateDoPlayerAct(),new CFMJRoomStateAskForRobotGang(),
		new CFMJRoomStateAskForPengOrHu(), new CFMJRoomStateAfterChiOrPeng()};
	for ( auto& pS : p )
	{
		addRoomState(pS);
	}
	setInitState(p[0]);
	return true;
}

IGamePlayer* CFMJRoom::createGamePlayer()
{
	return new CFMJPlayer();
}

void CFMJRoom::packRoomInfo(Json::Value& jsRoomInfo) {
	IMJRoom::packRoomInfo(jsRoomInfo);
	jsRoomInfo["bankIdx"] = m_nBankerIdx;
	jsRoomInfo["curActIdex"] = getCurState()->getCurIdx();
	jsRoomInfo["waitTimer"] = getCurState()->getStateDuring();

	Json::Value jsPu;
	for (auto pPlayer : m_vPlayers) {
		if (pPlayer) {
			Json::Value jsPerPu;
			jsPerPu["idx"] = pPlayer->getIdx();
			jsPerPu["race"] = pPlayer->getRace();
			jsPu[jsPu.size()] = jsPerPu;
		}
	}
	jsRoomInfo["races"] = jsPu;
}

void CFMJRoom::visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID) {
	if (!pPlayer)
	{
		return;
	}

	IMJRoom::visitPlayerInfo(pPlayer, jsPlayerInfo, nVisitorSessionID);
	jsPlayerInfo["extraTime"] = ((CFMJPlayer*)pPlayer)->getExtraTime();

	if (pPlayer->haveState(eRoomPeer_CanAct) == false)  // not join this round game ;
	{
		return;
	}

	((CFMJPlayer*)pPlayer)->getPlayerCard()->onVisitPlayerCardInfo(jsPlayerInfo, pPlayer->getSessionID() == nVisitorSessionID);
}

void CFMJRoom::onWillStartGame() {
	IMJRoom::onWillStartGame();
	m_vSettle.clear();

	doProduceNewBanker();
}

void CFMJRoom::onStartGame() {
	IMJRoom::onStartGame();
	sendStartGameMsg();
}

void CFMJRoom::onGameEnd() {
	Json::Value jsReal, jsPlayers;
	bool isHuangZhuang = true;
	settleInfoToJson(jsReal, isHuangZhuang);
	if (isHuangZhuang) {
		setNextBankerIdx();
	}

	for (auto& ref : m_vPlayers) {
		if (ref) {
			if (isHuangZhuang) {
				ref->clearSingleOffset();
			}
			Json::Value jsHoldCard, jsPlayer;
			IMJPlayerCard::VEC_CARD vHoldCard;
			((CFMJPlayer*)ref)->getPlayerCard()->getHoldCard(vHoldCard);
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
	jsMsg["isLiuJu"] = isHuangZhuang ? 1 : 0;
	jsMsg["players"] = jsPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_SCMJ_GAME_END);

	IMJRoom::onGameEnd();
}

bool CFMJRoom::isCanGoOnMoPai() {
	return getPoker()->getLeftCardCount() > 16;
}

void CFMJRoom::onPlayerMo(uint8_t nIdx) {
	IMJRoom::onPlayerMo(nIdx);
	auto pPlayer = (CFMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->clearGangFlag();
}

void CFMJRoom::onPlayerChu(uint8_t nIdx, uint8_t nCard) {
	auto pPlayer = (CFMJPlayer*)getPlayerByIdx(nIdx);
	bool haveGangFlag = false;
	if (pPlayer->haveFlag(IMJPlayer::eMJActFlag::eMJActFlag_Gang)) {
		haveGangFlag = true;
	}
	IMJRoom::onPlayerChu(nIdx, nCard);
	if (haveGangFlag) {
		pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_Gang);
	}
}

void CFMJRoom::onPlayerPeng(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	IMJRoom::onPlayerPeng(nIdx, nCard, nInvokeIdx);

	auto pPlayer = (CFMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->clearGangFlag();
}

void CFMJRoom::onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	IMJRoom::onPlayerMingGang(nIdx, nCard, nInvokeIdx);

	stSettle st;
	st.eSettleReason = eMJAct_MingGang;
	uint16_t nWinCoin = 0;
	uint16_t nLoseCoin = getBaseScore();
	for (auto& pp : m_vPlayers) {
		if (pp) {
			if (pp->getIdx() == nIdx) {
				continue;
			}
			else {
				auto tLoseCoin = nLoseCoin;
				if (pp->getIdx() == getBankerIdx() || nIdx == getBankerIdx()) {
					tLoseCoin *= 2;
				}
				st.addLose(pp->getIdx(), tLoseCoin);
				nWinCoin += tLoseCoin;
			}
		}
	}
	st.addWin(nIdx, nWinCoin);
	addSettle(st);
}

void CFMJRoom::onPlayerAnGang(uint8_t nIdx, uint8_t nCard) {
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
				auto tLoseCoin = nLoseCoin;
				if (pp->getIdx() == getBankerIdx() || nIdx == getBankerIdx()) {
					tLoseCoin *= 2;
				}
				st.addLose(pp->getIdx(), tLoseCoin);
				nWinCoin += tLoseCoin;
			}
		}
	}
	st.addWin(nIdx, nWinCoin);
	addSettle(st);
}

void CFMJRoom::onPlayerBuGang(uint8_t nIdx, uint8_t nCard) {
	IMJRoom::onPlayerBuGang(nIdx, nCard);

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
				auto tLoseCoin = nLoseCoin;
				if (pp->getIdx() == getBankerIdx() || nIdx == getBankerIdx()) {
					tLoseCoin *= 2;
				}
				st.addLose(pp->getIdx(), tLoseCoin);
				nWinCoin += tLoseCoin;
			}
		}
	}
	st.addWin(nIdx, nWinCoin);
	addSettle(st);
}

void CFMJRoom::onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	if (vHuIdx.empty())
	{
		LOGFMTE("why hu vec is empty ? room id = %u", getRoomID());
		return;
	}

	Json::Value jsDetail, jsMsg;
	bool isZiMo = vHuIdx.front() == nInvokeIdx;
	jsMsg["isZiMo"] = isZiMo ? 1 : 0;
	jsMsg["huCard"] = nCard;
	stSettle st;
	st.eSettleReason = eMJAct_Hu;

	if (isZiMo) {
		setNextBankerIdx(nInvokeIdx);
		auto pZiMoPlayer = (CFMJPlayer*)getPlayerByIdx(nInvokeIdx);
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
		auto pZiMoPlayerCard = (CFMJPlayerCard*)pZiMoPlayer->getPlayerCard();
		pZiMoPlayerCard->getLastHuFanxing(vType);
		auto nFanCnt = pZiMoPlayerCard->getLastHuCnt();

		//自摸1番
		nFanCnt += 1;

		//杠开1番
		if (pZiMoPlayer->haveGangFlag())
		{
			nFanCnt += 1;
			vType.push_back(eFanxing_GangKai);
		}

		Json::Value jsHuTyps;
		for (auto& refHu : vType)
		{
			jsHuTyps[jsHuTyps.size()] = refHu;
		}
		jsDetail["vhuTypes"] = jsHuTyps;
		if (getFanLimit() && nFanCnt > getFanLimit()) {
			nFanCnt = getFanLimit();
		}
		uint32_t nLoseCoin = 1;
		for (int32_t i = 0; i < nFanCnt; ++i) nLoseCoin *= 2;

		uint32_t nTotalWin = 0;
		for (auto& pLosePlayer : m_vPlayers)
		{
			if (pLosePlayer) {
				if (pLosePlayer == pZiMoPlayer)
				{
					continue;
				}
				auto nTLoseCoin = nLoseCoin;
				if (pLosePlayer->getIdx() == getBankerIdx() || nInvokeIdx == getBankerIdx()) {
					nTLoseCoin *= 2;
				}

				nTLoseCoin += (pZiMoPlayer->getRace() + pLosePlayer->getRace());

				st.addLose(pLosePlayer->getIdx(), nTLoseCoin);
				nTotalWin += nTLoseCoin;
			}
		}
		st.addWin(nInvokeIdx, nTotalWin);
		LOGFMTD("room id = %u player = %u zimo", getRoomID(), nInvokeIdx);
	}
	else {
		auto pLosePlayer = (CFMJPlayer*)getPlayerByIdx(nInvokeIdx);
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
			for (uint8_t offset = 1; offset <= 3; ++offset)
			{
				auto nCheckIdx = nInvokeIdx + offset;
				nCheckIdx = nCheckIdx % 4;
				auto iter = std::find(vHuIdx.begin(), vHuIdx.end(), nCheckIdx);
				if (iter != vHuIdx.end())
				{
					vOrderHu.push_back(nCheckIdx);
					setNextBankerIdx(nCheckIdx);
					break;
				}
			}
		}

		Json::Value jsHuPlayers;
		uint32_t nTotalLose = 0;
		for (auto& nHuIdx : vOrderHu)
		{
			auto pHuPlayer = (CFMJPlayer*)getPlayerByIdx(nHuIdx);
			if (pHuPlayer == nullptr)
			{
				LOGFMTE("room id = %u hu player idx = %u , is nullptr", getRoomID(), nHuIdx);
				continue;
			}
			pHuPlayer->addHuCnt();

			Json::Value jsHuPlayer;
			jsHuPlayer["idx"] = nHuIdx;
			pHuPlayer->setState(eRoomPeer_AlreadyHu);

			auto pHuPlayerCard = (CFMJPlayerCard*)pHuPlayer->getPlayerCard();
			std::vector<eFanxingType> vType;
			pHuPlayerCard->getLastHuFanxing(vType);
			auto nFanCnt = pHuPlayerCard->getLastHuCnt();

			//pHuPlayerCard->onDoHu(nInvokeIdx, nCard, pLosePlayer->haveGangFlag());
			//bool isGHP = std::find(vType.begin(), vType.end(), eFanxing_GangHouPao) != vType.end();
			
			//抢杠1番(等同于自摸)
			bool bRotGang = pLosePlayer->haveFlag(IMJPlayer::eMJActFlag_DeclBuGang);
			if (bRotGang) {
				nFanCnt++;
				vType.push_back(eFanxing_QiangGang);
			}

			Json::Value jsHuTyps;
			for (auto& refHu : vType)
			{
				jsHuTyps[jsHuTyps.size()] = refHu;
			}
			jsHuPlayer["vhuTypes"] = jsHuTyps;

			if (getFanLimit() && nFanCnt > getFanLimit()) {
				nFanCnt = getFanLimit();
			}
			uint32_t nWinCoin = 1;
			for (uint16_t i = 0; i < nFanCnt; ++i) nWinCoin *= 2;

			uint32_t nAllWinCoin = 0;
			for (auto& ref : m_vPlayers) {
				if (ref && ref != pHuPlayer) {
					auto nPCoin = nWinCoin;
					if (ref->getIdx() == getBankerIdx() || nHuIdx == getBankerIdx()) {
						nPCoin *= 2;//庄1番
					}

					if (!bRotGang && ref == pLosePlayer) {
						nPCoin *= 2;//点炮非抢杠1番
					}

					if (bRotGang) {
						st.addLose(ref->getIdx(), nPCoin);
					}
					else {
						st.addLose(nInvokeIdx, nPCoin);
					}

					nAllWinCoin += nPCoin;
				}
			}

			st.addWin(nHuIdx, nAllWinCoin);
			jsHuPlayers[jsHuPlayers.size()] = jsHuPlayer;
			LOGFMTD("room id = %u player = %u hu", getRoomID(), nHuIdx);
		}

		jsDetail["huPlayers"] = jsHuPlayers;
	}
	jsMsg["detail"] = jsDetail;
	st.jsHuMsg = jsMsg;
	sendRoomMsg(jsMsg, MSG_ROOM_MQMJ_PLAYER_HU);

	/*for (auto huIdx : vHuIdx) {
	auto pHuPlayer = (MQMJPlayer*)getPlayerByIdx(huIdx);
	if (pHuPlayer == nullptr)
	{
	LOGFMTE("room id = %u hu player idx = %u , is nullptr", getRoomID(), huIdx);
	continue;
	}
	auto pHuCard = (MQMJPlayerCard*)pHuPlayer->getPlayerCard();
	pHuCard->endDoHu(nCard);
	pHuCard->addHuCard(nCard);
	}*/

	addSettle(st);
}

void CFMJRoom::onPlayerLouHu(uint8_t nIdx, uint8_t nInvokerIdx) {
	IMJRoom::onPlayerLouHu(nIdx, nInvokerIdx);

	((CFMJPlayerCard*)((CFMJPlayer*)getPlayerByIdx(nIdx))->getPlayerCard())->onLouHu();
}

void CFMJRoom::sendStartGameMsg() {
	// bind room to player card 
	Json::Value jsMsg, jsPu;

	for (auto& pPlayer : m_vPlayers)
	{
		if (pPlayer == nullptr)
		{
			LOGFMTE("room id = %u , start game player is nullptr", getRoomID());
			continue;
		}
		Json::Value jsPerPu;
		auto pPlayerCard = ((CFMJPlayer*)pPlayer)->getPlayerCard();
		jsPerPu["idx"] = pPlayer->getIdx();
		jsPerPu["race"] = pPlayer->getRace();
		jsPu[jsPu.size()] = jsPerPu;
	}
	jsMsg["races"] = jsPu;
	sendRoomMsg(jsMsg, MSG_ROOM_MQMJ_GAME_START);

	// replay arg 
	addReplayFrame(eMJFrame_Xia_Zhu, jsMsg);
}

void CFMJRoom::doProduceNewBanker() {
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

void CFMJRoom::setNextBankerIdx(uint8_t nHuIdx) {
	if ((uint8_t)-1 == getBankerIdx()) {
		m_nNextBankerIdx = 0;
	}
	else {
		m_nNextBankerIdx = nHuIdx == getBankerIdx() ? nHuIdx : getNextActPlayerIdx(getBankerIdx());

		if (m_nNextBankerIdx == 0 && getBankerIdx()) {
			signOneCircleEnd();
		}
	}

	if ((uint8_t)-1 == m_nNextBankerIdx) {
		m_nNextBankerIdx = 0;
	}
}

uint8_t CFMJRoom::getRoomType()
{
	return eGame_CFMJ;
}

IPoker* CFMJRoom::getPoker()
{
	return &m_tPoker;
}

bool CFMJRoom::isHaveRace() {
	return true;
}

void CFMJRoom::onWaitRace(uint8_t nIdx) {
	Json::Value jsMsg, jsonRace;
	jsonRace[jsonRace.size()] = 0;
	jsonRace[jsonRace.size()] = 2;
	jsonRace[jsonRace.size()] = 4;
	jsMsg["races"] = jsonRace;
	if ((uint8_t)-1 == nIdx) {
		sendRoomMsg(jsMsg, MSG_ROOM_CF_GUA_PU);
	}
	else {
		auto pPlayer = getPlayerByIdx(nIdx);
		if (!pPlayer)
		{
			LOGFMTE("player idx = %u is null can not tell it gua pu act", nIdx);
			return;
		}
		sendMsgToPlayer(jsMsg, MSG_ROOM_CF_GUA_PU, pPlayer->getSessionID());
	}
}

void CFMJRoom::onAskForPengOrHuThisCard(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint16_t>& vOutWaitHuIdx, std::vector<uint16_t>& vOutWaitPengGangIdx, bool& isNeedWaitEat) {
	for (uint8_t i = 1; i < getSeatCnt(); i++)
	{
		uint8_t idx = (nInvokeIdx + i) % getSeatCnt();
		auto ref = (CFMJPlayer*)getPlayerByIdx(idx);

		if (ref == nullptr || nInvokeIdx == ref->getIdx())
		{
			continue;
		}
		auto pMJCard = ref->getPlayerCard();

		// check peng 
		if (pMJCard->canPengWithCard(nCard))
		{
			vOutWaitPengGangIdx.push_back(ref->getIdx());
		}

		// check ming gang 
		if (isCanGoOnMoPai() && pMJCard->canMingGangWithCard(nCard))
		{
			// already add in peng ;  vWaitPengGangIdx
			if (vOutWaitPengGangIdx.empty())
			{
				vOutWaitPengGangIdx.push_back(ref->getIdx());
			}
		}

		if (ref->getIdx() == (nInvokeIdx + 1) % getSeatCnt())
		{
			uint8_t a = 0, b = 0;
			isNeedWaitEat = false;
			if (pMJCard->canEatCard(nCard, a, b))
			{
				isNeedWaitEat = true;
			}
		}

		// check hu ;
		if (pMJCard->canHuWitCard(nCard))
		{
			vOutWaitHuIdx.push_back(ref->getIdx());
		}
	}
}

void CFMJRoom::onAskForRobotGang(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates) {
	for (uint8_t i = 1; i < getSeatCnt(); i++)
	{
		uint8_t idx = (nInvokeIdx + i) % getSeatCnt();
		auto ref = (CFMJPlayer*)getPlayerByIdx(idx);
		if (ref == nullptr || nInvokeIdx == ref->getIdx())
		{
			continue;
		}
		auto pMJCard = ref->getPlayerCard();
		// check hu
		if (pMJCard->canHuWitCard(nCard))
		{
			vOutCandinates.push_back(ref->getIdx());
		}
	}
}

bool CFMJRoom::needChu() {
	return getPoker()->getLeftCardCount() >= 20;
}

bool CFMJRoom::canGang() {
	return needChu();
}

uint8_t CFMJRoom::getFanLimit() {
	uint8_t nFanLimit = 0;
	if (m_jsOpts["fanLimit"].isNull() == false && m_jsOpts["fanLimit"].isUInt()) {
		nFanLimit = m_jsOpts["fanLimit"].asUInt();
	}
	return nFanLimit;
}

uint8_t CFMJRoom::getBaseScore() {
	return m_jsOpts["baseScore"].isUInt() ? m_jsOpts["baseScore"].asUInt() : 1;
}

bool CFMJRoom::isCircle() {
	return m_jsOpts["circle"].asBool();
}

void CFMJRoom::addSettle(stSettle& tSettle) {
	m_vSettle.push_back(tSettle);
}

void CFMJRoom::settleInfoToJson(Json::Value& jsRealTime, bool& bHuangZhuang) {
	for (auto& ref : m_vSettle) {
		Json::Value jsItem, jsRDetail;
		jsItem["actType"] = ref.eSettleReason;
		if (ref.eSettleReason == eMJAct_Hu) {
			jsItem["msg"] = ref.jsHuMsg;
			bHuangZhuang = false;
		}

		uint32_t nTotalGain = 0;
		for (auto& refl : ref.vLoseIdx)
		{
			auto pPlayer = getPlayerByIdx(refl.first);
			if (pPlayer) {
				pPlayer->addSingleOffset(-1 * (int32_t)refl.second);
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
				pPlayer->addSingleOffset(refl.second);
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

bool CFMJRoom::onWaitPlayerActAfterCP(uint8_t nIdx) {
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer) {
		LOGFMTE("player idx = %u is null can not tell it wait act", nIdx);
		return false;
	}

	if (pPlayer->haveState(eRoomPeer_AlreadyHu)) {
		LOGFMTE("player idx = %u is alreay hu, why wait him to act", nIdx);
		return false;
	}

	if (!pPlayer)
	{
		LOGFMTE("player idx = %u is null can not tell it wait act", nIdx);
		return false;
	}

	bool flag = false;
	if (isCanGoOnMoPai() && canGang()) {
		Json::Value jsArrayActs, jsFrameActs;
		auto pMJCard = pPlayer->getPlayerCard();
		IMJPlayerCard::VEC_CARD vCards;

		// check bu gang
		if (pMJCard->getHoldCardThatCanBuGang(vCards)) {
			for (auto& ref : vCards)
			{
				Json::Value jsAct;
				jsAct["act"] = eMJAct_BuGang;
				jsAct["cardNum"] = ref;
				jsArrayActs[jsArrayActs.size()] = jsAct;
				jsFrameActs[jsFrameActs.size()] = eMJAct_BuGang;
			}
			flag = true;
		}
		vCards.clear();

		// check an gang .
		if (pMJCard->getHoldCardThatCanAnGang(vCards)) {
			for (auto& ref : vCards)
			{
				Json::Value jsAct;
				jsAct["act"] = eMJAct_AnGang;
				jsAct["cardNum"] = ref;
				jsArrayActs[jsArrayActs.size()] = jsAct;
				jsFrameActs[jsFrameActs.size()] = eMJAct_AnGang;
			}
			flag = true;
		}
		vCards.clear();

		if (flag) {
			Json::Value jsMsg;
			jsMsg["acts"] = jsArrayActs;
			sendMsgToPlayer(jsMsg, MSG_ROOM_MQMJ_WAIT_ACT_AFTER_CP, pPlayer->getSessionID());

			Json::Value jsFrameArg;
			jsFrameArg["idx"] = nIdx;
			jsFrameArg["act"] = jsFrameActs;
			addReplayFrame(eMJFrame_WaitPlayerAct, jsFrameActs);
		}
	}
	return flag;
}