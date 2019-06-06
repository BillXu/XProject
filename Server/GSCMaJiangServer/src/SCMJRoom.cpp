#include "SCMJRoom.h"
#include "SCMJPlayer.h"
#include "CommonDefine.h"
#include "SCMJRoomStateConfirmMiss.h"
#include "SCMJRoomStateExchange3cards.h"
#include "MJRoomStateWaitReady.h"
#include "MJRoomStateWaitPlayerChu.h"
#include "SCMJRoomStateWaitPlayerAct.h"
#include "SCMJRoomStateStartGame.h"
#include "MJRoomStateGameEnd.h"
#include "MJRoomStateDoPlayerAct.h"
#include "MJRoomStateAskForRobotGang.h"
#include "MJRoomStateAskForPengOrHu.h"
#include "FanxingChecker.h"
#include "Fanxing7Dui.h"
#include "FanxingDuiDuiHu.h"
#include "MJReplayFrameType.h"
#include "SCMJOpts.h"
#include "IGameRoomDelegate.h"

bool SCMJRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)
{
	IMJRoom::init(pRoomMgr,nSeialNum,nRoomID,ptrGameOpts);
	m_cFanxingChecker.init(isEnable19(), isEnableMenQing(), isEnableZZ(), isEnableTDHu(), isEnableTDHu());
	// add room state ;
	IGameRoomState* p[] = { new CMJRoomStateWaitReady(), new MJRoomStateWaitPlayerChu(),new SCMJRoomStateWaitPlayerAct(),
		new SCMJRoomStateStartGame(),new MJRoomStateGameEnd(),new MJRoomStateDoPlayerAct(),new MJRoomStateAskForRobotGang(),
		new MJRoomStateAskForPengOrHu(),new SCMJRoomStateConfirmMiss(),new SCMJRoomStateExchange3Cards()
	};
	for ( auto& pS : p )
	{
		addRoomState(pS);
	}
	setInitState(p[0]);
	return true;
}

IGamePlayer* SCMJRoom::createGamePlayer()
{
	return new SCMJPlayer();
}

void SCMJRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	IMJRoom::packRoomInfo(jsRoomInfo);
	jsRoomInfo["bankIdx"] = m_nBankerIdx;
	jsRoomInfo["curActIdex"] = getCurState()->getCurIdx();
	jsRoomInfo["waitTimer"] = getCurState()->getStateDuring();
}

void SCMJRoom::visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)
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

	((SCMJPlayer*)pPlayer)->getPlayerCard()->onVisitPlayerCardInfo(jsPlayerInfo, pPlayer->getSessionID() == nVisitorSessionID);
	
	if (m_bIsShowMiss) {
		jsPlayerInfo["missType"] = ((SCMJPlayerCard*)((SCMJPlayer*)pPlayer)->getPlayerCard())->getMissType();
	}
}

void SCMJRoom::onWillStartGame() {
	IMJRoom::onWillStartGame();
	m_vSettle.clear();
	m_bIsShowMiss = false;

	doProduceNewBanker();
}

void SCMJRoom::onStartGame()
{
	IMJRoom::onStartGame();
	sendStartGameMsg();
}

void SCMJRoom::sendStartGameMsg() {
	Json::Value jsMsg;
	uint8_t nDice = 2 + rand() % 11;
	jsMsg["dice"] = nDice;
	Json::Value arrPeerCards;
	for (auto& pPlayer : m_vPlayers)
	{
		if (!pPlayer)
		{
			LOGFMTE("why player is null hz mj must all player is not null");
			continue;
		}
		Json::Value peer;
		auto pPlayerCard = ((SCMJPlayer*)pPlayer)->getPlayerCard();
		IMJPlayerCard::VEC_CARD vCard;
		pPlayerCard->getHoldCard(vCard);
		for (auto& vC : vCard)
		{
			peer[peer.size()] = vC;
		}
		jsMsg["cards"] = peer;
		sendMsgToPlayer(jsMsg, MSG_ROOM_SCMJ_GAME_START, pPlayer->getSessionID());
	}
}

uint8_t SCMJRoom::getRoomType()
{
	return eGame_SCMJ;
}

IPoker* SCMJRoom::getPoker()
{
	return &m_tPoker;
}

bool SCMJRoom::isGameOver() {
	if (isCanGoOnMoPai() == false) {
		return true;
	}
	uint8_t nNotHu = 0;
	for (auto& ref : m_vPlayers) {
		if (ref) {
			if (ref->haveState(eRoomPeer_CanAct) == false) {
				continue;
			}
			if (ref->haveState(eRoomPeer_AlreadyHu)) {
				continue;
			}
			nNotHu++;
		}
	}
	return nNotHu < 2;
}

void SCMJRoom::onGameEnd() {
	Json::Value jsReal, jsChaJiao, jsHuaZhu;
	settleInfoToJson(jsReal);
	onPlayerChaJiao(jsChaJiao);
	onPlayerHuaZhu(jsHuaZhu);

	Json::Value jsMsg;
	jsMsg["realTimeCal"] = jsReal;
	jsMsg["chaJiao"] = jsChaJiao;
	jsMsg["huaZhu"] = jsHuaZhu;
	sendRoomMsg(jsMsg, MSG_ROOM_SCMJ_GAME_END);

	IMJRoom::onGameEnd();
}

void SCMJRoom::doProduceNewBanker() {
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
	m_nNextBankerIdx = -1;
}

void SCMJRoom::onPlayerMo(uint8_t nIdx) {
	IMJRoom::onPlayerMo(nIdx);
	auto pPlayer = (SCMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->clearGangFlag();
}

void SCMJRoom::onPlayerChu(uint8_t nIdx, uint8_t nCard) {
	auto pPlayer = (SCMJPlayer*)getPlayerByIdx(nIdx);
	bool haveGangFlag = false;
	if (pPlayer->haveFlag(IMJPlayer::eMJActFlag::eMJActFlag_Gang)) {
		haveGangFlag = true;
	}
	IMJRoom::onPlayerChu(nIdx, nCard);
	if (haveGangFlag) {
		pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_Gang);
	}
}

void SCMJRoom::onPlayerPeng(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	IMJRoom::onPlayerPeng(nIdx, nCard, nInvokeIdx);

	auto pPlayer = (SCMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->clearGangFlag();
}

void SCMJRoom::onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	IMJRoom::onPlayerMingGang(nIdx, nCard, nInvokeIdx);

	stSettle st;
	st.eSettleReason = eMJAct_MingGang;
	st.addWin(nIdx, 2 * getBaseScore());
	st.addLose(nInvokeIdx, 2 * getBaseScore());
	addSettle(st);
}

void SCMJRoom::onPlayerAnGang(uint8_t nIdx, uint8_t nCard) {
	IMJRoom::onPlayerAnGang(nIdx, nCard);

	stSettle st;
	st.eSettleReason = eMJAct_AnGang;
	uint16_t nWinCoin = 0;
	for (auto& pp : m_vPlayers) {
		if (pp) {
			if (pp->getIdx() == nIdx) {
				continue;
			}
			else {
				if (isEnableBloodFlow()) {

				}
				else{
					if (pp->haveState(eRoomPeer_AlreadyHu)) {
						continue;
					}
				}
				st.addLose(pp->getIdx(), 2 * getBaseScore());
				nWinCoin += 2 * getBaseScore();
			}
		}
	}
	st.addWin(nIdx, nWinCoin);
	addSettle(st);
}

void SCMJRoom::onPlayerBuGang(uint8_t nIdx, uint8_t nCard) {
	auto pPlayer = (SCMJPlayer*)getPlayerByIdx(nIdx);
	if (pPlayer && pPlayer->getPlayerCard()->getNewestFetchedCard() == nCard) {
		stSettle st;
		st.eSettleReason = eMJAct_BuGang;
		uint16_t nWinCoin = 0;
		for (auto& pp : m_vPlayers) {
			if (pp) {
				if (pp->getIdx() == nIdx) {
					continue;
				}
				else {
					if (isEnableBloodFlow()) {

					}
					else {
						if (pp->haveState(eRoomPeer_AlreadyHu)) {
							continue;
						}
					}
					st.addLose(pp->getIdx(), getBaseScore());
					nWinCoin += getBaseScore();
				}
			}
		}
		st.addWin(nIdx, nWinCoin);
		addSettle(st);
	}
	IMJRoom::onPlayerBuGang(nIdx, nCard);
}

void SCMJRoom::onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx) {
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

	if (isZiMo) {
		auto pZiMoPlayer = (SCMJPlayer*)getPlayerByIdx(nInvokeIdx);
		if (pZiMoPlayer == nullptr)
		{
			LOGFMTE("room id = %u zi mo player is nullptr idx = %u ", getRoomID(), nInvokeIdx);
			return;
		}
		pZiMoPlayer->addZiMoCnt();
		pZiMoPlayer->setState(eRoomPeer_AlreadyHu);
		// svr :{ huIdx : 234 , baoPaiIdx : 2 , winCoin : 234,huardSoftHua : 23, isGangKai : 0 ,vhuTypes : [ eFanxing , ], LoseIdxs : [ {idx : 1 , loseCoin : 234 }, .... ]   }
		jsDetail["huIdx"] = nInvokeIdx;
		if ((uint8_t)-1 == m_nNextBankerIdx) {
			m_nNextBankerIdx = nInvokeIdx;
		}
		std::vector<eFanxingType> vType;
		uint16_t nFanCnt = 0;
		m_cFanxingChecker.checkFanxing(vType, pZiMoPlayer, nInvokeIdx, this);
		sortFanxing2FanCnt(vType, nFanCnt);
		Json::Value jsHuTyps;
		for (auto& refHu : vType)
		{
			jsHuTyps[jsHuTyps.size()] = refHu;
		}
		jsDetail["vhuTypes"] = jsHuTyps;
		if (nFanCnt > getFanLimit()) {
			nFanCnt = getFanLimit();
		}
		uint32_t nLoseCoin = 1;
		for (int32_t i = 0; i < nFanCnt; ++i) nLoseCoin *= 2;
		if (isZiMoAddFan()) {
			if (nFanCnt < getFanLimit()) {
				nLoseCoin *= 2;
			}
		}
		else {
			nLoseCoin += 1;
		}
		nLoseCoin += std::count(vType.begin(), vType.end(), eFanxing_SC_Gen);

		uint16_t nTotalWin = 0;
		for (auto& pLosePlayer : m_vPlayers)
		{
			if (pLosePlayer) {
				if (pLosePlayer == pZiMoPlayer)
				{
					continue;
				}
				if (isEnableBloodFlow() == false && pLosePlayer->haveState(eRoomPeer_AlreadyHu)) {
					continue;
				}
				st.addLose(pLosePlayer->getIdx(), nLoseCoin);
				nTotalWin += nLoseCoin;
			}
		}
		st.addWin(nInvokeIdx, nTotalWin);
		LOGFMTD("room id = %u player = %u zimo", getRoomID(), nInvokeIdx);
	}
	else {
		auto pLosePlayer = (SCMJPlayer*)getPlayerByIdx(nInvokeIdx);
		if (!pLosePlayer)
		{
			LOGFMTE("room id = %u lose but player idx = %u is nullptr", getRoomID(), nInvokeIdx);
			return;
		}
		jsDetail["dianPaoIdx"] = pLosePlayer->getIdx();
		pLosePlayer->addDianPaoCnt();
		std::vector<uint8_t> vOrderHu;
		if (vHuIdx.size() > 1)
		{
			m_nNextBankerIdx = nInvokeIdx;

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
		else
		{
			if ((uint8_t)-1 == m_nNextBankerIdx) {
				m_nNextBankerIdx = vHuIdx.front();
			}
			vOrderHu.swap(vHuIdx);
		}

		Json::Value jsHuPlayers, jsLosePlayers;
		uint32_t nTotalLose = 0;
		for (auto& nHuIdx : vOrderHu)
		{
			auto pHuPlayer = (SCMJPlayer*)getPlayerByIdx(nHuIdx);
			if (pHuPlayer == nullptr)
			{
				LOGFMTE("room id = %u hu player idx = %u , is nullptr", getRoomID(), nHuIdx);
				continue;
			}
			pHuPlayer->addHuCnt();

			Json::Value jsLosePlayer, jsHuPlayer;
			jsHuPlayer["idx"] = nHuIdx;
			pHuPlayer->setState(eRoomPeer_AlreadyHu);

			auto pHuPlayerCard = (SCMJPlayerCard*)pHuPlayer->getPlayerCard();

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

			if (nFanCnt > getFanLimit()) {
				nFanCnt = getFanLimit();
			}
			uint32_t nWinCoin = 1;
			for (int32_t i = 0; i < nFanCnt; ++i) nWinCoin *= 2;
			if (std::find(vType.begin(), vType.end(), eFanxing_GangHouPao) != vType.end() && isDianGangZiMo()) {
				eraseLastGangSettle();
				if (isZiMoAddFan()) {
					if (nFanCnt < getFanLimit()) {
						nWinCoin *= 2;
					}
				}
				else {
					nWinCoin += 1;
				}
			}
			nWinCoin += std::count(vType.begin(), vType.end(), eFanxing_SC_Gen);
			jsHuPlayers[jsHuPlayers.size()] = jsHuPlayer;
			st.addWin(nHuIdx, nWinCoin);
			nTotalLose += nWinCoin;

			LOGFMTD("room id = %u player = %u hu", getRoomID(), nHuIdx);
		}
		st.addLose(nInvokeIdx, nTotalLose);
		jsDetail["huPlayers"] = jsHuPlayers;
	}
	jsMsg["detail"] = jsDetail;
	st.jsHuMsg = jsMsg;
	sendRoomMsg(jsMsg, MSG_ROOM_SCMJ_PLAYER_HU);

	for (auto huIdx : vHuIdx) {
		auto pHuPlayer = (SCMJPlayer*)getPlayerByIdx(huIdx);
		if (pHuPlayer == nullptr)
		{
			LOGFMTE("room id = %u hu player idx = %u , is nullptr", getRoomID(), huIdx);
			continue;
		}
		auto pHuCard = (SCMJPlayerCard*)pHuPlayer->getPlayerCard();
		pHuCard->endDoHu(nCard);
		pHuCard->addHuCard(nCard);
	}

	addSettle(st);
}

void SCMJRoom::onWaitPlayerAct(uint8_t nIdx, bool& isCanPass) {
	auto pPlayer = getPlayerByIdx(nIdx);
	if (!pPlayer) {
		LOGFMTE("player idx = %u is null can not tell it wait act", nIdx);
		return;
	}
	
	if (pPlayer->haveState(eRoomPeer_AlreadyHu) && isEnableBloodFlow() == false) {
		LOGFMTE("player idx = %u is alreay hu, why wait him to act", nIdx);
		return;
	}

	IMJRoom::onWaitPlayerAct(nIdx, isCanPass);
}

void SCMJRoom::onAskForPengOrHuThisCard(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint16_t>& vOutWaitHuIdx, std::vector<uint16_t>& vOutWaitPengGangIdx, bool& isNeedWaitEat) {
	Json::Value jsFrameArg;

	for (auto& ref : m_vPlayers)
	{
		if (ref == nullptr || nInvokeIdx == ref->getIdx())
		{
			continue;
		}

		if (ref->haveState(eRoomPeer_AlreadyHu) && isEnableBloodFlow() == false) {
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
		if (isCanGoOnMoPai() && pMJCard->canMingGangWithCard(nCard))
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

bool SCMJRoom::isAnyPlayerRobotGang(uint8_t nInvokeIdx, uint8_t nCard) {
	for (auto& ref : m_vPlayers)
	{
		if (ref == nullptr || nInvokeIdx == ref->getIdx())
		{
			continue;
		}

		if (ref->haveState(eRoomPeer_AlreadyHu) && isEnableBloodFlow() == false) {
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

void SCMJRoom::onAskForRobotGang(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates) {
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

		if (ref->haveState(eRoomPeer_AlreadyHu) && isEnableBloodFlow() == false) {
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

uint8_t SCMJRoom::getNextActPlayerIdx(uint8_t nCurActIdx) {
	uint8_t nNext = nCurActIdx;
	for (uint8_t i = 1; i < getSeatCnt(); i++) {
		nNext = (nCurActIdx + i) % getSeatCnt();
		auto pp = getPlayerByIdx(nNext);
		if (pp) {
			if (pp->haveState(eRoomPeer_AlreadyHu) && isEnableBloodFlow() == false) {
				continue;
			}
			break;
		}
	}
	return nNext;
}

/*
	exchage cards
	统一命令号: MSG_ROOM_SCMJ_PLAYER_EXCHANGE_CARDS
	state(状态):
		0, 准备换牌 发给客户端的换牌信号, 等待客户端回复确认换牌信息
		1, 玩家决定要换的牌 即服务器接收客户端消息的方式, 如果出错服务器回复此消息状态
		2, 最终换牌
	所有的换牌数据将通过最终换牌的结果发送给客户端(2), 其他时候如果不出错不会向客户端发送额外数据
*/
bool SCMJRoom::onWaitPlayerExchangeCards() {
	if (isEnableExchange3Cards()) {
		Json::Value jsMsg;
		jsMsg["state"] = 0;
		sendRoomMsg(jsMsg, MSG_ROOM_SCMJ_PLAYER_EXCHANGE_CARDS);
		return true;
	}
	return false;
}

void SCMJRoom::onPlayerExchangeCards(std::map<uint8_t, std::vector<uint8_t>>& exChangeCards) {
	Json::Value jsMsg;
	jsMsg["state"] = 2;
	jsMsg["ret"] = 0;
	if (exChangeCards.size() != getSeatCnt()) {
		jsMsg["ret"] = 1;//换牌数据长度错误, 此错误一般只会发生在服务器
		sendRoomMsg(jsMsg, MSG_ROOM_SCMJ_PLAYER_EXCHANGE_CARDS);
		return;
	}

	uint8_t exchangeType = rand() % 3;//0 顺时针; 1 逆时针; 2 对家交换
	jsMsg["exchangeType"] = exchangeType;
	for (auto& ref : exChangeCards) {
		uint8_t targetIdx = ref.first;
		if (exchangeType) {
			targetIdx = (ref.first + exchangeType) % getSeatCnt();
		}
		else {
			if (ref.first > 0) {
				targetIdx = ref.first - 1;
			}
			else {
				targetIdx = getSeatCnt() - 1;
			}
		}
		if (targetIdx == ref.first) {
			continue;
		}
		if (exChangeCards.count(targetIdx)) {
			auto exchagePlayer = (SCMJPlayer*)getPlayerByIdx(targetIdx);
			if (exchagePlayer && ((SCMJPlayerCard*)exchagePlayer->getPlayerCard())->onExchageCards(exChangeCards.at(targetIdx), ref.second)) {
				Json::Value jsInCards, jsOutCards;
				for (uint8_t i = 0; i < ref.second.size(); i++) {
					jsInCards[i] = ref.second[i];
					jsOutCards[i] = exChangeCards[targetIdx][i];
				}
				jsMsg["inCards"] = jsInCards;
				jsMsg["outCards"] = jsOutCards;
				sendMsgToPlayer(jsMsg, MSG_ROOM_SCMJ_PLAYER_EXCHANGE_CARDS, exchagePlayer->getSessionID());
			}
		}
	}
}

void SCMJRoom::onAutoDecidePlayerExchangeCards(std::map<uint8_t, std::vector<uint8_t>>& exChangeCards) {
	for (auto& ref : m_vPlayers) {
		if (exChangeCards.count(ref->getIdx())) {
			continue;
		}
		std::vector<uint8_t> vCards;
		((SCMJPlayerCard*)((SCMJPlayer*)ref)->getPlayerCard())->onAutoDecideExchageCards(3, vCards);
		exChangeCards[ref->getIdx()] = vCards;
	}
}

void SCMJRoom::onPlayerDecideExchangeCards(uint8_t idx, std::vector<uint8_t> vCards, std::map<uint8_t, std::vector<uint8_t>>& exChangeCards) {
	auto pPlayer = (SCMJPlayer*)getPlayerByIdx(idx);
	if (pPlayer) {
		Json::Value jsMsg;
		jsMsg["state"] = 1;
		if (vCards.size() != 3) {
			jsMsg["ret"] = 1; //换张数量错误
			sendMsgToPlayer(jsMsg, MSG_ROOM_SCMJ_PLAYER_EXCHANGE_CARDS, pPlayer->getSessionID());
			return;
		}
		if (exChangeCards.count(pPlayer->getIdx())) {
			jsMsg["ret"] = 2; //重复决定无效
			sendMsgToPlayer(jsMsg, MSG_ROOM_SCMJ_PLAYER_EXCHANGE_CARDS, pPlayer->getSessionID());
			return;
		}
		auto pCard = (SCMJPlayerCard*)pPlayer->getPlayerCard();
		if (pCard->isHaveCards(vCards) == false) {
			jsMsg["ret"] = 3; //换张不存在
			sendMsgToPlayer(jsMsg, MSG_ROOM_SCMJ_PLAYER_EXCHANGE_CARDS, pPlayer->getSessionID());
			return;
		}
		auto nType = card_Type(vCards[0]);
		if (nType != eCT_Tiao && nType != eCT_Wan && nType != eCT_Tong) {
			jsMsg["ret"] = 4; //换张类型不合法
			sendMsgToPlayer(jsMsg, MSG_ROOM_SCMJ_PLAYER_EXCHANGE_CARDS, pPlayer->getSessionID());
			return;
		}
		for (uint8_t i = 1; i < vCards.size(); i++) {
			if (nType == card_Type(vCards[i])) {
				continue;
			}
			jsMsg["ret"] = 5; //换张类型不匹配
			sendMsgToPlayer(jsMsg, MSG_ROOM_SCMJ_PLAYER_EXCHANGE_CARDS, pPlayer->getSessionID());
			return;
		}
		Json::Value jsCards;
		for (auto& ref : vCards) {
			jsCards[jsCards.size()] = ref;
		}
		jsMsg["cards"] = jsCards;
		sendMsgToPlayer(jsMsg, MSG_ROOM_SCMJ_PLAYER_EXCHANGE_CARDS, pPlayer->getSessionID());
		exChangeCards[pPlayer->getIdx()] = vCards;
	}
}

/*
	miss card type
	统一命令号: MSG_ROOM_SCMJ_PLAYER_DECIDE_MISS
	state(状态):
		0, 准备定缺 发给客户端的定缺信号, 等待客户端回复确认定缺
		1, 玩家决定缺 即服务器接收客户端消息的方式, 如果出错服务器回复此消息状态
		2, 最终定缺
	所有的定缺数据将通过最终定缺的结果发送给客户端(2), 其他时候如果不出错不会向客户端发送额外数据
*/
bool SCMJRoom::onWaitPlayerConfirmMiss() {
	Json::Value jsMsg;
	jsMsg["state"] = 0;
	sendRoomMsg(jsMsg, MSG_ROOM_SCMJ_PLAYER_DECIDE_MISS);
	return true;
}

void SCMJRoom::onPlayerConfirmMiss(uint8_t idx, uint8_t nCardType) {
	auto pPlayer = (SCMJPlayer*)getPlayerByIdx(idx);
	if (pPlayer) {
		Json::Value jsMsg;
		jsMsg["state"] = 1;
		if (nCardType != eCT_Wan && nCardType != eCT_Tong && nCardType != eCT_Tiao) {
			jsMsg["ret"] = 1; //定缺类型错误
			sendMsgToPlayer(jsMsg, MSG_ROOM_SCMJ_PLAYER_DECIDE_MISS, pPlayer->getSessionID());
			return;
		}
		auto pCard = (SCMJPlayerCard*)pPlayer->getPlayerCard();
		if (pCard->isDecideMiss()) {
			jsMsg["ret"] = 2; //重复定缺
			sendMsgToPlayer(jsMsg, MSG_ROOM_SCMJ_PLAYER_DECIDE_MISS, pPlayer->getSessionID());
			return;
		}
		pCard->setMiss((eMJCardType)nCardType);
		jsMsg["missType"] = nCardType;
		sendMsgToPlayer(jsMsg, MSG_ROOM_SCMJ_PLAYER_DECIDE_MISS, pPlayer->getSessionID());
	}
}

void SCMJRoom::onAutoConfirmMiss() {
	for (auto& ref : m_vPlayers) {
		if (ref) {
			auto pCard = (SCMJPlayerCard*)((SCMJPlayer*)ref)->getPlayerCard();
			if (pCard->isDecideMiss()) {
				continue;
			}
			pCard->onAutoSetMiss();
		}
	}
}

bool SCMJRoom::isAllPlayerConfirmMiss() {
	uint8_t nDecidedCnt = 0;
	for (auto& ref : m_vPlayers) {
		if (ref) {
			if (((SCMJPlayerCard*)((SCMJPlayer*)ref)->getPlayerCard())->isDecideMiss()) {
				nDecidedCnt++;
			}
		}
	}
	return nDecidedCnt == getSeatCnt();
}

void SCMJRoom::onShowPlayerMiss() {
	Json::Value jsMsg, jsDetail;
	jsMsg["state"] = 2;
	for (auto& ref : m_vPlayers) {
		if (ref) {
			Json::Value jsMiss;
			jsMiss["idx"] = ref->getIdx();
			jsMiss["missType"] = ((SCMJPlayerCard*)((SCMJPlayer*)ref)->getPlayerCard())->getMissType();
			jsDetail[jsDetail.size()] = jsMiss;
		}
	}
	jsMsg["detail"] = jsDetail;
	sendRoomMsg(jsMsg, MSG_ROOM_SCMJ_PLAYER_DECIDE_MISS);

	m_bIsShowMiss = true;
}

bool SCMJRoom::isEnableBloodFlow() {
	auto pSCMJOpts = std::dynamic_pointer_cast<SCMJOpts>(getDelegate()->getOpts());
	return pSCMJOpts->isEnableBloodFlow();
}

bool SCMJRoom::isZiMoAddFan() {
	auto pSCMJOpts = std::dynamic_pointer_cast<SCMJOpts>(getDelegate()->getOpts());
	return pSCMJOpts->isZiMoAddFan();
}

bool SCMJRoom::isDianGangZiMo() {
	auto pSCMJOpts = std::dynamic_pointer_cast<SCMJOpts>(getDelegate()->getOpts());
	return pSCMJOpts->isDianGangZiMo();
}

bool SCMJRoom::isEnableExchange3Cards() {
	auto pSCMJOpts = std::dynamic_pointer_cast<SCMJOpts>(getDelegate()->getOpts());
	return pSCMJOpts->isEnableExchange3Cards();
}

bool SCMJRoom::isEnable19() {
	auto pSCMJOpts = std::dynamic_pointer_cast<SCMJOpts>(getDelegate()->getOpts());
	return pSCMJOpts->isEnable19();
}

bool SCMJRoom::isEnableMenQing() {
	auto pSCMJOpts = std::dynamic_pointer_cast<SCMJOpts>(getDelegate()->getOpts());
	return pSCMJOpts->isEnableMenQing();
}

bool SCMJRoom::isEnableZZ() {
	auto pSCMJOpts = std::dynamic_pointer_cast<SCMJOpts>(getDelegate()->getOpts());
	return pSCMJOpts->isEnableZZ();
}

bool SCMJRoom::isEnableTDHu() {
	auto pSCMJOpts = std::dynamic_pointer_cast<SCMJOpts>(getDelegate()->getOpts());
	return pSCMJOpts->isEnableTDHu();
}

uint8_t SCMJRoom::getFanLimit() {
	auto pSCMJOpts = std::dynamic_pointer_cast<SCMJOpts>(getDelegate()->getOpts());
	return pSCMJOpts->getFanLimit();
}

void SCMJRoom::addSettle(stSettle& tSettle) {
	m_vSettle.push_back(tSettle);
}

void SCMJRoom::eraseLastGangSettle() {
	auto iter = m_vSettle.rbegin();
	while (iter != m_vSettle.rend()) {
		if (eMJAct_MingGang == iter->eSettleReason ||
			eMJAct_AnGang == iter->eSettleReason ||
			eMJAct_BuGang == iter->eSettleReason) {
			m_vSettle.erase((++iter).base());
			break;
		}
		++iter;
	}
}

void SCMJRoom::settleInfoToJson(Json::Value& jsRealTime) {
	for (auto& ref : m_vSettle) {
		Json::Value jsItem, jsRDetail;
		jsItem["actType"] = ref.eSettleReason;
		if (ref.eSettleReason == eMJAct_Hu) {
			jsItem["msg"] = ref.jsHuMsg;
		}

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

void SCMJRoom::onPlayerChaJiao(Json::Value& jsReal) {
	std::vector<SCMJPlayer*> vNotHu, vNotTing;
	for (auto& ref : m_vPlayers) {
		if (ref) {
			((SCMJPlayer*)ref)->zeroFlag();
			if (ref->haveState(eRoomPeer_AlreadyHu)) {
				continue;
			}
			if (((SCMJPlayer*)ref)->getPlayerCard()->isTingPai()) {
				vNotHu.push_back((SCMJPlayer*)ref);
			}
			else if (((SCMJPlayerCard*)((SCMJPlayer*)ref)->getPlayerCard())->isHuaZhu()) {
				continue;
			}
			else {
				vNotTing.push_back((SCMJPlayer*)ref);
			}
		}
	}

	if (vNotHu.empty() || vNotTing.empty()) {
		return;
	}

	Json::Value jsChaJiao;
	for (auto& ref : vNotTing) {
		jsChaJiao[jsChaJiao.size()] = ref->getIdx();
	}
	jsReal["chaJiaoIdx"] = jsChaJiao;

	Json::Value jsDetail;
	for (auto& ref : vNotHu) {
		Json::Value jsWin, jsLoses;
		jsWin["idx"] = ref->getIdx();
		uint32_t nTotalWin = 0;
		uint32_t nLose = 0;
		for (auto& ref_1 : vNotTing) {
			if (nLose == 0) {
				nLose = getMaxChaJiaoScore(ref->getIdx(), ref_1->getIdx());
			}
			if (nLose) {
				Json::Value jsLose;
				ref_1->addSingleOffset(-1 * (int32_t)nLose);
				jsLose["idx"] = ref_1->getIdx();
				jsLose["lose"] = nLose;
				jsLoses[jsLoses.size()] = jsLose;
				nTotalWin += nLose;
			}
		}
		ref->addSingleOffset(nTotalWin);
		jsWin["win"] = nTotalWin;
		jsWin["loses"] = jsLoses;
		jsDetail[jsDetail.size()] = jsWin;
	}
	jsReal["detail"] = jsDetail;
}

void SCMJRoom::onPlayerHuaZhu(Json::Value& jsReal) {
	std::vector<SCMJPlayer*> vNotHu, vHuaZhu;
	for (auto& ref : m_vPlayers) {
		if (ref) {
			if (ref->haveState(eRoomPeer_AlreadyHu)) {
				continue;
			}
			if (((SCMJPlayerCard*)((SCMJPlayer*)ref)->getPlayerCard())->isHuaZhu()) {
				vHuaZhu.push_back((SCMJPlayer*)ref);
			}
			else{
				vNotHu.push_back((SCMJPlayer*)ref);
			}
		}
	}

	if (vHuaZhu.empty()) {
		return;
	}
	Json::Value jsHuaZhu;
	for (auto& ref : vHuaZhu) {
		jsHuaZhu[jsHuaZhu.size()] = ref->getIdx();
	}
	jsReal["huaZhuIdx"] = jsHuaZhu;
	if (vNotHu.empty()) {
		return;
	}

	uint16_t nLose = 1;
	uint8_t nFanCnt = getFanLimit();
	for (uint8_t i = 0; i < nFanCnt; ++i) nLose *= 2;
	Json::Value jsDetail;
	for (auto& ref : vNotHu) {
		Json::Value jsWin, jsLoses;
		jsWin["idx"] = ref->getIdx();
		uint16_t nTotalWin = 0;
		for (auto& ref_1 : vHuaZhu) {
			Json::Value jsLose;
			ref_1->addSingleOffset(-1 * (int32_t)nLose);
			jsLose["idx"] = ref_1->getIdx();
			jsLose["lose"] = nLose;
			jsLoses[jsLoses.size()] = jsLose;
			nTotalWin += nLose;
		}
		ref->addSingleOffset(nTotalWin);
		jsWin["win"] = nTotalWin;
		jsWin["loses"] = jsLoses;
		jsDetail[jsDetail.size()] = jsWin;
	}
	jsReal["detail"] = jsDetail;
}

void SCMJRoom::sortFanxing2FanCnt(std::vector<eFanxingType>& vType, uint16_t& nFanCnt) {
	for (auto& ref : vType) {
		switch (ref)
		{
		case eFanxing_DiHu:
		case eFanxing_TianHu:
		{
			nFanCnt += getFanLimit();
			break;
		}
		case eFanxing_ShuangQiDui:
		{
			nFanCnt += 3;
			break;
		}
		case eFanxing_QingYiSe:
		case eFanxing_QiDui:
		{
			nFanCnt += 2;
			break;
		}
		case eFanxing_DuiDuiHu:
		case eFanxing_GangKai:
		case eFanxing_QuanQiuDuDiao:
		case eFanxing_QiangGang:
		case eFanxing_GangHouPao:
		case eFanxing_HaiDiLaoYue:
		case eFanxing_SC_19JiangDui:
		case eFanxing_MengQing:
		case eFanxing_SC_ZhongZhang:
		{
			nFanCnt += 1;
			break;
		}
		}
	}
}

uint32_t SCMJRoom::getMaxChaJiaoScore(uint8_t idx, uint8_t nInvokeIdx) {
	auto pPlayer = (SCMJPlayer*)getPlayerByIdx(idx);
	auto pCard = (SCMJPlayerCard*)pPlayer->getPlayerCard();
	std::vector<eMJCardType> vType;
	pCard->getQueType(vType);
	uint32_t nScore = 0;
	for (auto& type : vType) {
		if (type < eCT_Wan || type > eCT_Tiao) {
			continue;
		}
		for (uint8_t i = 1; i < 10; i++) {
			auto nCard = make_Card_Num(type, i);
			if (pCard->canHuWitCard(nCard)) {
				pCard->onDoHu(nInvokeIdx, nCard, false);
				std::vector<eFanxingType> vType;
				uint16_t nFanCnt = 0;
				uint32_t nLoseCoin = 0;
				m_cFanxingChecker.checkFanxing(vType, pPlayer, nInvokeIdx, this);
				sortFanxing2FanCnt(vType, nFanCnt);
				for (int32_t i = 0; i < nFanCnt; ++i) nLoseCoin *= 2;
				nLoseCoin += std::count(vType.begin(), vType.end(), eFanxing_SC_Gen);
				if (nLoseCoin > nScore) {
					nScore = nLoseCoin;
				}
				pCard->endDoHu(nCard);
			}
		}
	}
	return nScore;
}