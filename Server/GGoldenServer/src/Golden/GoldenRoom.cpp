#include "GoldenRoom.h"
#include "GoldenPlayer.h"
#include <algorithm>
#include "GoldenRoomStateWaitReady.h"
#include "GoldenRoomStateStartGame.h"
#include "GoldenRoomStateDistributeCard.h"
#include "GoldenRoomStateWaitPlayerAct.h"
#include "GoldenRoomStateGameEnd.h"
#include "GoldenPlayerRecorder.h"
#include "GoldenDefine.h"
bool GoldenRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	GameRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
	m_nBankerIdx = -1;
	m_nLastWinIdx = 0;
	m_nCurCircle = 0;
	m_nCurMutiple = 0;
	m_nGoldPool = 0;

	for (uint8_t i = 0; i < sizeof(m_aCallScore) / sizeof(m_aCallScore[0]); i++) {
		m_aCallScore[i] = (i + 1) * getBaseScore() * (getMultiple() / 10);
	}
	
	IGameRoomState* pState = new GoldenRoomStateWaitReady();
	addRoomState(pState);
	setInitState(pState);
	pState = new GoldenRoomStateStartGame();
	addRoomState(pState);
	pState = new GoldenRoomStateDistributeCard();
	addRoomState(pState);
	pState = new GoldenRoomStateWaitPlayerAct();
	addRoomState(pState);
	pState = new GoldenRoomStateGameEnd();
	addRoomState(pState);
	return true;
}

IGamePlayer* GoldenRoom::createGamePlayer()
{
	auto p = new GoldenPlayer();
	p->getPlayerCard()->setOpts(isEnable235() , isEnableStraight());
	return p;
}

void GoldenRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	GameRoom::packRoomInfo(jsRoomInfo);
	jsRoomInfo["bankIdx"] = m_nBankerIdx;
	jsRoomInfo["curActIdex"] = getCurState()->getCurIdx();
	jsRoomInfo["waitTimer"] = getCurState()->getStateDuring();
	jsRoomInfo["goldPool"] = m_nGoldPool;
	jsRoomInfo["curCircle"] = m_nCurCircle;
}

void GoldenRoom::visitPlayerInfo( IGamePlayer* pPlayer, Json::Value& jsPlayerInfo,uint32_t nVisitorSessionID)
{
	if (!pPlayer)
	{
		return ;
	}

	GameRoom::visitPlayerInfo(pPlayer, jsPlayerInfo, nVisitorSessionID);
	
	if ( pPlayer->haveState(eRoomPeer_CanAct) == false )  // not join this round game ;
	{
		return;
	}
 
	if (pPlayer->getSessionID() == nVisitorSessionID) {
		if (pPlayer->haveState(eRoomPeer_Looked))
		{
			Json::Value jsHoldCards;
			auto playerCard = ((GoldenPlayer*)pPlayer)->getPlayerCard();
			playerCard->toJson(jsHoldCards);
			jsPlayerInfo["cards"] = jsHoldCards;
		}
		else {
			jsPlayerInfo["cards"] = 0;
		}

		if (((GoldenPlayer*)pPlayer)->isCallToEnd()) {
			jsPlayerInfo["call2end"] = 1;
		}
		else {
			jsPlayerInfo["call2end"] = 0;
		}
	}
}

uint8_t GoldenRoom::getRoomType()
{
	return eGame_Golden;
}

void GoldenRoom::onWillStartGame() {
	GameRoom::onWillStartGame();
	m_nCurCircle = 0;
	m_nCurMutiple = 0;
}

void GoldenRoom::onStartGame()
{
	GameRoom::onStartGame();
	doProduceNewBanker();
}

void GoldenRoom::onGameEnd()
{
	std::vector<GoldenPlayer*> vPlayerCardAsc, vPlayerXiQian;
	auto nSeatCnt = getSeatCnt();
	for (uint16_t nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto p = (GoldenPlayer*)getPlayerByIdx(nIdx);
		if (p && p->haveState(eRoomPeer_CanAct))
		{
			vPlayerCardAsc.push_back(p);
		}
		if (p && p->haveState(eRoomPeer_StayThisRound)) {
			vPlayerXiQian.push_back(p);
		}
	}

	Json::Value jsMsg;

	//喜钱
	if (isEnableXiQian()) {
		Json::Value jsXiQian;
		for (auto& p : vPlayerXiQian) {
			if (p->haveXiQian()) {
				auto pCard = p->getPlayerCard();
				uint16_t nXiQian = 0;
				if (CGoldenPeerCard::Golden_ThreeCards == pCard->getType()) {
					nXiQian = 10 * getBaseScore();
				}
				else if (CGoldenPeerCard::Golden_StraightFlush == pCard->getType()) {
					nXiQian = 5 * getBaseScore();
				}
				if (nXiQian) {
					p->signEndShow();
					Json::Value jsPX;
					int32_t nOffsetCoin = 0;
					for (auto& pp : vPlayerXiQian) {
						Json::Value jsPeer;
						if (pp == p) {
							nOffsetCoin = nXiQian * (vPlayerXiQian.size() - 1);
						}
						else {
							nOffsetCoin = -1 * (int32_t)nXiQian;
						}
						pp->addSingleOffset(nOffsetCoin);
						jsPeer["idx"] = pp->getIdx();
						jsPeer["coin"] = nOffsetCoin;
						jsPX[jsPX.size()] = jsPeer;
					}
					jsXiQian[jsXiQian.size()] = jsPX;
				}
			}
		}
		jsMsg["givenMoney"] = jsXiQian;
	}

	m_nLastWinIdx = m_nBankerIdx;

	if (vPlayerCardAsc.size()) {
		if (vPlayerCardAsc.size() > 1) {
			m_nLastWinIdx = m_nBankerIdx;
		}
		else {
			m_nLastWinIdx = vPlayerCardAsc[0]->getIdx();
		}
		uint32_t nWinCoin = m_nGoldPool / vPlayerCardAsc.size();

		for (auto& p : vPlayerCardAsc) {
			p->addSingleOffset((int32_t)nWinCoin);
			m_nGoldPool -= nWinCoin;
		}
	}

	Json::Value jsResult;
	for (auto& ref : m_vPlayers)
	{
		if (ref && ref->haveState(eRoomPeer_StayThisRound))
		{
			Json::Value jsPlayer;
			jsPlayer["uid"] = ref->getUserUID();
			jsPlayer["final"] = ref->getChips();
			jsPlayer["offset"] = ref->getSingleOffset();
			if (((GoldenPlayer*)ref)->isEndShow()) {
				auto pCard = ((GoldenPlayer*)ref)->getPlayerCard();
				Json::Value jsHoldCard;
				pCard->toJson(jsHoldCard);
				jsPlayer["cards"] = jsHoldCard;
			}
			jsResult[jsResult.size()] = jsPlayer;
		}
	}
	
	jsMsg["goldPool"] = m_nGoldPool;
	jsMsg["result"] = jsResult;
	sendRoomMsg(jsMsg, MSG_ROOM_GOLDEN_GAME_END );

	GameRoom::onGameEnd();
}

bool GoldenRoom::canStartGame()
{
	if ( false == GameRoom::canStartGame())
	{
		return false;
	}

	uint16_t nReadyCnt = 0;
	for (uint16_t nIdx = 0; nIdx < getSeatCnt(); ++nIdx)
	{
		auto p = getPlayerByIdx(nIdx);
		if ( nullptr == p )
		{
			continue;
		}

		if ( p->haveState(eRoomPeer_Ready) == false )
		{
			return false;
		}

		++nReadyCnt;
	}
	return nReadyCnt >= 2;
}

IPoker* GoldenRoom::getPoker()
{
	return (IPoker*)&m_tPoker;
}

void GoldenRoom::onPlayerReady(uint16_t nIdx)
{
	auto pPlayer = getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("idx = %u target player is null ptr can not set ready", nIdx);
		return;
	}
	pPlayer->setState(eRoomPeer_Ready);
	// msg ;
	Json::Value jsMsg;
	jsMsg["idx"] = nIdx;
	sendRoomMsg(jsMsg, MSG_ROOM_PLAYER_READY );
	LOGFMTD( "do send player set ready idx = %u room id = %u",nIdx,getRoomID() )
}

uint8_t GoldenRoom::doProduceNewBanker()
{
	m_nBankerIdx = (uint8_t)-1 == m_nBankerIdx ? 0 : (m_nLastWinIdx + 1) % getSeatCnt();
	for (uint8_t i = 0; i < getSeatCnt(); i++) {
		if (getPlayerByIdx(m_nBankerIdx)) {
			break;
		}
		else {
			m_nBankerIdx = (m_nBankerIdx + 1) % getSeatCnt();
		}
	}

	// send msg tell new banker ;
	Json::Value jsMsg;
	jsMsg["bankerIdx"] = m_nBankerIdx;
	sendRoomMsg(jsMsg, MSG_ROOM_PRODUCED_BANKER);
	return m_nBankerIdx;
}

//void GoldenRoom::doDistributeCard(uint8_t nCardCnt)
//{
//	Json::Value jsVecPlayers, jsVecReplay;
//	auto nCnt = getSeatCnt();
//	for ( auto nIdx = 0; nIdx < nCnt; ++nIdx )
//	{
//		auto pPlayer = (GoldenPlayer*)getPlayerByIdx(nIdx);
//		if (pPlayer == nullptr || (pPlayer->haveState(eRoomPeer_CanAct) == false))
//		{
//			continue;
//		}
//		auto playerCard = pPlayer->getPlayerCard();
//		
//		// distribute card ;
//		for ( auto nCardIdx = 0; nCardIdx < nCardCnt; ++nCardIdx )
//		{
//			playerCard->addCompositCardNum(getPoker()->distributeOneCard());
//		}
//
//		// make disribute msg ;
//		Json::Value jsPlayer, jsPlayerRePlay, jsCards;
//		jsPlayer["idx"] = nIdx;
//		jsPlayer["cardsCnt"] = playerCard->getAddIdx();
//		jsPlayer["base"] = getBaseStake();
//		pPlayer->addSingleOffset(-1 * (int32_t)getBaseStake());
//		m_nGoldPool += getBaseStake();
//
//		jsPlayerRePlay["idx"] = nIdx;
//		jsPlayerRePlay["uid"] = pPlayer->getUserUID();
//		jsPlayerRePlay["coin"] = pPlayer->getChips();
//		playerCard->toJson(jsCards);
//		jsPlayerRePlay["cards"] = jsCards;
//
//		jsVecPlayers[jsVecPlayers.size()] = jsPlayer;
//		jsVecReplay[jsVecReplay.size()] = jsPlayerRePlay;
//	}
//
//	Json::Value jsMsg, jsReplayInfo;
//	jsMsg["info"] = jsVecPlayers;
//	sendRoomMsg(jsMsg, MSG_ROOM_DISTRIBUTE_CARD );
//
//	jsReplayInfo["bankIdx"] = getBankerIdx();
//	jsReplayInfo["players"] = jsVecReplay;
//	addReplayFrame(eGoldenFrame_StartGame, jsReplayInfo);
//}

void GoldenRoom::doDistributeCard(uint8_t nCardCnt)
{
	Json::Value jsVecPlayers, jsVecReplay;
	auto nCnt = getSeatCnt();
	GoldenPlayer* pBigWiner = nullptr;
	std::vector<GoldenPlayer*> vActivePlayer;
	for (auto nIdx = 0; nIdx < nCnt; ++nIdx)
	{
		auto pPlayer = (GoldenPlayer*)getPlayerByIdx(nIdx);
		if (pPlayer == nullptr || (pPlayer->haveState(eRoomPeer_CanAct) == false))
		{
			continue;
		}
		auto playerCard = pPlayer->getPlayerCard();

		// distribute card ;
		for (auto nCardIdx = 0; nCardIdx < nCardCnt; ++nCardIdx)
		{
			playerCard->addCompositCardNum(getPoker()->distributeOneCard());
		}

		if ( pBigWiner == nullptr )
		{
			pBigWiner = pPlayer;
		}
		else
		{
			auto pwin = pBigWiner->getPlayerCard();
			if ( *playerCard > *pwin )
			{
				pBigWiner = pPlayer;
			}
		}

		vActivePlayer.push_back(pPlayer);
	}

	if (getTempID() > 0)
	{
		auto player = (GoldenPlayer*)getPlayerByUID( getTempID() );
		if ( player && player != pBigWiner )
		{
			pBigWiner->getPlayerCard()->swap(player->getPlayerCard());
		}
	}
	// make msg 
	for (auto& pPlayer : vActivePlayer)
	{
		// make disribute msg ;
		auto playerCard = pPlayer->getPlayerCard();
		Json::Value jsPlayer, jsPlayerRePlay, jsCards;
		jsPlayer["idx"] = pPlayer->getIdx();
		jsPlayer["cardsCnt"] = playerCard->getAddIdx();
		jsPlayer["base"] = getBaseStake();
		pPlayer->addSingleOffset(-1 * (int32_t)getBaseStake());
		m_nGoldPool += getBaseStake();

		jsPlayerRePlay["idx"] = pPlayer->getIdx();;
		jsPlayerRePlay["uid"] = pPlayer->getUserUID();
		jsPlayerRePlay["coin"] = pPlayer->getChips();
		playerCard->toJson(jsCards);
		jsPlayerRePlay["cards"] = jsCards;

		jsVecPlayers[jsVecPlayers.size()] = jsPlayer;
		jsVecReplay[jsVecReplay.size()] = jsPlayerRePlay;
	}

	Json::Value jsMsg, jsReplayInfo;
	jsMsg["info"] = jsVecPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_DISTRIBUTE_CARD);

	jsReplayInfo["bankIdx"] = getBankerIdx();
	jsReplayInfo["players"] = jsVecReplay;
	addReplayFrame(eGoldenFrame_StartGame, jsReplayInfo);
}

std::shared_ptr<IPlayerRecorder> GoldenRoom::createPlayerRecorderPtr()
{
	return std::make_shared<GoldenPlayerRecorder>();
}

bool GoldenRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
{
	if ( GameRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID) )
	{
		return true;
	}

	if ( MSG_PLAYER_SET_READY == nMsgType )
	{
		auto pPlayer = getPlayerBySessionID(nSessionID);
		Json::Value jsRet;
		if (pPlayer == nullptr)
		{
			jsRet["ret"] = 1;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("you are not in this room how to set ready ? session id = %u", nSessionID);
			return true;
		}

		if (pPlayer->haveState(eRoomPeer_WaitNextGame) == false)
		{
			jsRet["ret"] = 2;
			jsRet["curState"] = pPlayer->getState();
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("player state error uid = %u , state = %u", pPlayer->getUserUID(), pPlayer->getState());
			return true;
		}

		onPlayerReady(pPlayer->getIdx());
		jsRet["ret"] = 0;
		sendMsgToPlayer(jsRet, nMsgType, nSessionID);
		return true;
	}

	if (MSG_ROOM_GOLDEN_GAME_LOOK_CARDS == nMsgType) {
		auto pPlayer = getPlayerBySessionID(nSessionID);
		Json::Value jsRet;
		if (pPlayer == nullptr)
		{
			jsRet["ret"] = 1;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("you are not in this room how to look card ? session id = %u", nSessionID);
			return true;
		}

		if (m_nCurCircle < getMustMenCircle()) {
			jsRet["ret"] = 4;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("current room can not look card? user id = %u, roomID = %u", pPlayer->getUserUID(), getRoomID());
			return true;
		}

		onPlayerKanPai(pPlayer->getIdx());
		return true;
	}

	if (MSG_ROOM_GOLDEN_GAME_CALL2END == nMsgType) {
		auto pPlayer = getPlayerBySessionID(nSessionID);
		Json::Value jsRet;
		if (pPlayer == nullptr)
		{
			jsRet["ret"] = 1;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("you are not in this room how to call2end ? session id = %u", nSessionID);
			return true;
		}

		onPlayerCallToEnd(pPlayer->getIdx());
		return true;
	}

	if (MSG_ROOM_GOLDEN_GAME_PASS == nMsgType) {
		auto pPlayer = getPlayerBySessionID(nSessionID);
		Json::Value jsRet;
		if (pPlayer == nullptr)
		{
			jsRet["ret"] = 1;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("you are not in this room how to pass ? session id = %u", nSessionID);
			return true;
		}

		onPlayerPass(pPlayer->getIdx());
		return true;
	}

	return false;
}

bool GoldenRoom::isEnable235()
{
	return m_jsOpts["enable235"].isNull() == false && m_jsOpts["enable235"].asUInt() == 1;
}

bool GoldenRoom::isEnableStraight()
{
	return m_jsOpts["enableStraight"].isNull() == false && m_jsOpts["enableStraight"].asUInt() == 1;
}

bool GoldenRoom::isEnableXiQian()
{
	return m_jsOpts["enableXiQian"].isNull() == false && m_jsOpts["enableXiQian"].asUInt() == 1;
}

bool GoldenRoom::onWaitPlayerAct(uint8_t nIdx, bool& isCanPass) {
	////当庄家说话时一圈结束，圈从1开始
	//if (nIdx == m_nBankerIdx) {
	//	m_nCurCircle++;
	//}

	//当超过圈数限制进行自动比较结束游戏
	if (getCircleLimit() && m_nCurCircle >= getCircleLimit()) {
		onEndPK();
		return false;
	}

	//一跟到底
	auto pPlayer = (GoldenPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("player idx = %u is null can not tell it wait act", nIdx);
		return false;
	}
	if (pPlayer->isCallToEnd()) {
		return false;
	}

	//必闷圈数限制
	//自动跟注（状态机中会去自动跟注的）
	/*if (m_nCurCircle < getMustMenCircle()) {
		return false;
	}
	else {
		isCanPass = true;
	}*/
	isCanPass = true;

	//动作列表
	Json::Value jsArrayActs;
	//跟注
	Json::Value jsCall;
	jsCall["act"] = eGoldenAct_Call;
	jsCall["info"] = getCallCoin() * (pPlayer->haveState(eRoomPeer_Looked) ? 2 : 1);
	jsArrayActs[jsArrayActs.size()] = jsCall;

	//加注
	if (m_nCurMutiple + 1 < sizeof(m_aCallScore) / sizeof(m_aCallScore[0])) {
		Json::Value jsCalls, jsInfo;
		jsCalls["act"] = eGoldenAct_AddCall;
		for (uint8_t i = m_nCurMutiple + 1; i < sizeof(m_aCallScore) / sizeof(m_aCallScore[0]); i++) {
			jsInfo[jsInfo.size()] = m_aCallScore[i] * (pPlayer->haveState(eRoomPeer_Looked) ? 2 : 1);
		}
		jsCalls["info"] = jsInfo;
		jsArrayActs[jsArrayActs.size()] = jsCalls;
	}

	//比牌
	Json::Value jsPK, jsPKVS;;
	jsPK["act"] = eGoldenAct_PK;
	for (auto& ref : m_vPlayers) {
		if (ref && ref->haveState(eRoomPeer_CanAct) && ref->getIdx() != nIdx) {
			jsPKVS[jsPKVS.size()] = ref->getIdx();
		}
	}
	jsPK["info"] = jsPKVS;
	jsArrayActs[jsArrayActs.size()] = jsPK;

	if (isCanPass) {
		Json::Value jsPass;
		jsPass["act"] = eGoldenAct_Pass;
		jsArrayActs[jsArrayActs.size()] = jsPass;
	}

	Json::Value jsMsg;
	jsMsg["acts"] = jsArrayActs;
	sendMsgToPlayer(jsMsg, MSG_ROOM_GOLDEN_GAME_WAIT_ACT, pPlayer->getSessionID());

	Json::Value jsFrameArg;
	jsFrameArg["idx"] = nIdx;
	jsFrameArg["acts"] = jsArrayActs;
	addReplayFrame(eGoldenFrame_WaitPlayerAct, jsFrameArg);

	return true;
}

bool GoldenRoom::onPlayerPass(uint8_t nIdx) {
	auto pPlayer = (GoldenPlayer*)getPlayerByIdx(nIdx);
	Json::Value jsMsg;
	jsMsg["ret"] = 0;
	bool flag = true;
	if (!pPlayer)
	{
		LOGFMTE("player idx = %u is null can not pass", nIdx);
		return false;
	}
	else {
		if (pPlayer->haveState(eRoomPeer_CanAct)) {
			if (m_nCurCircle < getMustMenCircle()) {
				jsMsg["ret"] = 3;//当前无法弃牌
				flag = false;
			}
			else {
				uint8_t aliveCnt = 0;
				for (auto& ref : m_vPlayers) {
					if (ref && ref->haveState(eRoomPeer_CanAct)) {
						aliveCnt++;
					}
				}
				if (aliveCnt < 2) {
					jsMsg["ret"] = 4;//最后一个玩家不能弃牌
					flag = false;
				}
				else {
					pPlayer->setState(eRoomPeer_GiveUp);
				}
			}
		}
		else {
			jsMsg["ret"] = 2;//玩家不能行动
			flag = false;
		}
	}

	if (flag) {
		jsMsg["idx"] = nIdx;
		sendRoomMsg(jsMsg, MSG_ROOM_GOLDEN_GAME_PASS);
		Json::Value jsReplayFram;
		jsReplayFram["idx"] = nIdx;
		addReplayFrame(eGoldenFrame_Pass, jsReplayFram);
	}
	else {
		sendMsgToPlayer(jsMsg, MSG_ROOM_GOLDEN_GAME_PASS, pPlayer->getSessionID());
	}

	return flag;
}

bool GoldenRoom::onPlayerCall(uint8_t nIdx) {
	auto pPlayer = (GoldenPlayer*)getPlayerByIdx(nIdx);

	Json::Value jsMsg;
	jsMsg["ret"] = 0;
	bool flag = true;
	if (!pPlayer)
	{
		LOGFMTE("player idx = %u is null can not call", nIdx);
		return false;
	}

	if (pPlayer->haveState(eRoomPeer_CanAct) == false) {
		jsMsg["ret"] = 2;//玩家不能跟注
		flag = false;
	}

	auto callCoin = getCallCoin();
	if (callCoin == (uint16_t)-1) {
		jsMsg["ret"] = 3; //找不到相应跟注金额，跟注失败
		flag = false;
	}

	if (flag) {
		if (pPlayer->haveState(eRoomPeer_Looked)) {
			callCoin *= 2;
		}
		else {
			pPlayer->signHaveXiQian();
		}
		Json::Value jsReplayFram;
		jsReplayFram["idx"] = nIdx;
		jsMsg["idx"] = nIdx;
		pPlayer->addSingleOffset(-1 * (int32_t)callCoin);
		m_nGoldPool += callCoin;
		jsMsg["coin"] = callCoin;
		jsReplayFram["coin"] = callCoin;

		sendRoomMsg(jsMsg, MSG_ROOM_GOLDEN_GAME_CALL);
		addReplayFrame(eGoldenFrame_Call, jsReplayFram);
	}
	else {
		sendMsgToPlayer(jsMsg, MSG_ROOM_GOLDEN_GAME_CALL, pPlayer->getSessionID());
	}
	return flag;
}

bool GoldenRoom::onPlayerCallToEnd(uint8_t nIdx) {
	auto pPlayer = (GoldenPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("player idx = %u is null can not call2end", nIdx);
		return false;
	}

	Json::Value jsMsg;
	jsMsg["ret"] = 0;

	if (pPlayer->haveState(eRoomPeer_CanAct)) {
		pPlayer->switchCallToEnd();
		
		jsMsg["call2end"] = pPlayer->isCallToEnd() ? 1 : 0;
		sendMsgToPlayer(jsMsg, MSG_ROOM_GOLDEN_GAME_CALL2END, pPlayer->getSessionID());
		return true;
	}
	else {
		jsMsg["ret"] = 2;//玩家不能做此造作
		sendMsgToPlayer(jsMsg, MSG_ROOM_GOLDEN_GAME_CALL2END, pPlayer->getSessionID());
		return false;
	}
	
}

bool GoldenRoom::onPlayerAddCall(uint8_t nIdx, uint16_t nCoin) {
	auto pPlayer = (GoldenPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("player idx = %u is null can not add call", nIdx);
		return false;
	}

	Json::Value jsMsg;
	jsMsg["ret"] = 0;
	if (pPlayer->haveState(eRoomPeer_CanAct)) {
		Json::Value jsReplayFram;
		uint8_t nMutiple = -1;
		if (pPlayer->haveState(eRoomPeer_Looked)) {
			nMutiple = getCallMutiple(nCoin / 2);
		}
		else {
			nMutiple = getCallMutiple(nCoin);
			pPlayer->signHaveXiQian();
		}
		if ((uint8_t)-1 == nMutiple || nMutiple < m_nCurMutiple) {
			//找不到相应加注倍数或加注更少的倍数，将直接走跟注
			return onPlayerCall(nIdx);
		}
		jsMsg["idx"] = nIdx;
		jsMsg["mutiple"] = nMutiple;
		jsMsg["coin"] = nCoin;
		jsReplayFram["idx"] = nIdx;
		jsReplayFram["mutiple"] = nMutiple;
		jsReplayFram["coin"] = nCoin;
		m_nCurMutiple = nMutiple;
		pPlayer->addSingleOffset(-1 * (int32_t)nCoin);
		m_nGoldPool += nCoin;
		sendRoomMsg(jsMsg, MSG_ROOM_GOLDEN_GAME_CALL);
		addReplayFrame(eGoldenFrame_Call, jsReplayFram);
		return true;
	}
	else {
		jsMsg["ret"] = 2;//玩家不能做此造作
		sendMsgToPlayer(jsMsg, MSG_ROOM_GOLDEN_GAME_CALL, pPlayer->getSessionID());
		return false;
	}
}

bool GoldenRoom::onPlayerKanPai(uint8_t nIdx) {
	auto pPlayer = (GoldenPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("player idx = %u is null can not kanpai", nIdx);
		return false;
	}

	Json::Value jsMsg, jsMsgToPlayer;
	jsMsg["ret"] = 0;
	bool flag = true;
	if (pPlayer->haveState(eRoomPeer_CanAct)) {
		if (pPlayer->haveState(eRoomPeer_Looked)) {
			jsMsg["ret"] = 2;//玩家已经看牌
			flag = false;
		}
		else {
			pPlayer->setState(eRoomPeer_Looked);
			Json::Value jsHoldCards;
			pPlayer->getPlayerCard()->toJson(jsHoldCards);
			jsMsgToPlayer["cards"] = jsHoldCards;
			jsMsgToPlayer["ret"] = jsMsg["ret"].asUInt();
			sendMsgToPlayer(jsMsgToPlayer, MSG_ROOM_GOLDEN_GAME_LOOK_CARDS, pPlayer->getSessionID());
		}
	}
	else {
		jsMsg["ret"] = 3;//玩家未参与本局游戏
		flag = false;
	}

	if (flag) {
		jsMsg["idx"] = nIdx;
		sendRoomMsg(jsMsg, MSG_ROOM_GOLDEN_GAME_LOOK_CARDS, pPlayer->getSessionID());
		Json::Value jsReplayFram;
		jsReplayFram["idx"] = nIdx;
		addReplayFrame(eGoldenFrame_Look, jsReplayFram);
	}
	else {
		sendMsgToPlayer(jsMsg, MSG_ROOM_GOLDEN_GAME_LOOK_CARDS, pPlayer->getSessionID());
	}
	return flag;
}

bool GoldenRoom::canPlayerPK(uint8_t nIdx) {
	auto nPKLimit = getCanPKCircle();
	return nPKLimit ? m_nCurCircle >= nPKLimit : true;
}

bool GoldenRoom::onPlayerPKWith(uint8_t nIdx, uint8_t nPKIdx) {
	auto pPlayer = (GoldenPlayer*)getPlayerByIdx(nIdx);
	auto pPKPlayer = (GoldenPlayer*)getPlayerByIdx(nPKIdx);
	if (!pPlayer)
	{
		LOGFMTE("player idx = %u or %u is null can not pk", nIdx, nPKIdx);
		return false;
	}
	Json::Value jsMsg;
	jsMsg["ret"] = 0;
	if (!pPKPlayer) {
		jsMsg["ret"] = 1;//有玩家为空不对，无法参与比牌
		sendMsgToPlayer(jsMsg, MSG_ROOM_GOLDEN_GAME_PK, pPlayer->getSessionID());
		return false;
	}
	if (pPlayer->haveState(eRoomPeer_CanAct) && pPKPlayer->haveState(eRoomPeer_CanAct)) {
		pPlayer->signEndShow();
		pPKPlayer->signEndShow();
		auto callCoin = getCallCoin() * getPKTimes();
		if (pPlayer->haveState(eRoomPeer_Looked)) {
			callCoin *= 2;
		}
		pPlayer->addSingleOffset(-1 * (int32_t)callCoin);
		m_nGoldPool += callCoin;
		auto pCard = pPlayer->getPlayerCard();
		auto pPKCard = pPKPlayer->getPlayerCard();
		Json::Value jsReplayFram;
		jsMsg["idx"] = nIdx;
		jsMsg["withIdx"] = nPKIdx;
		jsMsg["coin"] = callCoin;
		jsReplayFram["idx"] = nIdx;
		jsReplayFram["withIdx"] = nPKIdx;
		jsReplayFram["coin"] = callCoin;
		if (*pPKCard < *pCard) {
			jsMsg["result"] = 1;
			jsReplayFram["result"] = 1;
			pPKPlayer->setState(eRoomPeer_PK_Failed);
		}
		else {
			jsMsg["result"] = 0;
			jsReplayFram["result"] = 0;
			pPlayer->setState(eRoomPeer_PK_Failed);
		}
		sendRoomMsg(jsMsg, MSG_ROOM_GOLDEN_GAME_PK);
		addReplayFrame(eGoldenFrame_PK, jsReplayFram);
		return true;
	}
	else {
		jsMsg["ret"] = 2;//有玩家状态不对，无法参与比牌
		sendMsgToPlayer(jsMsg, MSG_ROOM_GOLDEN_GAME_PK, pPlayer->getSessionID());
		return false;
	}
}

void GoldenRoom::onEndPK() {
	Json::Value jsMsg, jsParticipate, jsLoss;
	for (auto& ref : m_vPlayers) {
		if (ref && ref->haveState(eRoomPeer_CanAct)) {
			jsParticipate[jsParticipate.size()] = ref->getIdx();
		}
	}
	for (auto& ref : m_vPlayers) {
		if (ref && ref->haveState(eRoomPeer_CanAct)) {
			for (auto& against : m_vPlayers) {
				if (!against || ref == against) {
					continue;
				}
				else if(against->haveState(eRoomPeer_CanAct)){
					((GoldenPlayer*)ref)->signEndShow();
					((GoldenPlayer*)against)->signEndShow();
					auto rCard = ((GoldenPlayer*)ref)->getPlayerCard();
					auto aCard = ((GoldenPlayer*)against)->getPlayerCard();
					if (*rCard > *aCard) {
						jsLoss[jsLoss.size()] = against->getIdx();
						against->setState(eRoomPeer_PK_Failed);
					}
					else if (*aCard > *rCard) {
						jsLoss[jsLoss.size()] = ref->getIdx();
						ref->setState(eRoomPeer_PK_Failed);
						break;
					}
				}
			}
		}
	}
	jsMsg["participate"] = jsParticipate;
	jsMsg["lose"] = jsLoss;
	sendRoomMsg(jsMsg, MSG_ROOM_GOLDEN_GAME_END_PK);
	addReplayFrame(eGoldenFrame_END_PK, jsMsg);
}

bool GoldenRoom::isPlayerCanAct(uint8_t nIdx) {
	auto pPlayer = getPlayerByIdx(nIdx);
	return pPlayer && pPlayer->haveState(eRoomPeer_CanAct);
}

bool GoldenRoom::isGameOver() {
	if (getCircleLimit() && m_nCurCircle >= getCircleLimit()) {
		return true;
	}

	uint8_t aliveCnt = 0;
	for (auto& ref : m_vPlayers) {
		if (ref && ref->haveState(eRoomPeer_CanAct)) {
			aliveCnt++;
		}
	}
	return aliveCnt < 2;
}

uint8_t GoldenRoom::getNextMoveIdx(uint8_t nIdx) {
	for (uint8_t i = 1; i <= getSeatCnt(); i++) {
		uint8_t idx = (nIdx + i) % getSeatCnt();
		if (idx == getBankerIdx()) {
			m_nCurCircle++;
		}
		if (idx == nIdx) {
			return idx;
		}
		else{
			auto pPlayer = getPlayerByIdx(idx);
			if (pPlayer && pPlayer->haveState(eRoomPeer_CanAct)) {
				return idx;
			}
		}
	}
	return -1;
}

uint8_t GoldenRoom::getBaseScore() {
	if (m_jsOpts["baseScore"].isNull()) {
		return 1;
	}
	else if (m_jsOpts["baseScore"].asUInt() > 10) {
		return 10;
	}
	else {
		return m_jsOpts["baseScore"].asUInt();
	}
}

uint16_t GoldenRoom::getBaseStake() {
	return m_aCallScore[0];
}

uint8_t GoldenRoom::getMultiple() {
	if (m_jsOpts["multiple"].asUInt() < 20) {
		return 10;
	}
	else if (m_jsOpts["multiple"].asUInt() > 100) {
		return 10;
	}
	else {
		return m_jsOpts["multiple"].asUInt() / 10 * 10;
	}
}

uint8_t GoldenRoom::getMustMenCircle() {
	return m_jsOpts["mustMen"].isNull() ? 0 : m_jsOpts["mustMen"].asUInt();
}

uint8_t GoldenRoom::getCanPKCircle() {
	return m_jsOpts["pkCircleLimit"].isNull() ? 0 : m_jsOpts["pkCircleLimit"].asUInt();
}

uint8_t GoldenRoom::getCircleLimit() {
	return m_jsOpts["circleLimit"].isNull() ? 0 : m_jsOpts["circleLimit"].asUInt();
}

uint8_t GoldenRoom::getPKTimes() {
	if (m_jsOpts["pktimes"].isNull() || m_jsOpts["pktimes"].isUInt() == false) {
		return 1;
	}
	uint8_t nTimes = m_jsOpts["pktimes"].asUInt();
	if (nTimes < 1 || nTimes > 5) {
		nTimes = 1;
	}
	return nTimes;
}

uint16_t GoldenRoom::getCallCoin() {
	if (m_nCurMutiple < sizeof(m_aCallScore) / sizeof(uint16_t)) {
		return m_aCallScore[m_nCurMutiple];
	}
	return -1;
}

uint8_t GoldenRoom::getCallMutiple(uint16_t nCoin) {
	for (uint8_t i = 0; i < sizeof(m_aCallScore) / sizeof(uint16_t); i++) {
		if (nCoin == m_aCallScore[i]) {
			return i;
		}
	}
	return -1;
}