#include "ThirteenRoom.h"
#include "ThirteenPlayer.h"
#include "ThirteenRoomStateWaitReady.h"
#include "ThirteenRoomStateStartGame.h"
#include "ThirteenRoomStateDistributeCard.h"
#include "ThirteenRoomStateWaitPlayerAct.h"
#include "ThirteenRoomStateGameEnd.h"
#include "iterator"
#include "ThirteenPlayerRecorder.h"
bool ThirteenRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	GameRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
	
	IGameRoomState* pState = new ThirteenRoomStateWaitReady();
	addRoomState(pState);
	setInitState(pState);
	pState = new ThirteenRoomStateStartGame();
	addRoomState(pState);
	pState = new ThirteenRoomStateDistributeCard();
	addRoomState(pState);
	pState = new ThirteenRoomStateWaitPlayerAct();
	addRoomState(pState);
	pState = new ThirteenRoomStateGameEnd();
	addRoomState(pState);
	return true;
}

IGamePlayer* ThirteenRoom::createGamePlayer()
{
	auto p = new ThirteenPlayer();
	p->getPlayerCard()->setOpts();
	return p;
}

void ThirteenRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	GameRoom::packRoomInfo(jsRoomInfo);
	jsRoomInfo["curActIdex"] = getCurState()->getCurIdx();
	jsRoomInfo["waitTimer"] = getCurState()->getStateDuring();
}

void ThirteenRoom::visitPlayerInfo( IGamePlayer* pPlayer, Json::Value& jsPlayerInfo,uint32_t nVisitorSessionID)
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
		Json::Value jsHoldCards;
		auto pCard = ((ThirteenPlayer*)pPlayer)->getPlayerCard();
		pCard->holdCardToJson(jsHoldCards);
		jsPlayerInfo["holdCard"] = jsHoldCards;
		if (((ThirteenPlayer*)pPlayer)->hasDetermined()) {
			Json::Value jsGroupCards;
			pCard->groupCardToJson(jsGroupCards);
			jsPlayerInfo["groupCard"] = jsGroupCards;
		}
	}
}

uint8_t ThirteenRoom::getRoomType()
{
	return eGame_Thirteen;
}

void ThirteenRoom::onWillStartGame() {
	GameRoom::onWillStartGame();
}

void ThirteenRoom::onStartGame()
{
	GameRoom::onStartGame();
}

/*
	积分算法:
	{
	头道
	{{1,2,3}
	 {0,2,3}
	 {0,1,3}
	 {0,1,2}}
	 ,
	中道
	 {{1,2,3}
	 {0,2,3}
	 {0,1,3}
	 {0,1,2}}
	 ,
	尾道
	 {{1,2,3}
	 {0,2,3}
	 {0,1,3}
	 {0,1,2}}
	}
	先看是否打枪
	再算具体道赢得牌型分（如果为普通赢1道则在打抢的情况下不需要额外加）
	如果打枪某个玩家则赢6水，如果全垒打则赢13水
*/
void ThirteenRoom::onGameEnd()
{
	// find playing players ;
	std::vector<ThirteenPlayer*> vActivePlayers;
	auto nSeatCnt = getSeatCnt();
	for (uint8_t nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto p = getPlayerByIdx(nIdx);
		if (p && p->haveState(eRoomPeer_StayThisRound))
		{
			vActivePlayers.push_back((ThirteenPlayer*)p);
		}
	}

	// caculate per guo: Dao : winidx : loseridx
	std::vector<std::vector<std::vector<uint8_t>>> result;
	for (uint8_t nGuoIdx = DAO_HEAD; nGuoIdx < DAO_MAX; ++nGuoIdx)
	{
		std::vector<std::vector<uint8_t>> result_1;
		result_1.resize(nSeatCnt);
		result.push_back(result_1);
		for (auto& p : vActivePlayers)
		{
			p->getPlayerCard()->setCurGroupIdx(nGuoIdx);
		}

		std::sort(vActivePlayers.begin(), vActivePlayers.end(), [](ThirteenPlayer* pLeft, ThirteenPlayer* pRight)
		{
			auto pLeftCard = pLeft->getPlayerCard();
			auto pRightCard = pRight->getPlayerCard();
			return *pLeftCard < *pRightCard;
		});

		// caculate fen shu ;
		for (uint8_t i = 0; i < vActivePlayers.size(); i++) {
			for (uint8_t j = i + 1; j < vActivePlayers.size(); j++) {
				ThirteenPlayer* pWiner = nullptr;
				ThirteenPlayer* pLoser = nullptr;
				auto pLeftCard = vActivePlayers[i]->getPlayerCard();
				auto pRightCard = vActivePlayers[j]->getPlayerCard();
				if (*pLeftCard > *pRightCard) {
					pWiner = vActivePlayers[i];
					pLoser = vActivePlayers[j];
				}
				else {
					pLoser = vActivePlayers[i];
					pWiner = vActivePlayers[j];
				}
				if (pWiner && pLoser) {
					uint8_t nWinIdx = pWiner->getIdx();
					result_1[nWinIdx].push_back(pLoser->getIdx());
				}
			}
		}
	}

	// caculate coin
	// 先找全垒打
	uint8_t nQLDIdx = -1;
	for (auto& ref : result) {
		for (uint8_t i = 0; i < ref.size(); i++) {
			if (ref[i].size() < vActivePlayers.size() - 1) {
				if (i == nQLDIdx) {
					nQLDIdx = -1;
					break;
				}
				continue;
			}
			if ((uint8_t)-1 == nQLDIdx) {
				nQLDIdx = i;
			}
			else if (nQLDIdx == i) {
				continue;
			}
			else {
				nQLDIdx = -1;
				break;
			}
		}
		if ((uint8_t)-1 == nQLDIdx) {
			break;
		}
	}
	//找到全垒打算分
	if ((uint8_t)-1 != nQLDIdx) {
		auto pWinPlayer = getPlayerByIdx(nQLDIdx);
		uint32_t nWinCoin = 0;
		if (pWinPlayer) {
			int32_t nLoseCoin = getWinCoin(nQLDIdx, WIN_SHUI_TYPE_SWAT);
			for (auto& ref : vActivePlayers) {
				if (ref->getIdx() == nQLDIdx) {
					continue;
				}
				ref->addSingleOffset(-1 * nLoseCoin);
				nWinCoin += nLoseCoin;
			}
		}
		pWinPlayer->addSingleOffset(nWinCoin);
	}
	
	//再找打枪
	std::vector<std::vector<uint8_t>> vShoot;
	vShoot.resize(nSeatCnt);
	for (uint8_t i = DAO_HEAD; i < DAO_MAX; i++) {
		for (uint8_t j = 0; j < nSeatCnt; j++) {
			auto& temp = result[i][j];
			if (i == DAO_HEAD) {
				if (temp.empty()) {
					continue;
				}
				for (auto ref : temp) {
					vShoot[j].push_back(ref);
				}
			}
			else {
				if (vShoot[j].empty() | temp.empty()) {
					vShoot[j].clear();
					continue;
				}
				std::vector<uint8_t> v;
				std::sort(vShoot[j].begin(), vShoot[j].end());
				std::sort(temp.begin(), temp.end());
				std::set_intersection(vShoot[j].begin(), vShoot[j].end(), temp.begin(), temp.end(), back_inserter(v));
				if (v.empty()) {
					vShoot[j].clear();
					continue;
				}
				else {
					vShoot[j].clear();
					vShoot[j].assign(v.begin(), v.end());
				}
			}
		}
	}
	//找到打枪算分
	for (uint8_t i = 0; i < nSeatCnt; i++) {
		if (vShoot[i].empty()) {
			continue;
		}
		else {
			if (i != nQLDIdx) {
				auto pWinPlayer = getPlayerByIdx(nQLDIdx);
				if (pWinPlayer) {
					uint32_t nWinCoin = 0;
					int32_t nLoseCoin = getWinCoin(nQLDIdx, WIN_SHUI_TYPE_SHOOT);
					for (auto& loseIdx : vShoot[i]) {
						auto pLoser = getPlayerByIdx(loseIdx);
						if (pLoser) {
							pLoser->addSingleOffset(-1 * nLoseCoin);
							nWinCoin += nLoseCoin;
						}
					}
					pWinPlayer->addSingleOffset(nWinCoin);
				}
			}
		}
	}

	//最后算正常每道的输赢
	for (uint8_t nDao = DAO_HEAD; nDao < DAO_MAX; nDao++) {
		for (uint8_t nWinIdx = 0; nWinIdx < nSeatCnt; nWinIdx++) {
			for (auto& nLoseIdx : result[nDao][nWinIdx]) {
				if (nWinIdx == nQLDIdx ||
					(vShoot[nWinIdx].size() && std::find(vShoot[nWinIdx].begin(), vShoot[nWinIdx].end(), nLoseIdx) != vShoot[nWinIdx].end())) {
					continue;
				}
				auto pWiner = getPlayerByIdx(nWinIdx);
				if (pWiner) {
					auto pLoser = getPlayerByIdx(nLoseIdx);
					if (pLoser) {
						int32_t nWinCoin = getWinCoin(nQLDIdx, WIN_SHUI_TYPE_NONE, nDao);
						pLoser->addSingleOffset(-1 * nWinCoin);
						pWiner->addSingleOffset(nWinCoin);
					}
				}
			}
		}
	}

	// send game result msg ; 
	Json::Value jsArrayPlayers;
	for (auto& p : vActivePlayers)
	{
		Json::Value jsPlayerResult;
		auto nIdx = p->getIdx();
		jsPlayerResult["idx"] = nIdx;
		jsPlayerResult["offset"] = p->getSingleOffset();
		if (nQLDIdx == nIdx) {
			jsPlayerResult["swat"] = nIdx;
		}
		else if (vShoot[nIdx].size()) {
			Json::Value jsShoot;
			for (auto& ref : vShoot[nIdx]) {
				jsShoot[jsShoot.size()] = ref;
			}
			jsPlayerResult["shoot"] = jsShoot;
		}
		jsArrayPlayers[jsArrayPlayers.size()] = jsPlayerResult;
	}

	Json::Value jsMsg;
	jsMsg["players"] = jsArrayPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_GAME_END);
	GameRoom::onGameEnd();
}

bool ThirteenRoom::canStartGame()
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
			//return false;
			continue;
		}

		++nReadyCnt;
	}
	return nReadyCnt >= 2;
}

IPoker* ThirteenRoom::getPoker()
{
	return (IPoker*)&m_tPoker;
}

void ThirteenRoom::onPlayerReady(uint16_t nIdx)
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

void ThirteenRoom::doDistributeCard(uint8_t nCardCnt)
{
	Json::Value jsVecPlayers, jsVecReplay;
	auto nCnt = getSeatCnt();
	for ( auto nIdx = 0; nIdx < nCnt; ++nIdx )
	{
		auto pPlayer = (ThirteenPlayer*)getPlayerByIdx(nIdx);
		if (pPlayer == nullptr || (pPlayer->haveState(eRoomPeer_CanAct) == false))
		{
			continue;
		}
		auto playerCard = pPlayer->getPlayerCard();
		
		// distribute card ;
		for ( auto nCardIdx = 0; nCardIdx < nCardCnt; ++nCardIdx )
		{
			playerCard->addCompositCardNum(getPoker()->distributeOneCard());
		}

		// make disribute msg ;
		Json::Value jsPlayer, jsPlayerRePlay, jsCards;
		playerCard->holdCardToJson(jsCards);
		jsPlayer["idx"] = nIdx;
		jsPlayer["cards"] = jsCards;

		jsPlayerRePlay["idx"] = nIdx;
		jsPlayerRePlay["uid"] = pPlayer->getUserUID();
		jsPlayerRePlay["coin"] = pPlayer->getChips();
		
		jsPlayerRePlay["cards"] = jsCards;

		jsVecPlayers[jsVecPlayers.size()] = jsPlayer;
		jsVecReplay[jsVecReplay.size()] = jsPlayerRePlay;
	}

	Json::Value jsMsg, jsReplayInfo;
	jsMsg["info"] = jsVecPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_DISTRIBUTE_CARD );

	jsReplayInfo["players"] = jsVecReplay;
	addReplayFrame(eThirteenFrame_StartGame, jsReplayInfo);
}

std::shared_ptr<IPlayerRecorder> ThirteenRoom::createPlayerRecorderPtr()
{
	return std::make_shared<ThirteenPlayerRecorder>();
}

bool ThirteenRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
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

	return false;
}

bool ThirteenRoom::onWaitAct() {
	for (auto& ref : m_vPlayers) {
		if (ref && isPlayerCanAct(ref->getIdx())) {
			onWaitPlayerAct(ref->getIdx());
		}
	}
	return true;
}

bool ThirteenRoom::onWaitPlayerAct(uint8_t nIdx) {
	auto pPlayer = (ThirteenPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("player idx = %u is null can not tell it wait act", nIdx);
		return false;
	}

	if (pPlayer->hasDetermined()) {
		LOGFMTE("player idx = %u has alredy put cards", nIdx);
		return false;
	}

	Json::Value jsMsg;
	sendMsgToPlayer(jsMsg, MSG_ROOM_THIRTEEN_GAME_WAIT_ACT, pPlayer->getSessionID());

	Json::Value jsFrameArg;
	jsFrameArg["idx"] = nIdx;
	addReplayFrame(eThirteenFrame_WaitPlayerAct, jsFrameArg);
	return true;
}

bool ThirteenRoom::onAutoDoPlayerAct() {
	for (auto& ref : m_vPlayers) {
		if (ref && isPlayerCanAct(ref->getIdx())) {
			if (((ThirteenPlayer*)ref)->hasDetermined()) {
				continue;
			}
			auto pCard = ((ThirteenPlayer*)ref)->getPlayerCard();
			if (pCard->autoSetDao()) {
				((ThirteenPlayer*)ref)->signDetermined();
			}
			else {
				LOGFMTE("auto put cards error, roomID = %u, playerID = %u", getRoomID(), ref->getUserUID());
			}
		}
	}
}

bool ThirteenRoom::onPlayerSetDao(uint8_t nIdx, ThirteenPeerCard::VEC_CARD vCards) {
	if (vCards.size() != MAX_HOLD_CARD_COUNT) {
		return false;
	}
	auto pPlayer = (ThirteenPlayer*)getPlayerByIdx(nIdx);
	if (pPlayer && isPlayerCanAct(nIdx) && pPlayer->hasDetermined() == false) {
		ThirteenPeerCard::VEC_CARD vTemp;
		uint8_t nCurIdx = 0;
		auto pCard = pPlayer->getPlayerCard();
		for (uint8_t nDao = DAO_HEAD; nDao < DAO_MAX; nDao++) {
			vTemp.clear();
			vTemp.assign(vCards.begin() + (nDao > 0 ? 1 : 0) * HEAD_DAO_CARD_COUNT + (nDao > 0 ? nDao - 1 : 0) * OTHER_DAO_CARD_COUNT, vCards.begin() + HEAD_DAO_CARD_COUNT + OTHER_DAO_CARD_COUNT * nDao - 1);
			if (pCard->setDao(nDao, vTemp) == false) {
				LOGFMTE("put cards error, roomID = %u, playerID = %u, nDao = %u", getRoomID(), nIdx, nDao);
				return false;
			}
		}
		pPlayer->signDetermined();
		return true;
	}
	return false;
}

bool ThirteenRoom::isPlayerCanAct(uint8_t nIdx) {
	auto pPlayer = getPlayerByIdx(nIdx);
	return pPlayer && pPlayer->haveState(eRoomPeer_CanAct);
}

bool ThirteenRoom::isGameOver() {
	for (auto& ref : m_vPlayers) {
		if (ref && isPlayerCanAct(ref->getIdx())) {
			if (((ThirteenPlayer*)ref)->hasDetermined()) {
				continue;
			}
			return false;
		}
	}
	return true;
}

uint8_t ThirteenRoom::getBaseScore() {
	if (m_jsOpts["baseScore"].isNull()) {
		return 1;
	}
	else if (m_jsOpts["baseScore"].asUInt() > 100) {
		return 100;
	}
	else {
		return m_jsOpts["baseScore"].asUInt();
	}
}

uint8_t ThirteenRoom::getMultiple() {
	if (m_jsOpts["multiple"].isNull() || m_jsOpts["multiple"].asUInt() == 0) {
		return 0;
	}
	else if (m_jsOpts["multiple"].asUInt() < 4) {
		return 4;
	}
	else if (m_jsOpts["multiple"].asUInt() > 10) {
		return 0;
	}
	else {
		return m_jsOpts["multiple"].asUInt();
	}
}

uint8_t ThirteenRoom::getWinShui(uint8_t nIdx, uint8_t nWinType = WIN_SHUI_TYPE_NONE, uint8_t nDaoIdx = DAO_MAX) {
	auto pPlayer = (ThirteenPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer) {
		return 0;
	}
	
	auto pCard = pPlayer->getPlayerCard();
	uint32_t nShui = 0;
	switch (nWinType) {
	case WIN_SHUI_TYPE_NONE: {
		if (nDaoIdx < DAO_MAX) {
			nShui = getDaoWinShui(pCard->getType(nDaoIdx), nDaoIdx);
		}
		else if(DAO_MAX == nDaoIdx){
			for (uint8_t nDao = DAO_HEAD; nDao < DAO_MAX; nDao++) {
				nShui += getDaoWinShui(pCard->getType(nDao), nDao);
			}
		}
		break;
	}
	case WIN_SHUI_TYPE_SHOOT: {
		nShui += 6;
		for (uint8_t nDao = DAO_HEAD; nDao < DAO_MAX; nDao++) {
			uint8_t nTShui = getDaoWinShui(pCard->getType(nDao), nDao);
			nShui += nTShui > 1 ? nTShui : 0;
		}
		break;
	}
	case WIN_SHUI_TYPE_SWAT: {
		nShui += 13;
		for (uint8_t nDao = DAO_HEAD; nDao < DAO_MAX; nDao++) {
			uint8_t nTShui = getDaoWinShui(pCard->getType(nDao), nDao);
			nShui += nTShui > 1 ? nTShui : 0;
		}
		break;
	}
	}

	return nShui;
}

uint32_t ThirteenRoom::getWinCoin(uint8_t nIdx, uint8_t nWinType = WIN_SHUI_TYPE_NONE, uint8_t nDaoIdx = DAO_MAX) {
	return ((uint32_t)getWinShui(nIdx, nWinType, nDaoIdx)) * (uint32_t)getBaseScore();
}

uint8_t ThirteenRoom::getDaoWinShui(uint8_t nType, uint8_t nDaoIdx) {
	if (nDaoIdx >= DAO_MAX) {
		return 0;
	}
	uint8_t nShui = 0;
	switch (nDaoIdx) {
	case DAO_HEAD: {
		if (nType == Thirteen_ThreeCards) {
			nShui += 3;
		}
		else {
			nShui += 1;
		}
		break;
	}
	case DAO_MIDDLE: {
		if (nType == Thirteen_FuLu) {
			nShui += 2;
		}
		else if (nType == Thirteen_TieZhi) {
			nShui += 8;
		}
		else if (nType == Thirteen_StraightFlush) {
			nShui += 10;
		}
		else {
			nShui += 1;
		}
		break;
	}
	case DAO_TAIL: {
		if (nType == Thirteen_TieZhi) {
			nShui += 4;
		}
		else if (nType == Thirteen_StraightFlush) {
			nShui += 5;
		}
		else {
			nShui += 1;
		}
		break;
	}
	}
	return nShui;
}