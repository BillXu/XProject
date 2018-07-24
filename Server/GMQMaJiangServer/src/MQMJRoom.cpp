#include "MQMJRoom.h"
#include "MQMJPlayer.h"
#include "CommonDefine.h"
#include "MJRoomStateWaitReady.h"
#include "MJRoomStateWaitPlayerChu.h"
#include "MQMJRoomStateWaitPlayerAct.h"
#include "MJRoomStateStartGame.h"
#include "MJRoomStateGameEnd.h"
#include "MQMJRoomStateDoPlayerAct.h"
#include "MQMJRoomStateAfterChiOrPeng.h"
#include "MQMJRoomStateAskForRobotGang.h"
#include "MQMJRoomStateAskForPengOrHu.h"
#include "FanxingChecker.h"
#include "Fanxing7Dui.h"
#include "FanxingDuiDuiHu.h"
#include "MJReplayFrameType.h"
#include "MQMJPoker.h"

bool MQMJRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	IMJRoom::init(pRoomMgr,nSeialNum,nRoomID,nSeatCnt,vJsOpts);
	m_cFanxingChecker.init();
	m_vGainChip.resize(getSeatCnt());
	m_nGangCnt = 0;
	m_nDice = 0;
	m_nR7 = 0;
	m_nR15 = 0;
	// add room state ;
	IGameRoomState* p[] = { new CMJRoomStateWaitReady(), new MJRoomStateWaitPlayerChu(),new MQMJRoomStateWaitPlayerAct(),
		new MJRoomStateStartGame(),new MJRoomStateGameEnd(),new MQMJRoomStateDoPlayerAct(),new MQMJRoomStateAskForRobotGang(),
		new MQMJRoomStateAskForPengOrHu(), new MQMJRoomStateAfterChiOrPeng()
	};
	for ( auto& pS : p )
	{
		addRoomState(pS);
	}
	setInitState(p[0]);
	return true;
}

IGamePlayer* MQMJRoom::createGamePlayer()
{
	return new MQMJPlayer();
}

void MQMJRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	IMJRoom::packRoomInfo(jsRoomInfo);
	jsRoomInfo["bankIdx"] = m_nBankerIdx;
	jsRoomInfo["curActIdex"] = getCurState()->getCurIdx();
	jsRoomInfo["waitTimer"] = getCurState()->getStateDuring();
	jsRoomInfo["dice"] = m_nDice;
	jsRoomInfo["r7"] = m_nR7;
	jsRoomInfo["r15"] = m_nR15;
}

void MQMJRoom::visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)
{
	if (!pPlayer)
	{
		return;
	}

	IMJRoom::visitPlayerInfo(pPlayer, jsPlayerInfo, nVisitorSessionID);

	if (pPlayer->haveState(eRoomPeer_CanAct) == false)  // not join this round game ;
	{
		return;
	}

	((MQMJPlayer*)pPlayer)->getPlayerCard()->onVisitPlayerCardInfo(jsPlayerInfo, pPlayer->getSessionID() == nVisitorSessionID);
}

void MQMJRoom::onWillStartGame() {
	IMJRoom::onWillStartGame();
	m_vSettle.clear();
	clearGain();
	m_nGangCnt = 0;
	m_nDice = 0;
	m_nR7 = 0;
	m_nR15 = 0;

	doProduceNewBanker();
}

void MQMJRoom::onStartGame()
{
	IMJRoom::onStartGame();
	sendStartGameMsg();
}

void MQMJRoom::sendStartGameMsg() {
	Json::Value jsMsg;
	m_nDice = 1 + rand() % 6;
	m_nDice = m_nDice * 10 + (1 + rand() % 6);
	m_nR7 = m_tPoker.getCardByIdx(7, true);
	m_nR15 = m_tPoker.getCardByIdx(15, true);
	jsMsg["dice"] = m_nDice;
	jsMsg["bankerIdx"] = getBankerIdx();
	jsMsg["r7"] = m_nR7;
	jsMsg["r15"] = m_nR15;
	Json::Value arrPeerCards;
	for (auto& pPlayer : m_vPlayers)
	{
		if (!pPlayer)
		{
			LOGFMTE("why player is null hz mj must all player is not null");
			continue;
		}
		Json::Value peer;
		auto pPlayerCard = ((MQMJPlayer*)pPlayer)->getPlayerCard();
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

uint8_t MQMJRoom::getRoomType()
{
	return eGame_MQMJ;
}

IPoker* MQMJRoom::getPoker()
{
	return &m_tPoker;
}

bool MQMJRoom::isCanGoOnMoPai() {
	return getPoker()->getLeftCardCount() > 16;
}

bool MQMJRoom::canGang() {
	return getPoker()->getLeftCardCount() > 19;
}

void MQMJRoom::onPrePlayerGang() {
	m_nGangCnt++;
	if (m_nGangCnt == 7) {
		m_tPoker.pushCardToFron(m_tPoker.getCardByIdx(7, true));
	}
}

bool MQMJRoom::isGameOver() {
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

bool MQMJRoom::isRoomOver() {
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

void MQMJRoom::onGameEnd() {
	Json::Value jsReal, jsPlayers;
	settleInfoToJson(jsReal);

	for (auto& ref : m_vPlayers) {
		if (ref) {
			Json::Value jsHoldCard, jsPlayer;
			IMJPlayerCard::VEC_CARD vHoldCard;
			((MQMJPlayer*)ref)->getPlayerCard()->getHoldCard(vHoldCard);
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

void MQMJRoom::doProduceNewBanker() {
	if ((uint8_t)-1 == getBankerIdx()) {
		setBankIdx(0);
	}
	else {
		if ((uint8_t)-1 == m_nNextBankerIdx) {
			//��ׯׯ����
		}
		else {
			setBankIdx(m_nNextBankerIdx);
		}
	}

	//���ջ���
	if ((uint8_t)-1 == getBankerIdx()) {
		setBankIdx(0);
	}

	m_nNextBankerIdx = getBankerIdx();
}

void MQMJRoom::setNextBankerIdx(uint8_t nHuIdx) {
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

void MQMJRoom::onPlayerMo(uint8_t nIdx) {
	IMJRoom::onPlayerMo(nIdx);
	auto pPlayer = (MQMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->clearGangFlag();
	pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_NeedClearCanCyclone);
}

void MQMJRoom::onPlayerChu(uint8_t nIdx, uint8_t nCard) {
	auto pPlayer = (MQMJPlayer*)getPlayerByIdx(nIdx);
	bool haveGangFlag = false;
	if (pPlayer->haveFlag(IMJPlayer::eMJActFlag::eMJActFlag_Gang)) {
		haveGangFlag = true;
	}
	IMJRoom::onPlayerChu(nIdx, nCard);
	if (haveGangFlag) {
		pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_Gang);
	}
	if (pPlayer->haveFlag(IMJPlayer::eMJActFlag::eMJActFlag_NeedClearCanCyclone)) {
		pPlayer->clearFlag(IMJPlayer::eMJActFlag::eMJActFlag_CanCyclone);
	}
}

void MQMJRoom::onPlayerPeng(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	IMJRoom::onPlayerPeng(nIdx, nCard, nInvokeIdx);

	auto pPlayer = (MQMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->clearGangFlag();
}

void MQMJRoom::onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	onPrePlayerGang();
	IMJRoom::onPlayerMingGang(nIdx, nCard, nInvokeIdx);

	stSettle st;
	st.eSettleReason = eMJAct_MingGang;
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

	auto pPlayer = (MQMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_NeedClearCanCyclone);
}

void MQMJRoom::onPlayerAnGang(uint8_t nIdx, uint8_t nCard) {
	onPrePlayerGang();
	IMJRoom::onPlayerAnGang(nIdx, nCard);

	stSettle st;
	st.eSettleReason = eMJAct_AnGang;
	uint16_t nWinCoin = 0;
	uint16_t nLoseCoin = 4 * getBaseScore();
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

	auto pPlayer = (MQMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_NeedClearCanCyclone);
}

void MQMJRoom::onPlayerCyclone(uint8_t nIdx, uint8_t nCard) {
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("why this player is null idx = %u , can not an gang", nIdx);
		return;
	}
	onPrePlayerGang();

	pPlayer->signFlag(IMJPlayer::eMJActFlag_Cyclone);
	pPlayer->clearFlag(IMJPlayer::eMJActFlag_LouHu);
	pPlayer->addHuaGangCnt();
	auto nGangGetCard = getPoker()->distributeOneCard();
	if (((MQMJPlayerCard*)pPlayer->getPlayerCard())->onCyclone(nCard, nGangGetCard) == false)
	{
		LOGFMTE("nidx = %u an gang card = %u error,", nIdx, nCard);
	}

	// send msg ;
	Json::Value msg;
	msg["idx"] = nIdx;
	msg["actType"] = eMJAct_Cyclone;
	msg["card"] = nCard;
	msg["gangCard"] = nGangGetCard;
	sendRoomMsg(msg, MSG_ROOM_ACT);

	Json::Value jsFrameArg;
	jsFrameArg["idx"] = nIdx;
	jsFrameArg["gang"] = nCard;
	jsFrameArg["newCard"] = nGangGetCard;
	addReplayFrame(eMJFrame_Cyclone, jsFrameArg);

	stSettle st;
	st.eSettleReason = eMJAct_Cyclone;
	uint16_t nWinCoin = 0;
	uint16_t nLoseCoin = 8 * getBaseScore();
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

	pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_NeedClearCanCyclone);
}

void MQMJRoom::onPlayerBuGang(uint8_t nIdx, uint8_t nCard) {
	auto pPlayer = (MQMJPlayer*)getPlayerByIdx(nIdx);
	if (pPlayer) {
		stSettle st;
		st.eSettleReason = eMJAct_BuGang;
		uint16_t nWinCoin = 0;
		uint16_t nLoseCoin = 2 * getBaseScore();
		for (auto& pp : m_vPlayers) {
			if (pp) {
				if (pp->getIdx() == nIdx) {
					continue;
				}
				else {
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
				}
			}
		}
		st.addWin(nIdx, nWinCoin);
		addSettle(st);
	}
	onPrePlayerGang();
	IMJRoom::onPlayerBuGang(nIdx, nCard);
	pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_NeedClearCanCyclone);
}

void MQMJRoom::onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx) {
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
		auto pZiMoPlayer = (MQMJPlayer*)getPlayerByIdx(nInvokeIdx);
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
		uint16_t nFanCnt = 0;
		auto pZiMoPlayerCard = (MQMJPlayerCard*)pZiMoPlayer->getPlayerCard();
		pZiMoPlayerCard->setHuCard(nCard);
		m_cFanxingChecker.checkFanxing(vType, pZiMoPlayer, nInvokeIdx, this);
		sortFanxing2FanCnt(vType, nFanCnt);
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
		for (int32_t i = 0; i < nFanCnt; ++i) nLoseCoin *= 2;//paixing fan
		nLoseCoin *= 2;//zimo fan

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
				if (m_cFanxingChecker.checkFanxing(eFanxing_MengQing, (MQMJPlayer*)pLosePlayer, nInvokeIdx, this)) {
					nTLoseCoin *= 2;
				}
				st.addLose(pLosePlayer->getIdx(), nTLoseCoin);
				nTotalWin += nTLoseCoin;
			}
		}
		st.addWin(nInvokeIdx, nTotalWin);
		LOGFMTD("room id = %u player = %u zimo", getRoomID(), nInvokeIdx);
	}
	else {
		auto pLosePlayer = (MQMJPlayer*)getPlayerByIdx(nInvokeIdx);
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
			auto pHuPlayer = (MQMJPlayer*)getPlayerByIdx(nHuIdx);
			if (pHuPlayer == nullptr)
			{
				LOGFMTE("room id = %u hu player idx = %u , is nullptr", getRoomID(), nHuIdx);
				continue;
			}
			pHuPlayer->addHuCnt();

			Json::Value jsHuPlayer;
			jsHuPlayer["idx"] = nHuIdx;
			pHuPlayer->setState(eRoomPeer_AlreadyHu);

			auto pHuPlayerCard = (MQMJPlayerCard*)pHuPlayer->getPlayerCard();
			pHuPlayerCard->setHuCard(nCard);

			std::vector<eFanxingType> vType;
			uint16_t nFanCnt = 0;
			pHuPlayerCard->onDoHu(nInvokeIdx, nCard, pLosePlayer->haveGangFlag());
			m_cFanxingChecker.checkFanxing(vType, pHuPlayer, nInvokeIdx, this);
			sortFanxing2FanCnt(vType, nFanCnt);

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

			for (auto& ref : m_vPlayers) {
				if (ref && ref != pHuPlayer) {
					auto nPCoin = nWinCoin;
					if (ref->getIdx() == getBankerIdx() || nHuIdx == getBankerIdx()) {
						nPCoin *= 2;
					}

					if (ref == pLosePlayer) {
						nPCoin *= 2;
					}

					if (m_cFanxingChecker.checkFanxing(eFanxing_MengQing, (MQMJPlayer*)ref, nInvokeIdx, this)) {
						nPCoin *= 2;
					}

					if (isDPOnePay()) {
						st.addLose(nInvokeIdx, nPCoin);
					}
					else {
						st.addLose(ref->getIdx(), nPCoin);
					}

					st.addWin(nHuIdx, nPCoin);
				}
			}

			jsHuPlayers[jsHuPlayers.size()] = jsHuPlayer;
			//st.addWin(nHuIdx, nWinCoin);
			//nTotalLose += nWinCoin;

			LOGFMTD("room id = %u player = %u hu", getRoomID(), nHuIdx);
		}

		/*if (isDPOnePay()) {
			st.addLose(nInvokeIdx, nTotalLose);
		}*/
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

void MQMJRoom::onWaitPlayerAct(uint8_t nIdx, bool& isCanPass) {
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer) {
		LOGFMTE("player idx = %u is null can not tell it wait act", nIdx);
		return;
	}
	
	if (pPlayer->haveState(eRoomPeer_AlreadyHu)) {
		LOGFMTE("player idx = %u is alreay hu, why wait him to act", nIdx);
		return;
	}

	if (!pPlayer)
	{
		LOGFMTE("player idx = %u is null can not tell it wait act", nIdx);
		return;
	}
	auto pMJCard = (MQMJPlayerCard*)pPlayer->getPlayerCard();
	// send msg to tell player do act 
	Json::Value jsArrayActs;
	Json::Value jsFrameActs;
	if (isCanGoOnMoPai() && canGang())
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
		// check cyclone
		if (pPlayer->haveFlag(IMJPlayer::eMJActFlag::eMJActFlag_CanCyclone)) {
			pMJCard->getHoldCardThatCanCyclone(vCards);
			for (auto& ref : vCards)
			{
				Json::Value jsAct;
				jsAct["act"] = eMJAct_Cyclone;
				jsAct["cardNum"] = ref;
				jsArrayActs[jsArrayActs.size()] = jsAct;
				jsFrameActs[jsFrameActs.size()] = eMJAct_Cyclone;
			}
		}
		
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

bool MQMJRoom::onWaitPlayerActAfterCP(uint8_t nIdx) {
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
	if (isCanGoOnMoPai() && canGang()) {
		if (pPlayer->haveFlag(IMJPlayer::eMJActFlag::eMJActFlag_CanCyclone)) {
			auto pMJCard = (MQMJPlayerCard*)pPlayer->getPlayerCard();
			IMJPlayerCard::VEC_CARD vCards;
			if (pMJCard->getHoldCardThatCanCyclone(vCards)) {
				Json::Value jsArrayActs;
				Json::Value jsFrameActs;
				for (auto& ref : vCards)
				{
					Json::Value jsAct;
					jsAct["act"] = eMJAct_Cyclone;
					jsAct["cardNum"] = ref;
					jsArrayActs[jsArrayActs.size()] = jsAct;
					jsFrameActs[jsFrameActs.size()] = eMJAct_Cyclone;
				}

				Json::Value jsMsg;
				jsMsg["acts"] = jsArrayActs;
				sendMsgToPlayer(jsMsg, MSG_ROOM_MQMJ_WAIT_ACT_AFTER_CP, pPlayer->getSessionID());

				Json::Value jsFrameArg;
				jsFrameArg["idx"] = nIdx;
				jsFrameArg["act"] = jsFrameActs;
				addReplayFrame(eMJFrame_WaitPlayerAct, jsFrameActs);
				return true;
			}
		}
	}
	return false;
}

void MQMJRoom::onAskForPengOrHuThisCard(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint16_t>& vOutWaitHuIdx, std::vector<uint16_t>& vOutWaitPengGangIdx, bool& isNeedWaitEat) {
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
		auto pMJCard = ((IMJPlayer*)ref)->getPlayerCard();

		// check peng 
		if (pMJCard->canPengWithCard(nCard))
		{
			jsActs[jsActs.size()] = eMJAct_Peng;
			vOutWaitPengGangIdx.push_back(ref->getIdx());
		}

		// check ming gang 
		if (isCanGoOnMoPai() && canGang() && pMJCard->canMingGangWithCard(nCard))
		{
			jsActs[jsActs.size()] = eMJAct_MingGang;
			// already add in peng ;  vWaitPengGangIdx
			if (vOutWaitPengGangIdx.empty())
			{
				vOutWaitPengGangIdx.push_back(ref->getIdx());
			}
		}

		if (ref->getIdx() == (nInvokeIdx + 1) % getSeatCnt())
		{
			isNeedWaitEat = false;
			if (pMJCard->canEatCard(nCard))
			{
				isNeedWaitEat = true;
				jsActs[jsActs.size()] = eMJAct_Chi;
			}
		}

		// check hu ;
		auto isLouHu = ((IMJPlayer*)ref)->haveFlag(IMJPlayer::eMJActFlag_LouHu);
		if ((isLouHu == false) && pMJCard->canHuWitCard(nCard))
		{
			jsActs[jsActs.size()] = eMJAct_Hu;
			vOutWaitHuIdx.push_back(ref->getIdx());
		}

		if (jsActs.size() > 0)
		{
			jsActs[jsActs.size()] = eMJAct_Pass;
		}

		if (jsActs.size() == 0)
		{
			continue;
		}
		jsMsg["acts"] = jsActs;
		sendMsgToPlayer(jsMsg, MSG_PLAYER_WAIT_ACT_ABOUT_OTHER_CARD, ref->getSessionID());

		//LOGFMTD("inform uid = %u act about other card room id = %u card = %u", ref->getUID(), getRoomID(),nCard );

		Json::Value jsFramePlayer;
		jsFramePlayer["idx"] = ref->getIdx();
		jsFramePlayer["acts"] = jsActs;

		jsFrameArg[jsFrameArg.size()] = jsFramePlayer;
	}

	// add frame 
	addReplayFrame(eMJFrame_WaitPlayerActAboutCard, jsFrameArg);
}

bool MQMJRoom::isAnyPlayerRobotGang(uint8_t nInvokeIdx, uint8_t nCard) {
	for (auto& ref : m_vPlayers)
	{
		if (ref == nullptr || nInvokeIdx == ref->getIdx())
		{
			continue;
		}

		if (ref->haveState(eRoomPeer_AlreadyHu)) {
			continue;
		}

		auto pMJCard = ((IMJPlayer*)ref)->getPlayerCard();
		auto isLouHu = ((IMJPlayer*)ref)->haveFlag(IMJPlayer::eMJActFlag_LouHu);
		if ((isLouHu == false) && pMJCard->canHuWitCard(nCard))
		{
			return true;
		}
	}

	return false;
}

void MQMJRoom::onAskForRobotGang(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates) {
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
		auto pMJCard = ((IMJPlayer*)ref)->getPlayerCard();
		// check hu 
		auto isLouHu = ((IMJPlayer*)ref)->haveFlag(IMJPlayer::eMJActFlag_LouHu);
		if ((isLouHu == false) && pMJCard->canHuWitCard(nCard))
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

uint8_t MQMJRoom::getNextActPlayerIdx(uint8_t nCurActIdx) {
	return (nCurActIdx + 1) % getSeatCnt();
}

uint8_t MQMJRoom::getFanLimit() {
	uint8_t nFanLimit = 0;
	if (m_jsOpts["fanLimit"].isNull() == false && m_jsOpts["fanLimit"].isUInt()) {
		nFanLimit = m_jsOpts["fanLimit"].asUInt();
	}
	return nFanLimit;
}

bool MQMJRoom::isDPOnePay() {
	return m_jsOpts["dpOnePay"].isUInt() ? m_jsOpts["dpOnePay"].asBool() : false;
}

uint8_t MQMJRoom::getBaseScore() {
	return m_jsOpts["baseScore"].isUInt() ? m_jsOpts["baseScore"].asUInt() : 1;
}

uint32_t MQMJRoom::getGuang() {
	return m_jsOpts["guang"].isUInt() ? m_jsOpts["guang"].asUInt() : 0;
}

void MQMJRoom::addSettle(stSettle& tSettle) {
	m_vSettle.push_back(tSettle);
}

void MQMJRoom::settleInfoToJson(Json::Value& jsRealTime) {
	for (auto& ref : m_vSettle) {
		Json::Value jsItem, jsRDetail;
		jsItem["actType"] = ref.eSettleReason;
		if (ref.eSettleReason == eMJAct_Hu) {
			jsItem["msg"] = ref.jsHuMsg;
		}

		uint32_t nTotalGain = 0;
		for (auto& refl : ref.vLoseIdx)
		{
			auto pPlayer = (MQMJPlayer*)getPlayerByIdx(refl.first);
			if (pPlayer) {
				auto nGain = pPlayer->addGuangSingleOffset(-1 * (int32_t)refl.second, getGuang());
				nTotalGain += nGain;
				if (nGain && ref.vWinIdxs.size()) {
					stSettleGain ssg;
					ssg.nGainChips = nGain;
					ssg.nTargetIdx = ref.vWinIdxs[0];
					addGain(refl.first, ssg);
				}
				Json::Value jsPlayer;
				jsPlayer["idx"] = refl.first;
				jsPlayer["offset"] = -1 * refl.second;
				jsRDetail[jsRDetail.size()] = jsPlayer;
			}
		}

		for (auto& refl : ref.vWinIdxs)
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
				jsPlayer["offset"] = refl.second;
				jsRDetail[jsRDetail.size()] = jsPlayer;
			}
		}
		jsItem["detial"] = jsRDetail;
		jsRealTime[jsRealTime.size()] = jsItem;
	}
}

void MQMJRoom::sortFanxing2FanCnt(std::vector<eFanxingType>& vType, uint16_t& nFanCnt) {
	for (auto& ref : vType) {
		switch (ref)
		{
		case eFanxing_GangKai:
		case eFanxing_JiaHu:
		case eFanxing_QiangGang:
		case eFanxing_GangHouPao:
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
			nFanCnt += 3;
		{
			break;
		}
		}
	}
}

void MQMJRoom::addGain(uint8_t nIdx, stSettleGain stGain) {
	if (nIdx < m_vGainChip.size()) {
		m_vGainChip[nIdx].push_back(stGain);
	}
}

void MQMJRoom::clearGain() {
	for (auto& ref : m_vGainChip) {
		ref.clear();
	}
}

void MQMJRoom::backGain(uint8_t nIdx) {
	if (nIdx < m_vGainChip.size()) {
		auto& vGain = m_vGainChip[nIdx];
		if (vGain.empty()) {
			return;
		}

		auto pLosePlayer = (MQMJPlayer*)getPlayerByIdx(nIdx);
		if (pLosePlayer == nullptr) {
			return;
		}
		for (auto& ref : vGain) {
			if (pLosePlayer->canBackGain(getGuang()) == false) {
				return;
			}
			auto pTargetPlayer = (MQMJPlayer*)getPlayerByIdx(ref.nTargetIdx);
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

bool MQMJRoom::isPlayerRootDirectGang(uint8_t nInvokerIdx, uint8_t nCard) {
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nInvokerIdx);
	if (pPlayer) {
		auto pMJCard = pPlayer->getPlayerCard();
		return pMJCard->canHuWitCard(nCard);
	}
	return false;
}

void MQMJRoom::onAskForRobotDirectGang(uint8_t nInvokeIdx, uint8_t nActIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates) {
	// send decalre gang msg ;
	Json::Value msg;
	msg["idx"] = nInvokeIdx;
	msg["actType"] = eMJAct_MingGang_Pre;
	msg["card"] = nCard;
	sendRoomMsg(msg, MSG_ROOM_ACT);

	// inform target player do this things 
	auto pPlayer = (MQMJPlayer*)getPlayerByIdx(nActIdx);
	Json::Value jsFrameArg, jsMsg, jsActs;

	if (pPlayer) {
		auto pMJCard = pPlayer->getPlayerCard();

		if (pMJCard->canHuWitCard(nCard))
		{
			jsActs[jsActs.size()] = eMJAct_Hu;
			vOutCandinates.push_back(nActIdx);
			jsFrameArg[jsFrameArg.size()] = nActIdx;
		}

		if (jsActs.size() > 0)
		{
			jsActs[jsActs.size()] = eMJAct_Pass;
		}

		jsMsg["acts"] = jsActs;
		sendMsgToPlayer(jsMsg, MSG_PLAYER_WAIT_ACT_ABOUT_OTHER_CARD, nActIdx);
	}

	// add frame 
	addReplayFrame(eMJFrame_WaitRobotGang, jsFrameArg);
}

bool MQMJRoom::canStartGame() {
	for (auto ref : m_vPlayers) {
		if (ref && ref->haveState(eRoomPeer_Ready)) {
			continue;
		}
		return false;
	}

	return IMJRoom::canStartGame();
}

bool MQMJRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)
{
	GameRoom::onPlayerEnter(pEnterRoomPlayer);
	// check if already in room ;
	uint8_t nEmptyIdx = 0/*rand() % getSeatCnt()*/;
	for (uint8_t nIdx = 0; nIdx < getSeatCnt(); ++nIdx)
	{
		nEmptyIdx = (nEmptyIdx + nIdx) % getSeatCnt();
		if (getPlayerByIdx(nEmptyIdx) == nullptr)
		{
			//nEmptyIdx = nIdx;
			break;
		}
	}

	if (nEmptyIdx == (uint8_t)-1)
	{
		//LOGFMTE("why player enter , but do not have empty seat");
		return true;
	}

	doPlayerSitDown(pEnterRoomPlayer, nEmptyIdx);
	return true;
}