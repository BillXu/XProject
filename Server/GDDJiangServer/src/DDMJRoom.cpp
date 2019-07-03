#include "DDMJRoom.h"
#include "DDMJPlayer.h"
#include "CommonDefine.h"
#include "DDMJRoomStateWaitReady.h"
#include "DDMJRoomStateWaitPlayerChu.h"
#include "DDMJRoomStateWaitPlayerAct.h"
#include "MJRoomStateStartGame.h"
#include "MJRoomStateGameEnd.h"
#include "DDMJRoomStateDoPlayerAct.h"
#include "DDMJRoomStateAfterChiOrPeng.h"
#include "DDMJRoomStateAskForRobotGang.h"
#include "DDMJRoomStateAskForPengOrHu.h"
#include "FanxingChecker.h"
#include "Fanxing7Dui.h"
#include "FanxingDuiDuiHu.h"
#include "MJReplayFrameType.h"
#include "DDMJPoker.h"
#include "IGameRoomDelegate.h"
#include "DDMJOpts.h"

bool DDMJRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)
{
	IMJRoom::init(pRoomMgr,nSeialNum,nRoomID, ptrGameOpts);
	m_cFanxingChecker.init();
	m_vGainChip.resize(getSeatCnt());
	m_nGangCnt = 0;
	m_nDice = 0;
	m_nR8 = 0;
	m_nR16 = 0;
	// add room state ;
	IGameRoomState* p[] = { new DDMJRoomStateWaitReady(), new DDMJRoomStateWaitPlayerChu(),new DDMJRoomStateWaitPlayerAct(),
		new MJRoomStateStartGame(),new MJRoomStateGameEnd(),new DDMJRoomStateDoPlayerAct(),new DDMJRoomStateAskForRobotGang(),
		new DDMJRoomStateAskForPengOrHu(), new DDMJRoomStateAfterChiOrPeng()
	};
	for ( auto& pS : p )
	{
		addRoomState(pS);
	}
	setInitState(p[0]);
	return true;
}

IGamePlayer* DDMJRoom::createGamePlayer()
{
	return new DDMJPlayer();
}

void DDMJRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	IMJRoom::packRoomInfo(jsRoomInfo);
	jsRoomInfo["bankIdx"] = m_nBankerIdx;
	jsRoomInfo["curActIdex"] = getCurState()->getCurIdx();
	jsRoomInfo["waitTimer"] = getCurState()->getStateDuring();
	jsRoomInfo["dice"] = m_nDice;
	jsRoomInfo["r8"] = m_nR8;
	jsRoomInfo["r16"] = m_nR16;
}

void DDMJRoom::visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)
{
	if (!pPlayer)
	{
		return;
	}

	IMJRoom::visitPlayerInfo(pPlayer, jsPlayerInfo, nVisitorSessionID);
	jsPlayerInfo["extraTime"] = ((DDMJPlayer*)pPlayer)->getExtraTime();

	if (pPlayer->haveState(eRoomPeer_CanAct) == false)  // not join this round game ;
	{
		return;
	}

	((DDMJPlayer*)pPlayer)->getPlayerCard()->onVisitPlayerCardInfo(jsPlayerInfo, pPlayer->getSessionID() == nVisitorSessionID);
}

void DDMJRoom::onWillStartGame() {
	IMJRoom::onWillStartGame();
	m_vSettle.clear();
	clearGain();
	m_nGangCnt = 0;
	m_nDice = 0;
	m_nR8 = 0;
	m_nR16 = 0;

	doProduceNewBanker();
}

void DDMJRoom::onStartGame()
{
	IMJRoom::onStartGame();
	sendStartGameMsg();
}

void DDMJRoom::sendStartGameMsg() {
	Json::Value jsMsg;
	m_nDice = 1 + rand() % 6;
	m_nDice = m_nDice * 10 + (1 + rand() % 6);
	m_nR8 = m_tPoker.getCardByIdx(8, true);
	m_nR16 = m_tPoker.getCardByIdx(16, true);
	jsMsg["dice"] = m_nDice;
	jsMsg["bankerIdx"] = getBankerIdx();
	jsMsg["r8"] = m_nR8;
	jsMsg["r16"] = m_nR16;
	//��Χ���߷��Ϳ�ʼ��Ϣ
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
		auto pPlayerCard = ((DDMJPlayer*)pPlayer)->getPlayerCard();
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

uint8_t DDMJRoom::getRoomType()
{
	return eGame_DDMJ;
}

IPoker* DDMJRoom::getPoker()
{
	return &m_tPoker;
}

bool DDMJRoom::isCanGoOnMoPai() {
	auto nLeftCnt = getPoker()->getLeftCardCount();
	return nLeftCnt > 0 && nLeftCnt + m_nGangCnt > 16;
}

bool DDMJRoom::canGang() {
	auto nLeftCnt = getPoker()->getLeftCardCount();
	if (nLeftCnt > 3) {
		return nLeftCnt + m_nGangCnt > 19;
	}
	return false;
}

bool DDMJRoom::needChu() {
	return canGang();
}

void DDMJRoom::onPrePlayerGang() {
	m_nGangCnt++;
	if (m_nGangCnt == 7 || m_nGangCnt == 15) {
		m_tPoker.pushCardToFron(m_tPoker.getCardByIdx(m_nGangCnt, true));
	}
}

bool DDMJRoom::isGameOver() {
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

bool DDMJRoom::isRoomOver() {
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

void DDMJRoom::onGameEnd() {
	Json::Value jsReal, jsPlayers;
	settleInfoToJson(jsReal);

	for (auto& ref : m_vPlayers) {
		if (ref) {
			Json::Value jsHoldCard, jsPlayer;
			IMJPlayerCard::VEC_CARD vHoldCard;
			((DDMJPlayer*)ref)->getPlayerCard()->getHoldCard(vHoldCard);
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

void DDMJRoom::doProduceNewBanker() {
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

void DDMJRoom::setNextBankerIdx(uint8_t nHuIdx) {
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

void DDMJRoom::onPlayerMo(uint8_t nIdx) {
	IMJRoom::onPlayerMo(nIdx);
	auto pPlayer = (DDMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->clearGangFlag();
	pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_NeedClearCanCyclone);
}

void DDMJRoom::onPlayerChu(uint8_t nIdx, uint8_t nCard) {
	auto pPlayer = (DDMJPlayer*)getPlayerByIdx(nIdx);
	bool haveGangFlag = false;
	if (pPlayer->haveFlag(IMJPlayer::eMJActFlag::eMJActFlag_Gang)) {
		haveGangFlag = true;
	}
	/*bool needClearCanCyclone = false;
	if (pPlayer->haveFlag(IMJPlayer::eMJActFlag::eMJActFlag_NeedClearCanCyclone)) {
		needClearCanCyclone = true;
	}*/
	IMJRoom::onPlayerChu(nIdx, nCard);
	if (haveGangFlag) {
		pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_Gang);
	}
	/*if (needClearCanCyclone) {
		pPlayer->clearFlag(IMJPlayer::eMJActFlag::eMJActFlag_CanCyclone);
	}
	else {
		pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_CanCyclone);
	}*/
}

void DDMJRoom::onPlayerEat(uint8_t nIdx, uint8_t nCard, uint8_t nWithA, uint8_t nWithB, uint8_t nInvokeIdx) {
	IMJRoom::onPlayerEat(nIdx, nCard, nWithA, nWithB, nInvokeIdx);

	auto pPlayer = (DDMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->clearGangFlag();
}

void DDMJRoom::onPlayerPeng(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	IMJRoom::onPlayerPeng(nIdx, nCard, nInvokeIdx);

	auto pPlayer = (DDMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->clearGangFlag();
}

void DDMJRoom::onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	onPrePlayerGang();
	IMJRoom::onPlayerMingGang(nIdx, nCard, nInvokeIdx);

	stSettle st;
	st.eSettleReason = eMJAct_MingGang;
	uint16_t nWinCoin = 0;
	uint16_t nLoseCoin = 2 * getBaseScore();
	auto nCardType = card_Type(nCard);
	if (nCardType == eCT_Jian) {
		nLoseCoin *= 2;
	}
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

	auto pPlayer = (DDMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_NeedClearCanCyclone);
}

void DDMJRoom::onPlayerAnGang(uint8_t nIdx, uint8_t nCard) {
	onPrePlayerGang();
	IMJRoom::onPlayerAnGang(nIdx, nCard);

	stSettle st;
	st.eSettleReason = eMJAct_AnGang;
	uint16_t nWinCoin = 0;
	uint16_t nLoseCoin = 4 * getBaseScore();
	auto nCardType = card_Type(nCard);
	if (nCardType == eCT_Jian) {
		nLoseCoin *= 2;
	}
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

	auto pPlayer = (DDMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_NeedClearCanCyclone);
}

void DDMJRoom::onPlayerCyclone(uint8_t nIdx, uint8_t nCard) {
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
	if (((DDMJPlayerCard*)pPlayer->getPlayerCard())->onCyclone(nCard, nGangGetCard) == false)
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

void DDMJRoom::onPlayerBuGang(uint8_t nIdx, uint8_t nCard) {
	auto pPlayer = (DDMJPlayer*)getPlayerByIdx(nIdx);
	if (pPlayer) {
		stSettle st;
		st.eSettleReason = eMJAct_BuGang;
		uint16_t nWinCoin = 0;
		uint16_t nLoseCoin = 2 * getBaseScore();
		auto nCardType = card_Type(nCard);
		if (nCardType == eCT_Jian) {
			nLoseCoin *= 2;
		}
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
	onPrePlayerGang();
	IMJRoom::onPlayerBuGang(nIdx, nCard);
	pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_NeedClearCanCyclone);
}

void DDMJRoom::onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx) {
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
		auto pZiMoPlayer = (DDMJPlayer*)getPlayerByIdx(nInvokeIdx);
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
		uint16_t nFanCnt = 1;
		auto pZiMoPlayerCard = (DDMJPlayerCard*)pZiMoPlayer->getPlayerCard();
		pZiMoPlayerCard->setHuCard(nCard);
		m_cFanxingChecker.checkFanxing(vType, pZiMoPlayer, nInvokeIdx, this);
		sortFanxing2FanCnt(vType, nFanCnt);
		pZiMoPlayer->setBestCards(nFanCnt);
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
				/*if (m_cFanxingChecker.checkFanxing(eFanxing_MengQing, (DDMJPlayer*)pLosePlayer, nInvokeIdx, this)) {
					nTLoseCoin *= 2;
				}*/
				st.addLose(pLosePlayer->getIdx(), nTLoseCoin);
				nTotalWin += nTLoseCoin;
			}
		}
		st.addWin(nInvokeIdx, nTotalWin);
		pZiMoPlayer->setBestChips(nTotalWin);
		LOGFMTD("room id = %u player = %u zimo", getRoomID(), nInvokeIdx);
	}
	else {
		auto pLosePlayer = (DDMJPlayer*)getPlayerByIdx(nInvokeIdx);
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
			auto pHuPlayer = (DDMJPlayer*)getPlayerByIdx(nHuIdx);
			if (pHuPlayer == nullptr)
			{
				LOGFMTE("room id = %u hu player idx = %u , is nullptr", getRoomID(), nHuIdx);
				continue;
			}
			pHuPlayer->addHuCnt();

			Json::Value jsHuPlayer;
			jsHuPlayer["idx"] = nHuIdx;
			pHuPlayer->setState(eRoomPeer_AlreadyHu);

			auto pHuPlayerCard = (DDMJPlayerCard*)pHuPlayer->getPlayerCard();
			pHuPlayerCard->setHuCard(nCard);

			std::vector<eFanxingType> vType;
			uint16_t nFanCnt = 0;
			pHuPlayerCard->onDoHu(nInvokeIdx, nCard, pLosePlayer->haveGangFlag());
			m_cFanxingChecker.checkFanxing(vType, pHuPlayer, nInvokeIdx, this);
			sortFanxing2FanCnt(vType, nFanCnt);
			pHuPlayer->setBestCards(nFanCnt);

			bool isGHP = std::find(vType.begin(), vType.end(), eFanxing_GangHouPao) != vType.end();

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
						nPCoin *= 2;
					}

					if (ref == pLosePlayer) {
						nPCoin *= 2;
						if (isGHP) {
							nPCoin *= 2;
						}
					}

					/*if (m_cFanxingChecker.checkFanxing(eFanxing_MengQing, (DDMJPlayer*)ref, nInvokeIdx, this)) {
						nPCoin *= 2;
					}*/

					if (isDPOnePay()) {
						st.addLose(nInvokeIdx, nPCoin);
					}
					else {
						st.addLose(ref->getIdx(), nPCoin);
					}

					nAllWinCoin += nPCoin;
				}
			}

			st.addWin(nHuIdx, nAllWinCoin);
			pHuPlayer->setBestChips(nAllWinCoin);
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
		auto pHuPlayer = (DDMJPlayer*)getPlayerByIdx(huIdx);
		if (pHuPlayer == nullptr)
		{
			LOGFMTE("room id = %u hu player idx = %u , is nullptr", getRoomID(), huIdx);
			continue;
		}
		auto pHuCard = (DDMJPlayerCard*)pHuPlayer->getPlayerCard();
		pHuCard->endDoHu(nCard);
		pHuCard->addHuCard(nCard);
	}*/

	addSettle(st);
}

void DDMJRoom::onWaitPlayerAct(uint8_t nIdx, bool& isCanPass) {
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer) {
		LOGFMTE("player idx = %u is null can not tell it wait act", nIdx);
		return;
	}
	
	if (pPlayer->haveState(eRoomPeer_AlreadyHu)) {
		LOGFMTE("player idx = %u is alreay hu, why wait him to act", nIdx);
		return;
	}

	auto pMJCard = (DDMJPlayerCard*)pPlayer->getPlayerCard();
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
		// check cyclone
		/*if (pPlayer->haveFlag(IMJPlayer::eMJActFlag::eMJActFlag_CanCyclone)) {
			pMJCard->getHoldCardThatCanCyclone(vCards);
			for (auto& ref : vCards)
			{
				Json::Value jsAct;
				jsAct["act"] = eMJAct_Cyclone;
				jsAct["cardnum"] = ref;
				jsArrayActs[jsArrayActs.size()] = jsAct;
				jsFrameActs[jsFrameActs.size()] = eMJAct_Cyclone;
			}
		}*/
		
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

bool DDMJRoom::onWaitPlayerActAfterCP(uint8_t nIdx) {
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
		auto pMJCard = (DDMJPlayerCard*)pPlayer->getPlayerCard();
		IMJPlayerCard::VEC_CARD vCards;
		/*if (pPlayer->haveFlag(IMJPlayer::eMJActFlag::eMJActFlag_CanCyclone)) {
			if (pMJCard->getHoldCardThatCanCyclone(vCards)) {
				for (auto& ref : vCards)
				{
					Json::Value jsAct;
					jsAct["act"] = eMJAct_Cyclone;
					jsAct["cardNum"] = ref;
					jsArrayActs[jsArrayActs.size()] = jsAct;
					jsFrameActs[jsFrameActs.size()] = eMJAct_Cyclone;
				}
				flag = true;
			}
			vCards.clear();
		}*/

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

void DDMJRoom::onAskForPengOrHuThisCard(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint16_t>& vOutWaitHuIdx, std::vector<uint16_t>& vOutWaitPengGangIdx, bool& isNeedWaitEat) {
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
		if (pMJCard->canPengWithCard(nCard) && canGang())
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
			if (pMJCard->canEatCard(nCard) && canGang())
			{
				isNeedWaitEat = true;
				jsActs[jsActs.size()] = eMJAct_Chi;
				LOGFMTE("room = %u send player = %u can eat card = %u", getRoomID(), ref->getUserUID(), nCard);
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

bool DDMJRoom::isAnyPlayerRobotGang(uint8_t nInvokeIdx, uint8_t nCard) {
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

void DDMJRoom::onAskForRobotGang(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates) {
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

uint8_t DDMJRoom::getNextActPlayerIdx(uint8_t nCurActIdx) {
	return (nCurActIdx + 1) % getSeatCnt();
}

uint8_t DDMJRoom::getFanLimit() {
	auto pDDMJOpts = std::dynamic_pointer_cast<DDMJOpts>(getDelegate()->getOpts());
	return pDDMJOpts->getFanLimit();
}

bool DDMJRoom::isDPOnePay() {
	auto pDDMJOpts = std::dynamic_pointer_cast<DDMJOpts>(getDelegate()->getOpts());
	return pDDMJOpts->isDianPaoOnePay();
}

uint32_t DDMJRoom::getGuang() {
	auto pDDMJOpts = std::dynamic_pointer_cast<DDMJOpts>(getDelegate()->getOpts());
	return pDDMJOpts->getGuang();
}

void DDMJRoom::addSettle(stSettle& tSettle) {
	m_vSettle.push_back(tSettle);

	Json::Value jsItem, jsRDetail;
	jsItem["actType"] = tSettle.eSettleReason;
	if (tSettle.eSettleReason == eMJAct_Hu) {
		jsItem["msg"] = tSettle.jsHuMsg;
	}

	uint32_t nTotalGain = 0;
	for (auto& refl : tSettle.vLoseIdx)
	{
		auto pPlayer = (DDMJPlayer*)getPlayerByIdx(refl.first);
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

void DDMJRoom::settleInfoToJson(Json::Value& jsRealTime) {
	for (auto& ref : m_vSettle) {
		Json::Value jsItem, jsRDetail;
		jsItem["actType"] = ref.eSettleReason;
		if (ref.eSettleReason == eMJAct_Hu) {
			jsItem["msg"] = ref.jsHuMsg;
		}

		//uint32_t nTotalGain = 0;
		for (auto& refl : ref.vLoseIdx)
		{
			auto pPlayer = (DDMJPlayer*)getPlayerByIdx(refl.first);
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

void DDMJRoom::sortFanxing2FanCnt(std::vector<eFanxingType>& vType, uint16_t& nFanCnt) {
	for (auto& ref : vType) {
		switch (ref)
		{
		case eFanxing_GangKai:
		case eFanxing_JiaHu:
		//case eFanxing_QiangGang:
		//case eFanxing_GangHouPao:
		case eFanxing_HaiDiLaoYue:
		case eFanxing_MengQing:
		case eFanxing_BianHu:
		{
			nFanCnt += 1;
			break;
		}
		/*case eFanxing_BianHu:
		{
			nFanCnt += 2;
			break;
		}*/
		case eFanxing_DuiDuiHu:
		{
			nFanCnt += 2;
			break;
		}
		}
	}
}

void DDMJRoom::addGain(uint8_t nIdx, stSettleGain stGain) {
	if (nIdx < m_vGainChip.size()) {
		m_vGainChip[nIdx].push_back(stGain);
	}
}

void DDMJRoom::clearGain() {
	for (auto& ref : m_vGainChip) {
		ref.clear();
	}
}

void DDMJRoom::backGain(uint8_t nIdx) {
	if (nIdx < m_vGainChip.size()) {
		auto& vGain = m_vGainChip[nIdx];
		if (vGain.empty()) {
			return;
		}

		auto pLosePlayer = (DDMJPlayer*)getPlayerByIdx(nIdx);
		if (pLosePlayer == nullptr) {
			return;
		}
		for (auto& ref : vGain) {
			if (pLosePlayer->canBackGain(getGuang()) == false) {
				return;
			}
			auto pTargetPlayer = (DDMJPlayer*)getPlayerByIdx(ref.nTargetIdx);
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

bool DDMJRoom::isPlayerRootDirectGang(uint8_t nInvokerIdx, uint8_t nCard) {
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nInvokerIdx);
	if (pPlayer) {
		auto pMJCard = pPlayer->getPlayerCard();
		return pMJCard->canHuWitCard(nCard);
	}
	return false;
}

void DDMJRoom::onAskForRobotDirectGang(uint8_t nInvokeIdx, uint8_t nActIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates) {
	// send decalre gang msg ;
	Json::Value msg;
	msg["idx"] = nInvokeIdx;
	msg["actType"] = eMJAct_MingGang_Pre;
	msg["card"] = nCard;
	sendRoomMsg(msg, MSG_ROOM_ACT);

	// inform target player do this things 
	auto pPlayer = (DDMJPlayer*)getPlayerByIdx(nActIdx);
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
		sendMsgToPlayer(jsMsg, MSG_PLAYER_WAIT_ACT_ABOUT_OTHER_CARD, pPlayer->getSessionID());
	}

	// add frame 
	addReplayFrame(eMJFrame_WaitRobotGang, jsFrameArg);
}

bool DDMJRoom::canStartGame() {
	for (auto ref : m_vPlayers) {
		if (ref && ref->haveState(eRoomPeer_Ready)) {
			continue;
		}
		return false;
	}

	return IMJRoom::canStartGame();
}

bool DDMJRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)
{
	//GameRoom::onPlayerEnter(pEnterRoomPlayer);
	////check if already in room ;
	//uint8_t nEmptyIdx = rand() % getSeatCnt();
	//for (uint8_t nIdx = 0; nIdx < getSeatCnt(); ++nIdx)
	//{
	//	nEmptyIdx = (nEmptyIdx + nIdx) % getSeatCnt();
	//	if (getPlayerByIdx(nEmptyIdx) == nullptr)
	//	{
	//		//nEmptyIdx = nIdx;
	//		break;
	//	}
	//}

	//if (nEmptyIdx == (uint8_t)-1)
	//{
	//	//LOGFMTE("why player enter , but do not have empty seat");
	//	return true;
	//}

	//doPlayerSitDown(pEnterRoomPlayer, nEmptyIdx);
	//return true;

	// check is already sit down ?
	auto psitDown = getPlayerByUID(pEnterRoomPlayer->nUserUID);
	if (psitDown)
	{
		LOGFMTE("this player is already sitdown uid = %u , room id = %u , why enter room again ?", psitDown->getUserUID(), getRoomID());
		//sendRoomInfo(pEnterRoomPlayer->nSessionID);
		return true;
	}

	auto iterStand = m_vStandPlayers.find(pEnterRoomPlayer->nUserUID);
	if (iterStand == m_vStandPlayers.end())
	{
		if (m_vStandPlayers.size() > 50) {
			LOGFMTD("room id = %u , player uid = %u enter room error, room is full!", getRoomID(), pEnterRoomPlayer->nUserUID);
			return true;
		}

		auto p = new stStandPlayer();
		p->nSessionID = pEnterRoomPlayer->nSessionID;
		p->nUserUID = pEnterRoomPlayer->nUserUID;
		m_vStandPlayers[p->nUserUID] = p;
		LOGFMTD("room id = %u , player uid = %u enter room chip = %u", getRoomID(), p->nUserUID, pEnterRoomPlayer->nChip);
	}
	else
	{
		LOGFMTE("room id = %u uid = %u already in this room why enter again ?", getRoomID(), pEnterRoomPlayer->nUserUID);
		iterStand->second->nSessionID = pEnterRoomPlayer->nSessionID;
	}
	//sendRoomInfo(pEnterRoomPlayer->nSessionID);
	return true;
}

void DDMJRoom::doRandomChangeSeat() {
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

bool DDMJRoom::doChangeSeat(uint16_t nIdx, uint16_t nWithIdx) {
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

uint8_t DDMJRoom::checkPlayerCanSitDown(stEnterRoomData* pEnterRoomPlayer)
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