#include "NCMJRoom.h"
#include "NCMJPlayer.h"
#include "CommonDefine.h"
#include "MJRoomStateAutoStartWaitReady.h"
#include "MJRoomStateWaitPlayerChu.h"
#include "NCMJRoomStateWaitPlayerAct.h"
#include "NCMJRoomStateStartGame.h"
#include "MJRoomStateGameEnd.h"
#include "NCMJRoomStateDoPlayerAct.h"
#include "NCMJRoomStateAskForRobotGang.h"
#include "NCMJRoomStateAskForPengOrHu.h"
#include "NCMJRoomStateAfterChiOrPeng.h"
#include "MJReplayFrameType.h"
#include "IGameRoomDelegate.h"
#include "NCMJOpts.h"
bool NCMJRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)
{
	IMJRoom::init(pRoomMgr,nSeialNum,nRoomID,ptrGameOpts);
	clearOneCircleEnd();
	// add room state ;
	IGameRoomState* p[] = { new MJRoomStateAutoStartWaitReady(), new MJRoomStateWaitPlayerChu(),new NCMJRoomStateWaitPlayerAct(),
		new NCMJRoomStateStartGame(),new MJRoomStateGameEnd(),new NCMJRoomStateDoPlayerAct(),new NCMJRoomStateAskForRobotGang(),
		new NCMJRoomStateAskForPengOrHu(), new NCMJRoomStateAfterChiOrPeng()};
	for ( auto& pS : p )
	{
		addRoomState(pS);
	}
	setInitState(p[0]);
	return true;
}

IGamePlayer* NCMJRoom::createGamePlayer()
{
	return new NCMJPlayer();
}

void NCMJRoom::packRoomInfo(Json::Value& jsRoomInfo) {
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

void NCMJRoom::visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID) {
	if (!pPlayer)
	{
		return;
	}

	IMJRoom::visitPlayerInfo(pPlayer, jsPlayerInfo, nVisitorSessionID);
	jsPlayerInfo["extraTime"] = ((NCMJPlayer*)pPlayer)->getExtraTime();

	if (pPlayer->haveState(eRoomPeer_CanAct) == false)  // not join this round game ;
	{
		return;
	}

	((NCMJPlayer*)pPlayer)->getPlayerCard()->onVisitPlayerCardInfo(jsPlayerInfo, pPlayer->getSessionID() == nVisitorSessionID);
}

void NCMJRoom::onWillStartGame() {
	IMJRoom::onWillStartGame();
	m_vSettle.clear();

	doProduceNewBanker();
	sendWillStartGameMsg();
}

void NCMJRoom::onStartGame() {
	IMJRoom::onStartGame();
	sendStartGameMsg();
}

void NCMJRoom::onGameEnd() {
	Json::Value jsReal, jsPlayers;
	bool isHuangZhuang = true;
	settleInfoToJson(jsReal, isHuangZhuang);
	if (isHuangZhuang) {
		setNextBankerIdx();
	}

	for (auto& ref : m_vPlayers) {
		if (ref) {
			Json::Value jsHoldCard, jsPlayer;
			IMJPlayerCard::VEC_CARD vHoldCard;
			((NCMJPlayer*)ref)->getPlayerCard()->getHoldCard(vHoldCard);
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

bool NCMJRoom::isCanGoOnMoPai() {
	return getPoker()->getLeftCardCount() > 16;
}

void NCMJRoom::onPlayerMo(uint8_t nIdx) {
	IMJRoom::onPlayerMo(nIdx);
	auto pPlayer = (NCMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->clearGangFlag();
}

void NCMJRoom::onPlayerChu(uint8_t nIdx, uint8_t nCard) {
	auto pPlayer = (NCMJPlayer*)getPlayerByIdx(nIdx);
	bool haveGangFlag = false;
	if (pPlayer->haveFlag(IMJPlayer::eMJActFlag::eMJActFlag_Gang)) {
		haveGangFlag = true;
	}
	IMJRoom::onPlayerChu(nIdx, nCard);
	if (haveGangFlag) {
		pPlayer->signFlag(IMJPlayer::eMJActFlag::eMJActFlag_Gang);
	}
}

void NCMJRoom::onPlayerEat(uint8_t nIdx, uint8_t nCard, uint8_t nWithA, uint8_t nWithB, uint8_t nInvokeIdx) {
	IMJRoom::onPlayerEat(nIdx, nCard, nWithA, nWithB, nInvokeIdx);

	auto pPlayer = (NCMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->clearGangFlag();
}

void NCMJRoom::onPlayerPeng(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	IMJRoom::onPlayerPeng(nIdx, nCard, nInvokeIdx);

	auto pPlayer = (NCMJPlayer*)getPlayerByIdx(nIdx);
	pPlayer->clearGangFlag();
}

void NCMJRoom::onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx) {
	IMJRoom::onPlayerMingGang(nIdx, nCard, nInvokeIdx);

	stSettle st;
	st.eSettleReason = eMJAct_MingGang;
	uint16_t nWinCoin = 0;
	uint16_t nLoseCoin = getBaseScore();
	for (auto& pp : m_vPlayers) {
		if (pp) {
			if (pp->getIdx() == nIdx) {
				((NCMJPlayer*)pp)->setLastGangType(eMJAct_MingGang);
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

void NCMJRoom::onPlayerAnGang(uint8_t nIdx, uint8_t nCard) {
	IMJRoom::onPlayerAnGang(nIdx, nCard);

	stSettle st;
	st.eSettleReason = eMJAct_AnGang;
	uint16_t nWinCoin = 0;
	uint16_t nLoseCoin = 2 * getBaseScore();
	for (auto& pp : m_vPlayers) {
		if (pp) {
			if (pp->getIdx() == nIdx) {
				((NCMJPlayer*)pp)->setLastGangType(eMJAct_AnGang);
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

void NCMJRoom::onPlayerBuGang(uint8_t nIdx, uint8_t nCard) {
	IMJRoom::onPlayerBuGang(nIdx, nCard);

	stSettle st;
	st.eSettleReason = eMJAct_BuGang;
	uint16_t nWinCoin = 0;
	uint16_t nLoseCoin = getBaseScore();
	for (auto& pp : m_vPlayers) {
		if (pp) {
			if (pp->getIdx() == nIdx) {
				((NCMJPlayer*)pp)->setLastGangType(eMJAct_BuGang);
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

void NCMJRoom::onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx) {
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
		auto pZiMoPlayer = (NCMJPlayer*)getPlayerByIdx(nInvokeIdx);
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
		auto pZiMoPlayerCard = (NCMJPlayerCard*)pZiMoPlayer->getPlayerCard();
		pZiMoPlayerCard->getLastHuFanxing(vType);
		auto nFanCnt = pZiMoPlayerCard->getLastHuCnt();

		//自摸1番
		//nFanCnt += 1;

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

				//nTLoseCoin += (pZiMoPlayer->getRace() + pLosePlayer->getRace());

				st.addLose(pLosePlayer->getIdx(), nTLoseCoin);
				nTotalWin += nTLoseCoin;

				//sort race
				int32_t nRace = pZiMoPlayer->getRace() + pLosePlayer->getRace();
				pZiMoPlayer->addSingleOffset(nRace);
				pLosePlayer->addSingleOffset(-1 * nRace);
			}
		}
		st.addWin(nInvokeIdx, nTotalWin);
		LOGFMTD("room id = %u player = %u zimo", getRoomID(), nInvokeIdx);
	}
	else {
		auto pLosePlayer = (NCMJPlayer*)getPlayerByIdx(nInvokeIdx);
		if (!pLosePlayer)
		{
			LOGFMTE("room id = %u lose but player idx = %u is nullptr", getRoomID(), nInvokeIdx);
			return;
		}
		jsDetail["dianPaoIdx"] = pLosePlayer->getIdx();
		pLosePlayer->addDianPaoCnt();
		std::vector<uint8_t> vOrderHu;
		uint8_t nSeatCnt = getSeatCnt();
		if (vHuIdx.size() > 0)
		{
			for (uint8_t offset = 1; offset < nSeatCnt; ++offset)
			{
				uint8_t nCheckIdx = (nInvokeIdx + offset) % nSeatCnt;
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
			auto pHuPlayer = (NCMJPlayer*)getPlayerByIdx(nHuIdx);
			if (pHuPlayer == nullptr)
			{
				LOGFMTE("room id = %u hu player idx = %u , is nullptr", getRoomID(), nHuIdx);
				continue;
			}
			pHuPlayer->addHuCnt();

			Json::Value jsHuPlayer;
			jsHuPlayer["idx"] = nHuIdx;
			pHuPlayer->setState(eRoomPeer_AlreadyHu);

			auto pHuPlayerCard = (NCMJPlayerCard*)pHuPlayer->getPlayerCard();
			std::vector<eFanxingType> vType;
			pHuPlayerCard->getLastHuFanxing(vType);
			auto nFanCnt = pHuPlayerCard->getLastHuCnt();

			//pHuPlayerCard->onDoHu(nInvokeIdx, nCard, pLosePlayer->haveGangFlag());
			//bool isGHP = std::find(vType.begin(), vType.end(), eFanxing_GangHouPao) != vType.end();
			
			//抢杠1番(等同于自摸)
			bool bRotGang = pLosePlayer->haveFlag(IMJPlayer::eMJActFlag_DeclBuGang);
			if (bRotGang) {
				//nFanCnt++;
				vType.push_back(eFanxing_QiangGang);
				pLosePlayer->signGangHouPao();
			}

			bool isGangHouPao = pLosePlayer->haveGangFlag();
			if (isGangHouPao) {
				nFanCnt++;
				vType.push_back(eFanxing_GangHouPao);
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

			if (nInvokeIdx == getBankerIdx() || nHuIdx == getBankerIdx()) {
				nWinCoin *= 2;//庄1番
			}

			//guapu
			auto pInvoker = (NCMJPlayer*)getPlayerByIdx(nInvokeIdx);
			int32_t nRace = pInvoker->getRace() + pHuPlayer->getRace();
			pHuPlayer->addSingleOffset(nRace);
			pInvoker->addSingleOffset(-1 * nRace);

			st.addLose(nInvokeIdx, nWinCoin);
			st.addWin(nHuIdx, nWinCoin);
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

void NCMJRoom::onPlayerLouHu(uint8_t nIdx, uint8_t nInvokerIdx) {
	IMJRoom::onPlayerLouHu(nIdx, nInvokerIdx);

	((NCMJPlayerCard*)((NCMJPlayer*)getPlayerByIdx(nIdx))->getPlayerCard())->onLouHu();
}

void NCMJRoom::sendStartGameMsg() {
	// bind room to player card 
	Json::Value jsMsg;
	//getDelegate()->packStartGameMsg(jsMsg);

	//for (auto& pPlayer : m_vPlayers)
	//{
	//	if (pPlayer == nullptr)
	//	{
	//		LOGFMTE("room id = %u , start game player is nullptr", getRoomID());
	//		continue;
	//	}
	//	Json::Value jsPerPu;
	//	auto pPlayerCard = ((NCMJPlayer*)pPlayer)->getPlayerCard();
	//	jsPerPu["idx"] = pPlayer->getIdx();
	//	jsPerPu["race"] = pPlayer->getRace();
	//	jsPu[jsPu.size()] = jsPerPu;
	//}
	//jsMsg["races"] = jsPu;
	////sendRoomMsg(jsMsg, MSG_ROOM_MQMJ_GAME_START);

	//// replay arg 
	//addReplayFrame(eMJFrame_Xia_Zhu, jsPu);

	//jsMsg["bankerIdx"] = getBankerIdx();

	//Json::Value arrPeerCards;
	for (auto& pPlayer : m_vPlayers)
	{
		if (!pPlayer)
		{
			LOGFMTE("why player is null hz mj must all player is not null");
			continue;
		}
		Json::Value peer;
		auto pPlayerCard = ((NCMJPlayer*)pPlayer)->getPlayerCard();
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

void NCMJRoom::sendWillStartGameMsg() {
	Json::Value jsMsg, jsPu;
	getDelegate()->packStartGameMsg(jsMsg);
	jsMsg["bankerIdx"] = getBankerIdx();

	sendRoomMsg(jsMsg, MSG_ROOM_CFMJ_GAME_WILL_START);
}

void NCMJRoom::doProduceNewBanker() {
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
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(m_nNextBankerIdx);
	pPlayer->addBankerCnt();
}

void NCMJRoom::setNextBankerIdx(uint8_t nHuIdx) {
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

uint8_t NCMJRoom::getRoomType()
{
	return eGame_NCMJ;
}

IPoker* NCMJRoom::getPoker()
{
	return &m_tPoker;
}

bool NCMJRoom::isHaveRace() {
	return true;
}

void NCMJRoom::onWaitRace(uint8_t nIdx) {
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

void NCMJRoom::onAskForPengOrHuThisCard(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint16_t>& vOutWaitHuIdx, std::vector<uint16_t>& vOutWaitPengGangIdx, bool& isNeedWaitEat) {
	for (uint8_t i = 1; i < getSeatCnt(); i++)
	{
		uint8_t idx = (nInvokeIdx + i) % getSeatCnt();
		auto ref = (NCMJPlayer*)getPlayerByIdx(idx);

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

void NCMJRoom::onAskForRobotGang(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates) {
	for (uint8_t i = 1; i < getSeatCnt(); i++)
	{
		uint8_t idx = (nInvokeIdx + i) % getSeatCnt();
		auto ref = (NCMJPlayer*)getPlayerByIdx(idx);
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

bool NCMJRoom::needChu() {
	return getPoker()->getLeftCardCount() >= 20;
}

bool NCMJRoom::canGang() {
	return needChu();
}

uint8_t NCMJRoom::getFanLimit() {
	auto pNCMJOpts = std::dynamic_pointer_cast<NCMJOpts>(getDelegate()->getOpts());
	return pNCMJOpts->getFanLimit();
}

void NCMJRoom::addSettle(stSettle& tSettle) {
	m_vSettle.push_back(tSettle);
}

void NCMJRoom::settleInfoToJson(Json::Value& jsRealTime, bool& bHuangZhuang) {
	bHuangZhuang = m_vSettle.rbegin()->eSettleReason != eMJAct_Hu;

	std::vector<stSettle>::reverse_iterator rit;
	for (rit = m_vSettle.rbegin(); rit != m_vSettle.rend(); rit++) {
		Json::Value jsItem, jsRDetail;
		jsItem["actType"] = rit->eSettleReason;
		if (rit->eSettleReason == eMJAct_Hu) {
			jsItem["msg"] = rit->jsHuMsg;
		}

		bool bIgnore = false;
		if (rit->vWinIdxs.empty() == false) {
			auto pWinPlayer = (NCMJPlayer*)getPlayerByIdx(rit->vWinIdxs[0]);
			if (pWinPlayer->isGangHouPao()) {
				if (rit->eSettleReason == pWinPlayer->getLaseGangType()) {
					bIgnore = true;
					pWinPlayer->clearGangHouPao();
				}
			}
		}
		

		uint32_t nTotalGain = 0;
		for (auto& refl : rit->vLoseIdx)
		{
			auto pPlayer = getPlayerByIdx(refl.first);
			if (pPlayer) {
				int32_t nOffset = bIgnore ? 0 : -1 * (int32_t)refl.second;
				if (bHuangZhuang) {
					nOffset = -1 * nOffset;
				}
				pPlayer->addSingleOffset(nOffset);
				Json::Value jsPlayer;
				jsPlayer["idx"] = refl.first;
				jsPlayer["offset"] = nOffset;
				jsRDetail[jsRDetail.size()] = jsPlayer;
			}
		}

		for (auto& refl : rit->vWinIdxs)
		{
			auto pPlayer = getPlayerByIdx(refl.first);
			if (pPlayer) {
				int32_t nOffset = bIgnore ? 0 : refl.second;
				if (bHuangZhuang) {
					nOffset = -1 * nOffset;
				}
				pPlayer->addSingleOffset(nOffset);
				Json::Value jsPlayer;
				jsPlayer["idx"] = refl.first;
				jsPlayer["offset"] = nOffset;
				jsRDetail[jsRDetail.size()] = jsPlayer;
			}
		}

		jsItem["detial"] = jsRDetail;
		jsRealTime[jsRealTime.size()] = jsItem;
	}
}

bool NCMJRoom::onWaitPlayerActAfterCP(uint8_t nIdx) {
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

bool NCMJRoom::isGameOver() {
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