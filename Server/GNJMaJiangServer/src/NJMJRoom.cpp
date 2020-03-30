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
#include "NJMJRoomStateDoPlayerAct.h"
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
	clearOneCircleEnd();
	m_cFollowCards.setSeatCnt(ptrGameOpts->getSeatCnt());
	m_bWillFanBei = false;
	m_bFanBei = false;
	m_nLeiBaTaCnt = 0;
	m_nGangCnt = 0;
	m_vBaoMiIdx.clear();
	// add room state ;
	IGameRoomState* p[] = { new CMJRoomStateWaitReady(), new NJMJRoomStateWaitPlayerChu(),new NJMJRoomStateWaitPlayerAct(),
		new NJMJRoomStateStartGame(),new MJRoomStateGameEnd(),new NJMJRoomStateDoPlayerAct(),new NJMJRoomStateAskForRobotGang(),
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
	m_vBaoMiIdx.clear();

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
	bool bHuangZhuang = true;

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
			if (isEnableWaiBao()) {
				jsPlayer["extraOffset"] = ((NJMJPlayer*)ref)->getExtraOffset();
			}
			jsPlayers[jsPlayers.size()] = jsPlayer;

			if (ref->haveState(eRoomPeer_AlreadyHu)) {
				bHuangZhuang = false;
			}
		}
		//jsHoldCards[jsHoldCards.size()] = jsHoldCard;
	}

	if (bHuangZhuang) {
		m_bWillFanBei = true;
	}

	Json::Value jsMsg;
	jsMsg["realTimeCal"] = jsReal;
	jsMsg["players"] = jsPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_SCMJ_GAME_END);

	IMJRoom::onGameEnd();

	if (isInternalShouldCloseAll() && m_vBaoMiIdx.size()) {
		NJMJPlayer* pWinMost = nullptr;
		NJMJPlayer* pBaoMi = nullptr;
		uint8_t nGuangCnt = 0;
		for (auto ref : m_vPlayers) {
			auto ref_nj = (NJMJPlayer*)ref;

			if (ref_nj->canBackGain(getGuang())) {
				if (ref_nj->getChips() > 0) {
					pWinMost = ref_nj;
				}
				else {
					pBaoMi = ref_nj;
				}
			}
			else {
				nGuangCnt++;
			}
		}

		if (pBaoMi) {
			if (std::find(m_vBaoMiIdx.begin(), m_vBaoMiIdx.end(), pBaoMi->getIdx()) == m_vBaoMiIdx.end()) {
				pBaoMi = nullptr;
			}
		}
		else {
			if (pWinMost && nGuangCnt + 1 == getSeatCnt()) {
				for (auto rIter = m_vBaoMiIdx.rbegin(); rIter != m_vBaoMiIdx.rend(); rIter++) {
					auto ref = *rIter;
					if (ref == pWinMost->getIdx()) {
						continue;
					}
					pBaoMi = (NJMJPlayer*)getPlayerByIdx(ref);
					break;
				}
			}
		}

		if (pBaoMi && pWinMost) {
			if (pBaoMi->getChips() < 0 && pWinMost->getChips() > 0) {
				auto nBaoMiChips = (uint32_t)abs(pBaoMi->getChips());
				if (nBaoMiChips <= getGuang()) {
					uint32_t nOffset = getGuang() - nBaoMiChips;
					if (nOffset < pWinMost->getChips()) {
						pBaoMi->addSingleOffset(nOffset);
						pWinMost->addSingleOffset(-1 * (int32_t)nOffset);
						pBaoMi->signBaoMi();
					}
				}
			}
		}
	}
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
				m_bWillFanBei = true;
			}
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
	onPlayerAfterChu(nIdx, nCard);
}

void NJMJRoom::onPlayerChu(uint8_t nIdx, uint8_t nCard, uint8_t& nTing) {
	auto pActPlayer = (NJMJPlayer*)getPlayerByIdx(nIdx);
	if (nTing) {
		if (pActPlayer->haveFlag(IMJPlayer::eMJActFlag_WaitCheckTianTing) == false) {
			nTing = 0;
		}
	}

	IMJRoom::onPlayerChu(nIdx, nCard, nTing);

	if (nTing) {
		auto pCard = (NJMJPlayerCard*)pActPlayer->getPlayerCard();
		pCard->signTing();
	}

	onPlayerAfterChu(nIdx, nCard);
}

void NJMJRoom::onPlayerAfterChu(uint8_t nIdx, uint8_t nCard) {
	auto pActPlayer = (NJMJPlayer*)getPlayerByIdx(nIdx);
	pActPlayer->setSongGangIdx();
	pActPlayer->clearFlag(IMJPlayer::eMJActFlag_WaitCheckTianTing);
	pActPlayer->clearFlag(IMJPlayer::eMJActFlag_CanTianHu);

	auto pActCard = (NJMJPlayerCard*)pActPlayer->getPlayerCard();
	if (isEnableSiLianFeng()) {
		if (pActCard->isSiLianFeng()) {
			m_bWillFanBei = true;
			//TODO
			stSettle st;
			st.eSettleReason = eMJAct_4Feng;
			st.nBeginIdx = nIdx;
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
		st.nBeginIdx = nIdx;
		st.bOneLose = true;
		uint16_t nWinCoin = 0;
		uint16_t nLoseCoin = 5 * getBaseScore();
		for (auto& pp : m_vPlayers) {
			if (pp) {
				if (pp->getIdx() == nIdx) {
					continue;
				}
				else {
					st.addWin(pp->getIdx(), nLoseCoin);
					nWinCoin += nLoseCoin;
				}
			}
		}
		st.addLose(nIdx, nWinCoin);
		addSettle(st);
	}

	if (m_cFollowCards.onChuCard(nIdx, nCard)) {
		auto pLosePlayer = (NJMJPlayer*)getPlayerByIdx(m_cFollowCards.m_nFirstIdx);
		if (pLosePlayer) {
			m_bWillFanBei = true;
			//TODO
			stSettle st;
			st.eSettleReason = eMJAct_Followed;
			st.nBeginIdx = nIdx;
			st.bOneLose = true;
			uint16_t nWinCoin = 5 * getBaseScore();
			uint16_t nLoseCoin = 0;

			for (auto& pp : m_vPlayers) {
				if (pp) {
					if (pp->getIdx() == nIdx) {
						continue;
					}
					else {
						st.addWin(pp->getIdx(), nWinCoin);
						nLoseCoin += nWinCoin;
					}
				}
			}
			st.addLose(nIdx, nLoseCoin);
			addSettle(st);
		}
	}
}

void NJMJRoom::onPlayerPeng(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	IMJRoom::onPlayerPeng(nIdx, nCard, nInvokeIdx);

	auto pActPlayer = (NJMJPlayer*)getPlayerByIdx(nIdx);
	pActPlayer->clearFlag(IMJPlayer::eMJActFlag_WaitCheckTianTing);
	pActPlayer->clearFlag(IMJPlayer::eMJActFlag_CanTianHu);
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
	pActPlayer->clearFlag(IMJPlayer::eMJActFlag_CanTianHu);

	stSettle st;
	st.eSettleReason = eMJAct_MingGang;
	st.nBeginIdx = nIdx;
	uint16_t nLoseCoin = 10 * getBaseScore();
	st.addWin(nIdx, nLoseCoin);
	st.addLose(nInvokeIdx, nLoseCoin);
	addSettle(st);
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

		stSettle st;
		st.eSettleReason = eMJAct_BuGang;
		st.nBeginIdx = nIdx;
		uint16_t nLoseCoin = 10 * getBaseScore();
		st.addWin(nIdx, nLoseCoin);
		st.addLose(nInvokerIdx, nLoseCoin);
		addSettle(st);
	}
}

void NJMJRoom::onPlayerAnGang(uint8_t nIdx, uint8_t nCard) {
	IMJRoom::onPlayerAnGang(nIdx, nCard);
	auto pActPlayer = (NJMJPlayer*)getPlayerByIdx(nIdx);
	pActPlayer->clearFlag(IMJPlayer::eMJActFlag_WaitCheckTianTing);
	pActPlayer->clearFlag(IMJPlayer::eMJActFlag_CanTianHu);

	if (isEnableShuangGang()) {
		m_bWillFanBei = true;
	}

	stSettle st;
	st.eSettleReason = eMJAct_AnGang;
	st.nBeginIdx = nIdx;
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

bool NJMJRoom::onPlayerHuaGang(uint8_t nIdx, uint8_t nCard) {
	auto pPlayer = (NJMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->signFlag(IMJPlayer::eMJActFlag_HuaGang);
	m_bWillFanBei = true;

	stSettle st;
	st.eSettleReason = eMJAct_HuaGang;
	st.nBeginIdx = nIdx;
	uint16_t nWinCoin = 0;
	uint16_t nLoseCoin = 10 * getBaseScore();
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

	return true;
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

	if (isZiMo) {
		Json::Value stJson;
		stJson = jsMsg;
		stSettle st;
		st.eSettleReason = eMJAct_Hu;
		st.nBeginIdx = nInvokeIdx;

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
		stJson["huIdx"] = nInvokeIdx;
		stSortFanInformation stHuInfomation;
		auto pZiMoPlayerCard = (NJMJPlayerCard*)pZiMoPlayer->getPlayerCard();
		pZiMoPlayerCard->setHuCard(0);
		checkFanxing(pZiMoPlayer, nInvokeIdx, stHuInfomation);
		pZiMoPlayer->setBestCards(stHuInfomation.m_nHuHuaCnt);

		if (stHuInfomation.m_bWaiBao && isEnableWaiBao()) {
			st.bWaiBao = true;
		}

		Json::Value jsHuTyps;
		bool bDaHu = true;
		for (auto& refHu : stHuInfomation.m_vFanxing)
		{
			jsHuTyps[jsHuTyps.size()] = refHu;

			if (refHu == eFanxing_GangKai) {
				m_bWillFanBei = true;
			}
			else if (refHu == eFanxing_PingHu) {
				bDaHu = false;
			}
		}
		if (bDaHu) {
			m_bWillFanBei = true;
		}

		jsDetail["vhuTypes"] = jsHuTyps;
		jsDetail["holdHuaCnt"] = stHuInfomation.m_nHoldHuaCnt;
		jsDetail["huHuaCnt"] = stHuInfomation.m_nHuHuaCnt;
		jsDetail["waiBao"] = st.bWaiBao ? 1 : 0;

		stJson["vhuTypes"] = jsHuTyps;
		stJson["holdHuaCnt"] = stHuInfomation.m_nHoldHuaCnt;
		stJson["huHuaCnt"] = stHuInfomation.m_nHuHuaCnt;
		stJson["waiBao"] = st.bWaiBao ? 1 : 0;

		//花砸
		if (isEnableHuaZa()) {
			stHuInfomation.m_nHoldHuaCnt *= 2;
		}

		auto nFanCnt = stHuInfomation.m_nHoldHuaCnt + stHuInfomation.m_nHuHuaCnt + getHuBaseScore() + (getLBTCnt() * 10);

		if (st.bWaiBao) {
			nFanCnt = 50;
		}
		else if(stHuInfomation.m_bBaoPai) {
			nFanCnt *= getSeatCnt() - 1;
		}

		if (isFanBei()) {
			nFanCnt *= 2;
		}

		uint32_t nTotalWin = 0;
		if (stHuInfomation.m_bWaiBao || stHuInfomation.m_bBaoPai) {
			m_bWillFanBei = true;
			if (stHuInfomation.m_nBaoIdx == -1) {
				stHuInfomation.m_nBaoIdx = m_cCheckHuCard.m_nIdx;
			}

			jsDetail["baoPaiIdx"] = stHuInfomation.m_nBaoIdx;
			stJson["baoPaiIdx"] = stHuInfomation.m_nBaoIdx;

			st.addLose(stHuInfomation.m_nBaoIdx, nFanCnt);
			st.addWin(nInvokeIdx, nFanCnt);
		}
		else {
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
		}
		//jsMsg["detail"] = jsDetail;
		st.jsHuMsg = stJson;
		addSettle(st);
		
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
			m_bWillFanBei = true;
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
			stSettle st;
			st.eSettleReason = eMJAct_Hu;
			st.nBeginIdx = nHuIdx;

			Json::Value stJson;
			stJson = jsMsg;
			stJson["dianPaoIdx"] = pLosePlayer->getIdx();

			auto pHuPlayer = (NJMJPlayer*)getPlayerByIdx(nHuIdx);
			if (pHuPlayer == nullptr)
			{
				LOGFMTE("room id = %u hu player idx = %u , is nullptr", getRoomID(), nHuIdx);
				continue;
			}
			pHuPlayer->addHuCnt();

			Json::Value jsHuPlayer;
			jsHuPlayer["idx"] = nHuIdx;
			stJson["huIdx"] = pLosePlayer->getIdx();
			pHuPlayer->setState(eRoomPeer_AlreadyHu);

			auto pHuPlayerCard = (NJMJPlayerCard*)pHuPlayer->getPlayerCard();

			stSortFanInformation stHuInfomation;
			pHuPlayerCard->onDoHu(nInvokeIdx, nCard, pLosePlayer->haveGangFlag());
			checkFanxing(pHuPlayer, nInvokeIdx, stHuInfomation);
			pHuPlayer->setBestCards(stHuInfomation.m_nHuHuaCnt);

			if (stHuInfomation.m_bWaiBao && isEnableWaiBao()) {
				st.bWaiBao = true;
			}

			Json::Value jsHuTyps;
			bool bDaHu = true;
			for (auto& refHu : stHuInfomation.m_vFanxing)
			{
				jsHuTyps[jsHuTyps.size()] = refHu;

				if (refHu == eFanxing_PingHu) {
					bDaHu = false;
				}
			}
			if (bDaHu) {
				m_bWillFanBei = true;
			}

			jsHuPlayer["vhuTypes"] = jsHuTyps;
			jsHuPlayer["holdHuaCnt"] = stHuInfomation.m_nHoldHuaCnt;
			jsHuPlayer["huHuaCnt"] = stHuInfomation.m_nHuHuaCnt;
			jsHuPlayer["waiBao"] = st.bWaiBao ? 1 : 0;

			stJson["vhuTypes"] = jsHuTyps;
			stJson["holdHuaCnt"] = stHuInfomation.m_nHoldHuaCnt;
			stJson["huHuaCnt"] = stHuInfomation.m_nHuHuaCnt;
			stJson["waiBao"] = st.bWaiBao ? 1 : 0;

			//花砸
			if (isEnableHuaZa()) {
				stHuInfomation.m_nHoldHuaCnt *= 2;
			}

			auto nFanCnt = stHuInfomation.m_nHoldHuaCnt + stHuInfomation.m_nHuHuaCnt + getHuBaseScore() + (getLBTCnt() * 10);

			if (st.bWaiBao) {
				nFanCnt = 50;
			}
			else if (stHuInfomation.m_bBaoPai) {
				if (std::find(stHuInfomation.m_vFanxing.begin(), stHuInfomation.m_vFanxing.end(), eFanxing_QiangGang) != stHuInfomation.m_vFanxing.end()) {
					nFanCnt *= getSeatCnt() - 1;
				}
			}

			if (isFanBei()) {
				nFanCnt *= 2;
			}

			auto nLoseIdx = nInvokeIdx;
			if (stHuInfomation.m_bWaiBao || stHuInfomation.m_bBaoPai) {
				m_bWillFanBei = true;
				if (stHuInfomation.m_nBaoIdx == -1) {
					stHuInfomation.m_nBaoIdx = m_cCheckHuCard.m_nIdx;
				}
				nLoseIdx = stHuInfomation.m_nBaoIdx;
				jsHuPlayer["baoPaiIdx"] = nLoseIdx;
				stJson["baoPaiIdx"] = nLoseIdx;
			}

			st.addLose(nLoseIdx, nFanCnt);
			st.addWin(nHuIdx, nFanCnt);

			st.jsHuMsg = stJson;
			addSettle(st);

			jsHuPlayers[jsHuPlayers.size()] = jsHuPlayer;

			LOGFMTD("room id = %u player = %u hu", getRoomID(), nHuIdx);
		}
		jsDetail["huPlayers"] = jsHuPlayers;
	}
	jsMsg["detail"] = jsDetail;
	sendRoomMsg(jsMsg, MSG_ROOM_MQMJ_PLAYER_HU);
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

uint32_t NJMJRoom::getHuBaseScore() {
	auto pNJMJOpts = std::dynamic_pointer_cast<NJMJOpts>(getDelegate()->getOpts());
	return pNJMJOpts->getHuBaseScore();
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

bool NJMJRoom::isEnableBaoMi() {
	auto pNJMJOpts = std::dynamic_pointer_cast<NJMJOpts>(getDelegate()->getOpts());
	return pNJMJOpts->isEnableBaoMi();
}

void NJMJRoom::addSettle(stSettle& tSettle) {
	m_vSettle.push_back(tSettle);

	Json::Value jsItem, jsRDetail;
	jsItem["actType"] = tSettle.eSettleReason;
	jsItem["waiBao"] = tSettle.bWaiBao ? 1 : 0;
	/*if (tSettle.eSettleReason == eMJAct_Hu) {
		jsItem["msg"] = tSettle.jsHuMsg;
	}*/
	
	auto nBeginIdx = tSettle.nBeginIdx;
	if (nBeginIdx == -1) {
		return;
	}
	auto nLopIdx = tSettle.vWinIdxs.begin()->first;
	auto nLopOffset = tSettle.vWinIdxs.begin()->second;
	auto mLopMap = tSettle.vLoseIdx;
	if (tSettle.bOneLose) {
		nLopIdx = tSettle.vLoseIdx.begin()->first;
		nLopOffset = tSettle.vLoseIdx.begin()->second;
		mLopMap = tSettle.vWinIdxs;
	}
	auto pLopPlayer = (NJMJPlayer*)getPlayerByIdx(nLopIdx);

	for (uint8_t i = 0; i < getSeatCnt(); i++) {
		uint8_t nCurIdx = (nBeginIdx + i) % getSeatCnt();
		if (mLopMap.count(nCurIdx)) {
			auto nCurCoin = mLopMap.at(nCurIdx);
			auto pCurPlayer = (NJMJPlayer*)getPlayerByIdx(nCurIdx);
			if (tSettle.bOneLose) {
				if (tSettle.bWaiBao || pLopPlayer->canPayOffset(nLopOffset, getGuang())) {
					if (tSettle.bWaiBao) {
						pLopPlayer->addExtraOffset(-1 * (int32_t)nCurCoin);
						tSettle.addRealOffset(pLopPlayer->getIdx(), -1 * (int32_t)nCurCoin);

						pCurPlayer->addExtraOffset(nCurCoin);
						tSettle.addRealOffset(pCurPlayer->getIdx(), nCurCoin);
					}
					else {
						auto nGain = pLopPlayer->addGuangSingleOffset(-1 * (int32_t)nCurCoin, getGuang());
						uint32_t nRealOffset = nCurCoin - nGain;
						tSettle.addRealOffset(pLopPlayer->getIdx(), -1 * (int32_t)nRealOffset);

						pCurPlayer->addSingleOffset(nRealOffset);
						tSettle.addRealOffset(pCurPlayer->getIdx(), nRealOffset);
					}
				}
			}
			else {
				if (tSettle.bWaiBao) {
					pLopPlayer->addExtraOffset(nCurCoin);
					tSettle.addRealOffset(pLopPlayer->getIdx(), nCurCoin);

					pCurPlayer->addExtraOffset(-1 * (int32_t)nCurCoin);
					tSettle.addRealOffset(pCurPlayer->getIdx(), -1 * (int32_t)nCurCoin);
				}
				else {
					auto nGain = pCurPlayer->addGuangSingleOffset(-1 * (int32_t)nCurCoin, getGuang());
					uint32_t nRealOffset = nCurCoin - nGain;
					tSettle.addRealOffset(pCurPlayer->getIdx(), -1 * (int32_t)nRealOffset);

					pLopPlayer->addSingleOffset(nRealOffset);
					tSettle.addRealOffset(pLopPlayer->getIdx(), nRealOffset);
				}
			}
		}
	}

	Json::Value jsPlayer;
	jsPlayer["idx"] = nLopIdx;
	jsPlayer["chips"] = pLopPlayer->getChips();
	jsPlayer["offset"] = tSettle.getRealOffset(nLopIdx);
	jsRDetail[jsRDetail.size()] = jsPlayer;

	for (auto ref_lop : mLopMap) {
		jsPlayer["idx"] = ref_lop.first;
		jsPlayer["chips"] = getPlayerByIdx(ref_lop.first)->getChips();
		jsPlayer["offset"] = tSettle.getRealOffset(ref_lop.first);
		jsRDetail[jsRDetail.size()] = jsPlayer;
	}

	jsItem["detial"] = jsRDetail;

	sendRoomMsg(jsItem, MSG_ROOM_FXMJ_REAL_TIME_CELL);

	// add frame 
	addReplayFrame(eMJFrame_Settle, jsItem);

	if (tSettle.bOneLose == false && getGuang() && isEnableBaoMi()) {
		uint8_t nGuangCnt = 0;
		for (auto ref : m_vPlayers) {
			if (((NJMJPlayer*)ref)->canBackGain(getGuang())) {
				continue;
			}
			nGuangCnt++;
		}
		if (nGuangCnt > 1 && pLopPlayer->canBackGain(getGuang())) {
			m_vBaoMiIdx.push_back(nLopIdx);
		}
	}
}

void NJMJRoom::settleInfoToJson(Json::Value& jsRealTime) {
	for (auto& ref : m_vSettle) {
		Json::Value jsItem, jsRDetail;
		jsItem["actType"] = ref.eSettleReason;
		jsItem["waiBao"] = ref.bWaiBao ? 1 : 0;
		/*if (ref.eSettleReason == eMJAct_Hu) {
			jsItem["msg"] = ref.jsHuMsg;
		}*/

		for (auto& refl : ref.vLoseIdx)
		{
			auto pPlayer = (NJMJPlayer*)getPlayerByIdx(refl.first);
			if (pPlayer) {
				Json::Value jsPlayer;
				jsPlayer["idx"] = refl.first;
				jsPlayer["offset"] = -1 * refl.second;
				jsPlayer["realOffset"] = ref.getRealOffset(refl.first);
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
				jsPlayer["realOffset"] = ref.getRealOffset(refl.first);
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

bool NJMJRoom::isAnyPlayerPengCard(uint8_t nCard) {
	for (auto ref : m_vPlayers) {
		auto pCard = (NJMJPlayerCard*)((NJMJPlayer*)ref)->getPlayerCard();
		if (pCard->isPengCard(nCard)) {
			return true;
		}
	}
	
	return false;
}

uint8_t NJMJRoom::getQiangGangIdx() {
	if (m_cCheckHuCard.m_nActType == eMJAct_BuGang) {
		return m_cCheckHuCard.m_nIdx;
	}

	return -1;
}

void NJMJRoom::checkFanxing(IMJPlayer* pPlayer, uint8_t nInvokerIdx, stSortFanInformation& stInformation) {
	auto pCard = (NJMJPlayerCard*)pPlayer->getPlayerCard();
	bool bZiMo = nInvokerIdx == pPlayer->getIdx();
	bool bKuaiZhaoHu = false;

	if (bZiMo) {
		bKuaiZhaoHu = pCard->checkKuaiZhaoZiMo(stInformation.m_nBaoIdx, stInformation.m_vFanxing);
		if (pPlayer->haveFlag(IMJPlayer::eMJActFlag_Gang)) {
			if (pPlayer->haveFlag(IMJPlayer::eMJActFlag_BuHua)) {
				stInformation.m_vFanxing.push_back(eFanxing_XiaoGangKai);
			}
			else {
				stInformation.m_vFanxing.push_back(eFanxing_GangKai);
			}
		}
	}
	else {
		bKuaiZhaoHu = pCard->checkKuaiZhaoHu(stInformation.m_nBaoIdx, stInformation.m_vFanxing);
		if (((NJMJPlayer*)pPlayer)->getSongGangIdx() != -1) {
			stInformation.m_vFanxing.push_back(eFanxing_QiangGang);
		}
	}

	if (bKuaiZhaoHu) {
		stInformation.m_bWaiBao = true;
	}
	else {
		//TODO
		if (pCard->getWaiBaoIdx(stInformation.m_nBaoIdx, bZiMo)) {
			stInformation.m_bWaiBao = true;
		}
		else if (pCard->getNormalBaoIdx(stInformation.m_nBaoIdx, bZiMo)) {
			stInformation.m_bBaoPai = true;
		}

		pCard->getNormalHuType(stInformation.m_vFanxing, bZiMo);
	}

	stInformation.m_nHoldHuaCnt = pCard->getHuaCntWithoutHuTypeHuaCnt(stInformation.m_vFanxing, bKuaiZhaoHu);

	sortHuHuaCnt(stInformation.m_nHuHuaCnt, stInformation.m_vFanxing);
}

void NJMJRoom::sortHuHuaCnt(uint32_t& nFanCnt, std::vector<eFanxingType>& vFanxing) {
	for (auto ref : vFanxing) {
		switch (ref)
		{
		case eFanxing_XiaoGangKai:
		{
			nFanCnt += 10;
		}
		break;
		case eFanxing_DuiDuiHu:
		case eFanxing_YaJue:
		case eFanxing_WuHuaGuo:
		case eFanxing_HunYiSe:
		case eFanxing_DiHu:
		case eFanxing_GangKai:
		{
			nFanCnt += 20;
		}
		break;
		case eFanxing_QingYiSe:
		case eFanxing_QuanQiuDuDiao:
		case eFanxing_QiDui:
		{
			nFanCnt += 30;
		}
		break;
		case eFanxing_ShuangQiDui:
		{
			nFanCnt += 80;
		}
		break;
		case eFanxing_DoubleShuangQiDui:
		{
			nFanCnt += 120;
		}
		break;
		case eFanxing_TribleShuangQiDui:
		{
			nFanCnt += 180;
		}
		break;
		case eFanxing_TianHu:
		{
			nFanCnt += 300;
		}
		break;
		}
	}
}