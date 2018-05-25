#include "ThirteenRoom.h"
#include "ThirteenPlayer.h"
#include "ThirteenRoomStateWaitReady.h"
#include "ThirteenRoomStateStartGame.h"
#include "ThirteenRoomStateDistributeCard.h"
#include "ThirteenRoomStateWaitPlayerAct.h"
#include "ThirteenRoomStateGameEnd.h"
#include "ThirteenRoomStateRobBanker.h"
#include "iterator"
#include "ThirteenPlayerRecorder.h"
#include "IGameRoomDelegate.h"
#include "IGameRoomManager.h"
#include "ISeverApp.h"
#define BASE_DRAGIN_RATIO 100
#define GAME_AUTO_PICK_OUT_TIME 180
#define GAME_SHWO_CARDS_EXTRA_SHUI 10
#define GAME_ROT_BANKE_RATIO 2
#define GAME_WIN_SHOOT_EXTRA 6
#define GAME_WIN_SWAT_EXTRA 13
bool ThirteenRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	GameRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
	
	m_nBankerIdx = -1;
	m_bRotBanker = false;
	m_bShowCards = false;
	m_bIsWaiting = true;
	m_nMTTBaseScore = 0;

	IGameRoomState* pState = new ThirteenRoomStateWaitReady();
	addRoomState(pState);
	setInitState(pState);
	pState = new ThirteenRoomStateStartGame();
	addRoomState(pState);
	pState = new ThirteenRoomStateRobBanker();
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
	jsRoomInfo["bankIdx"] = m_nBankerIdx;
	jsRoomInfo["rotBanker"] = m_bRotBanker ? 1 : 0;
	jsRoomInfo["baseScore"] = getBaseScore();
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
 
	auto pCard = ((ThirteenPlayer*)pPlayer)->getPlayerCard();
	if (pPlayer->getSessionID() == nVisitorSessionID) {
		Json::Value jsHoldCards;
		if (((ThirteenPlayer*)pPlayer)->hasDetermined()) {
			pCard->groupCardToJson(jsHoldCards);
		}
		else {
			pCard->holdCardToJson(jsHoldCards);
		}
		
		jsPlayerInfo["holdCard"] = jsHoldCards;
		jsPlayerInfo["waitDrag"] = ((ThirteenPlayer*)pPlayer)->getWaitDragIn();
		/*if (((ThirteenPlayer*)pPlayer)->hasDetermined()) {
			Json::Value jsGroupCards;
			pCard->groupCardToJson(jsGroupCards);
			jsPlayerInfo["groupCard"] = jsGroupCards;
		}*/
	}
	else {
		if (((ThirteenPlayer*)pPlayer)->hasShowCards()) {
			Json::Value jsHoldCards;
			pCard->groupCardToJson(jsHoldCards);
			jsPlayerInfo["holdCard"] = jsHoldCards;
		}
	}

	jsPlayerInfo["determined"] = ((ThirteenPlayer*)pPlayer)->hasDetermined() ? 1 : 0;
	jsPlayerInfo["rotBanker"] = ((ThirteenPlayer*)pPlayer)->hasRotBanker() ? 1 : 0;
	jsPlayerInfo["showCards"] = ((ThirteenPlayer*)pPlayer)->hasShowCards() ? 1 : 0;
}

uint8_t ThirteenRoom::getRoomType()
{
	return eGame_Thirteen;
}

uint8_t ThirteenRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer) {
	return 0;
}

bool ThirteenRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer) {
	if (GameRoom::onPlayerEnter(pEnterRoomPlayer)) {
		getDelegate()->doPlayerEnter(this, pEnterRoomPlayer->nUserUID);
		return true;
	}
	return false;
}

void ThirteenRoom::onWillStartGame() {
	GameRoom::onWillStartGame();
	m_bRotBanker = false;
	m_bShowCards = false;
	if (isMTT() && getDelegate()) {
		m_nMTTBaseScore = getDelegate()->getBlindBaseScore();
	}
}

void ThirteenRoom::onStartGame()
{
	GameRoom::onStartGame();

	if (isMTT() && getDelegate()) {
		for (auto ref : m_vPlayers) {
			if (ref && ref->haveState(eRoomPeer_StayThisRound)) {
				auto nCoin = getDelegate()->getBlindPreScore();
				ref->addSingleOffset(-1 * (int32_t)nCoin, false);
				getDelegate()->onMTTPlayerCostPreScore(ref);
				Json::Value jsMsg;
				jsMsg["idx"] = ref->getIdx();
				jsMsg["chips"] = ref->getChips();
				sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_GOLDEN_UPDATE);
			}
		}
	}
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
	auto pPlayer = getPlayerByIdx(getBankerIdx());
	uint32_t nBankerUID = 0;
	if (pPlayer) {
		nBankerUID = pPlayer->getUserUID();
	}
	getCurRoundRecorder()->setBankerUID(nBankerUID);
	if (hasRotBanker()) {
		getCurRoundRecorder()->signRotBanker();
	}
	
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
		result.push_back(result_1);
	}

	// caculate coin
	bool bMinus = isClubRoom() == 0;
	if (isMTT()) {
		bMinus = true;
	}
	// 先找全垒打
	// 两个人没有全垒打
	uint8_t nQLDIdx = -1;
	if (vActivePlayers.size() > 2) {
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
	}
	
	//找到全垒打算分
	if ((uint8_t)-1 != nQLDIdx) {
		auto pWinPlayer = (ThirteenPlayer*)getPlayerByIdx(nQLDIdx);
		uint32_t nWinCoin = 0;
		if (pWinPlayer) {
			int32_t nLoseCoin = getWinCoin(nQLDIdx, WIN_SHUI_TYPE_SWAT);
			for (auto& ref : vActivePlayers) {
				auto nLoseTemp = nLoseCoin;
				if (ref->getIdx() == nQLDIdx) {
					continue;
				}
				if (pWinPlayer->hasShowCards() || ((ThirteenPlayer*)ref)->hasShowCards()) {
					nLoseTemp += GAME_SHWO_CARDS_EXTRA_SHUI * getBaseScore();
				}
				if (hasRotBanker() &&
					(getBankerIdx() == pWinPlayer->getIdx() ||
						getBankerIdx() == ref->getIdx())) {
					nLoseTemp *= GAME_ROT_BANKE_RATIO;
				}
				nLoseTemp = ref->addSingleOffset(-1 * nLoseTemp, bMinus);
				nWinCoin += nLoseTemp;
			}
		}
		pWinPlayer->addSingleOffset(nWinCoin, bMinus);
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
				if (vShoot[j].empty() || temp.empty()) {
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
		if (vShoot[i].empty() || i == nQLDIdx) {
			continue;
		}
		else {
			if (i != nQLDIdx) {
				auto pWinPlayer = (ThirteenPlayer*)getPlayerByIdx(i);
				if (pWinPlayer) {
					uint32_t nWinCoin = 0;
					int32_t nLoseCoin = getWinCoin(i, WIN_SHUI_TYPE_SHOOT);
					for (auto& loseIdx : vShoot[i]) {
						auto nLoseTemp = nLoseCoin;
						auto pLoser = (ThirteenPlayer*)getPlayerByIdx(loseIdx);
						if (pLoser) {
							if (pWinPlayer->hasShowCards() || pLoser->hasShowCards()) {
								nLoseTemp += GAME_SHWO_CARDS_EXTRA_SHUI * getBaseScore();
							}
							if (hasRotBanker() &&
								(getBankerIdx() == pWinPlayer->getIdx() ||
									getBankerIdx() == pLoser->getIdx())) {
								nLoseTemp *= GAME_ROT_BANKE_RATIO;
							}

							nLoseTemp = pLoser->addSingleOffset(-1 * nLoseTemp, bMinus);
							nWinCoin += nLoseTemp;
						}
					}
					pWinPlayer->addSingleOffset(nWinCoin, bMinus);
				}
			}
		}
	}

	//最后算正常每道的输赢
	std::map<uint8_t, std::vector<uint8_t>> vShowCards;
	for (uint8_t nDao = DAO_HEAD; nDao < DAO_MAX; nDao++) {
		for (uint8_t nWinIdx = 0; nWinIdx < nSeatCnt; nWinIdx++) {
			for (auto& nLoseIdx : result[nDao][nWinIdx]) {
				if (nWinIdx == nQLDIdx ||
					(vShoot[nWinIdx].size() &&
						std::find(vShoot[nWinIdx].begin(), vShoot[nWinIdx].end(), nLoseIdx) != vShoot[nWinIdx].end())) {
					continue;
				}
				auto pWiner = (ThirteenPlayer*)getPlayerByIdx(nWinIdx);
				if (pWiner) {
					auto pLoser = (ThirteenPlayer*)getPlayerByIdx(nLoseIdx);
					if (pLoser) {
						int32_t nWinCoin = 0;
						if (pWiner->hasShowCards()) {
							std::vector<uint8_t> vShowTemp;
							if (vShowCards.count(nWinIdx)) {
								vShowTemp.assign(vShowCards[nWinIdx].begin(), vShowCards[nWinIdx].end());
							}
							else {
								vShowCards[nWinIdx] = vShowTemp;
							}
							if (std::find(vShowTemp.begin(), vShowTemp.end(), nLoseIdx) != vShowTemp.end()) {
								continue;
							}
							vShowCards[nWinIdx].push_back(nLoseIdx);
							auto pTemp = pWiner;
							pWiner = pLoser;
							pLoser = pTemp;
							nWinCoin = getWinCoin(nLoseIdx, WIN_SHUI_TYPE_SHOWCARDS);
						}
						else if (pLoser->hasShowCards()) {
							continue;
						}
						else {
							nWinCoin = getWinCoin(nWinIdx, WIN_SHUI_TYPE_NONE, nDao);
						}

						/*if (pWiner->hasShowCards() || ((ThirteenPlayer*)pLoser)->hasShowCards()) {
							nWinCoin += GAME_SHWO_CARDS_EXTRA_SHUI * getBaseScore();
						}*/
						if (hasRotBanker() &&
							(getBankerIdx() == pWiner->getIdx() ||
								getBankerIdx() == pLoser->getIdx())) {
							nWinCoin *= GAME_ROT_BANKE_RATIO;
						}

						nWinCoin = pLoser->addSingleOffset(-1 * nWinCoin, bMinus);
						pWiner->addSingleOffset(nWinCoin, bMinus);
					}
				}
			}
		}
	}

	// send game result msg ;
	Json::Value jsArrayPlayers;
	for (auto& p : vActivePlayers)
	{
		Json::Value jsPlayerResult, jsCards, jsCardsType, jsCardsWeight, jsDaoResult;
		auto nIdx = p->getIdx();
		jsPlayerResult["idx"] = nIdx;
		jsPlayerResult["offset"] = p->getSingleOffset();
		jsPlayerResult["final"] = p->getChips();
		p->getPlayerCard()->groupCardToJson(jsCards);
		jsPlayerResult["cards"] = jsCards;
		p->getPlayerCard()->groupCardTypeToJson(jsCardsType);
		p->getPlayerCard()->groupCardWeightToJson(jsCardsWeight);
		jsPlayerResult["cardsType"] = jsCardsType;
		jsPlayerResult["cardsWeight"] = jsCardsWeight;

		for (uint8_t ref_dao = 0; ref_dao < result.size(); ref_dao++) {
			int32_t nResult = 0;
			for (uint8_t ref_i = 0; ref_i < result[ref_dao].size(); ref_i++) {
				for (auto& ref_j : result[ref_dao][ref_i]) {
					if (nIdx == ref_i) {
						nResult += getWinCoin(ref_i, WIN_SHUI_TYPE_NONE, ref_dao);
					}
					else if (nIdx == ref_j) {
						nResult -= getWinCoin(ref_i, WIN_SHUI_TYPE_NONE, ref_dao);
					}
				}
			}
			jsDaoResult[jsDaoResult.size()] = nResult;
		}
		jsPlayerResult["daoOffset"] = jsDaoResult;

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
	jsMsg["result"] = jsArrayPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_GAME_END);

	for (auto& ref : vActivePlayers) {
		Json::Value jsMsg;
		jsMsg["targetUID"] = ref->getUserUID();
		if (ref->getSingleOffset() > 0) {
			jsMsg["win"] = 1;
		}
		else {
			jsMsg["win"] = 0;
		}
		getRoomMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, ref->getUserUID(), eAsync_player_game_add, jsMsg);
	}

	GameRoom::onGameEnd();
}

void ThirteenRoom::onGameDidEnd() {
	getDelegate()->onPreGameDidEnd(this);

	if (getDelegate() && (isClubRoom() || isMTT()) && isRoomGameOver() == false) {
		for (auto& ref : m_vPlayers) {
			if (ref && ref->getChips() < getDragInNeed() && ref->haveState(eRoomPeer_WaitDragIn) == false) {
				ref->setState(eRoomPeer_WaitDragIn);
				getDelegate()->onPlayerWaitDragIn(ref->getUserUID());
				if (getDelegate()->canPlayerDragIn(ref->getUserUID()) == 0) {
					Json::Value jsMsg;
					jsMsg["ret"] = 0;
					jsMsg["idx"] = ref->getIdx();
					jsMsg["min"] = getMinDragIn();
					jsMsg["max"] = getMaxDragIn();
					jsMsg["enterClubID"] = getDelegate()->getEnterClubID(ref->getUserUID());
					if (isMTT()) {
						jsMsg["dragIn"] = 1;
					}
					if (isLeagueRoom()) {
						if (getDelegate() && getDelegate()->getDragInClubID(ref->getUserUID())) {
							Json::Value jsClubs;
							jsClubs[jsClubs.size()] = getDelegate()->getDragInClubID(ref->getUserUID());
							jsMsg["clubIDs"] = jsClubs;
							sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_NEED_DRAGIN);
							//return;
						}
						else {
							Json::Value jsReq;
							jsReq["playerUID"] = ref->getUserUID();
							jsReq["leagueID"] = isLeagueRoom();
							getRoomMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, ref->getUserUID(), eAsync_player_apply_DragIn_Clubs, jsReq, [this, ref](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
							{
								if (isTimeOut)
								{
									LOGFMTE("inform player drag in clubs error,time out  room id = %u , uid = %u", getRoomID(), ref->getUserUID());
									Json::Value jsClubs;
									jsClubs[jsClubs.size()] = isClubRoom();
									jsUserData["clubIDs"] = jsClubs;
									sendRoomMsg(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN);
									return;
								}

								if (retContent["ret"].asUInt() != 0)
								{
									LOGFMTE("inform player drag in clubs error, request error, room id = %u , uid = %u", getRoomID(), ref->getUserUID());
									Json::Value jsClubs;
									jsClubs[jsClubs.size()] = isClubRoom();
									jsUserData["clubIDs"] = jsClubs;
									sendRoomMsg(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN);
									return;
								}
								Json::Value jsClubs;
								jsClubs[jsClubs.size()] = isClubRoom();
								jsUserData["clubIDs"] = retContent["clubIDs"];
								sendRoomMsg(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN);
							}, jsMsg);
						}
					}
					else {
						Json::Value jsClubs;
						jsClubs[jsClubs.size()] = isClubRoom();
						jsMsg["clubIDs"] = jsClubs;
						sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_NEED_DRAGIN);
					}
				}
				else if (isMTT()) {
					doPlayerStandUp(ref->getUserUID());
				}
			}
		}
	}

	GameRoom::onGameDidEnd();
}

bool ThirteenRoom::isRoomGameOver() {
	return getDelegate()->isRoomGameOver();
}

bool ThirteenRoom::canStartGame()
{
	return GameRoom::canStartGame();
	//if ( false == GameRoom::canStartGame())
	//{
	//	return false;
	//}

	//uint16_t nReadyCnt = 0;
	//for (uint16_t nIdx = 0; nIdx < getSeatCnt(); ++nIdx)
	//{
	//	auto p = getPlayerByIdx(nIdx);
	//	if ( nullptr == p )
	//	{
	//		continue;
	//	}

	//	if ( p->haveState(eRoomPeer_Ready) == false && p->haveState(eRoomPeer_WaitDragIn) == false)
	//	{
	//		return false;
	//		//continue;
	//	}

	//	if (p->haveState(eRoomPeer_WaitDragIn)) {
	//		continue;
	//	}

	//	++nReadyCnt;
	//}
	//return nReadyCnt >= getOpenCnt();
}

IPoker* ThirteenRoom::getPoker()
{
	return (IPoker*)&m_tPoker;
}

bool ThirteenRoom::canPlayerSitDown(stEnterRoomData* pEnterRoomPlayer, uint16_t nIdx) {
	if (isClubRoom()) {
		if (pEnterRoomPlayer->nChip < getMinDragIn()) {
			if (getDelegate()) {
				return getDelegate()->canPlayerSitDown(pEnterRoomPlayer->nUserUID);
			}
			else {
				return false;
			}
		}
	}
	return true;
}

bool ThirteenRoom::doPlayerSitDown(stEnterRoomData* pEnterRoomPlayer, uint16_t nIdx) {
	if (GameRoom::doPlayerSitDown(pEnterRoomPlayer, nIdx)) {
		if (isClubRoom() || isMTT()) {
			auto pPlayer = getPlayerByIdx(nIdx);
			if (pPlayer->getChips() < getDragInNeed()) {
				pPlayer->setState(eRoomPeer_WaitDragIn);
				getDelegate()->onPlayerWaitDragIn(pPlayer->getUserUID());
				if (getDelegate() && getDelegate()->canPlayerDragIn(pPlayer->getUserUID()) == 0) {
					Json::Value jsMsg;
					jsMsg["ret"] = 0;
					jsMsg["idx"] = nIdx;
					jsMsg["min"] = getMinDragIn();
					jsMsg["max"] = getMaxDragIn();
					jsMsg["enterClubID"] = getDelegate()->getEnterClubID(pPlayer->getUserUID());
					if (isMTT()) {
						jsMsg["dragIn"] = 1;
					}
					if (isLeagueRoom()) {
						if (getDelegate() && getDelegate()->getDragInClubID(pPlayer->getUserUID())) {
							Json::Value jsClubs;
							jsClubs[jsClubs.size()] = getDelegate()->getDragInClubID(pPlayer->getUserUID());
							jsMsg["clubIDs"] = jsClubs;
							sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_NEED_DRAGIN);
							return true;
						}
						Json::Value jsReq;
						jsReq["playerUID"] = pPlayer->getUserUID();
						jsReq["leagueID"] = isLeagueRoom();
						getRoomMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_player_apply_DragIn_Clubs, jsReq, [this, pPlayer](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
						{
							if (isTimeOut)
							{
								LOGFMTE("inform player drag in clubs error,time out  room id = %u , uid = %u", getRoomID(), pPlayer->getUserUID());
								Json::Value jsClubs;
								jsClubs[jsClubs.size()] = isClubRoom();
								jsUserData["clubIDs"] = jsClubs;
								sendRoomMsg(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN);
								return;
							}

							if (retContent["ret"].asUInt() != 0)
							{
								LOGFMTE("inform player drag in clubs error, request error, room id = %u , uid = %u", getRoomID(), pPlayer->getUserUID());
								Json::Value jsClubs;
								jsClubs[jsClubs.size()] = isClubRoom();
								jsUserData["clubIDs"] = jsClubs;
								sendRoomMsg(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN);
								return;
							}
							Json::Value jsClubs;
							jsClubs[jsClubs.size()] = isClubRoom();
							jsUserData["clubIDs"] = retContent["clubIDs"];
							sendRoomMsg(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN);
						}, jsMsg);
					}
					else {
						Json::Value jsClubs;
						jsClubs[jsClubs.size()] = isClubRoom();
						jsMsg["clubIDs"] = jsClubs;
						sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_NEED_DRAGIN);
					}
				}
				else if (isMTT()) {
					doPlayerStandUp(pPlayer->getUserUID());
				}
			}
		}
		
		return true;
	}
	return false;
}

std::shared_ptr<IGameRoomRecorder> ThirteenRoom::getRoomRecorder() {
	if (getDelegate() && getDelegate()->getRoomRecorder()) {
		return getDelegate()->getRoomRecorder();
	}
	return GameRoom::getRoomRecorder();
}

bool ThirteenRoom::doDeleteRoom() {
	// process player leave ;
	std::vector<uint32_t> vAllInRoomPlayers;
	for (auto& pPayer : m_vPlayers)
	{
		if (pPayer == nullptr)
		{
			continue;
		}
		vAllInRoomPlayers.push_back(pPayer->getUserUID());
	}

	for (auto& pStand : m_vStandPlayers)
	{
		vAllInRoomPlayers.push_back(pStand.second->nUserUID);
	}

	for (auto& ref : vAllInRoomPlayers)
	{
		doPlayerLeaveRoom(ref);
	}
	LOGFMTD("room id = %u do delete", getRoomID());
	return true;
}

void ThirteenRoom::update(float fDelta) {
	GameRoom::update(fDelta);
	for (auto& ref : m_vPlayers) {
		if (ref && ref->getState() == eRoomPeer_WaitDragIn) {
			((ThirteenPlayer*)ref)->addWaitDragInTime(fDelta);
			if (((ThirteenPlayer*)ref)->getWaitDragInTime() > GAME_AUTO_PICK_OUT_TIME) {
				doPlayerStandUp(ref->getUserUID());
			}
		}
	}
}

bool ThirteenRoom::doPlayerLeaveRoom(uint32_t nUserUID) {
	auto pPlayer = getPlayerByUID(nUserUID);
	if (pPlayer)
	{
		doPlayerStandUp(nUserUID);
	}

	auto iterStand = m_vStandPlayers.find(nUserUID);
	if (m_vStandPlayers.end() == iterStand)
	{
		LOGFMTE("uid = %u not stand in this room = %u how to leave ?", nUserUID, getRoomID());
		return false;
	}

	delete iterStand->second;
	m_vStandPlayers.erase(iterStand);
	if (getDelegate())
	{
		getDelegate()->onPlayerDoLeaved(this, nUserUID);
	}

	return true;
}

bool ThirteenRoom::doPlayerTempLeave(uint32_t nUserUID) {
	auto pPlayer = getPlayerByUID(nUserUID);
	if (pPlayer)
	{
		doPlayerStandUp(nUserUID);
	}

	auto iterStand = m_vStandPlayers.find(nUserUID);
	if (m_vStandPlayers.end() == iterStand)
	{
		LOGFMTE("uid = %u not stand in this room = %u how to leave ?", nUserUID, getRoomID());
		return false;
	}

	stEnterRoomData pEnterRoomPlayer;
	pEnterRoomPlayer.nChip = 0;
	pEnterRoomPlayer.nDiamond = 0;
	pEnterRoomPlayer.nCurInRoomID = 0;
	pEnterRoomPlayer.nUserUID = nUserUID;
	pEnterRoomPlayer.nSessionID = iterStand->second->nSessionID;

	delete iterStand->second;
	m_vStandPlayers.erase(iterStand);
	if (getDelegate())
	{
		getDelegate()->onPlayerTempLeaved(this, &pEnterRoomPlayer);
	}

	return true;
}

bool ThirteenRoom::doAllPlayerStandUp() {
	// process player leave ;
	std::vector<uint32_t> vAllInRoomPlayers;
	for (auto& pPayer : m_vPlayers)
	{
		if (pPayer == nullptr)
		{
			continue;
		}
		vAllInRoomPlayers.push_back(pPayer->getUserUID());
	}

	for (auto& ref : vAllInRoomPlayers)
	{
		doPlayerStandUp(ref);
	}

	LOGFMTD("room id = %u do all player stand up", getRoomID());
	return true;
}

bool ThirteenRoom::clearRoom() {
	// process player leave ;
	std::vector<uint32_t> vAllInRoomPlayers;
	for (auto& pPayer : m_vPlayers)
	{
		if (pPayer == nullptr)
		{
			continue;
		}
		vAllInRoomPlayers.push_back(pPayer->getUserUID());
	}

	for (auto& pStand : m_vStandPlayers)
	{
		vAllInRoomPlayers.push_back(pStand.second->nUserUID);
	}

	for (auto& ref : vAllInRoomPlayers)
	{
		/*if (getDelegate() && isRoomGameOver()) {
			getDelegate()->onPlayerAutoLeave(ref);
		}*/
		if (isMTT()) {
			doPlayerTempLeave(ref);
		}
		else {
			doPlayerLeaveRoom(ref);
		}
		
	}
	LOGFMTD("room id = %u do clear", getRoomID());
	return true;
}

void ThirteenRoom::onPlayerReady(uint16_t nIdx)
{
	auto pPlayer = getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("idx = %u target player is null ptr can not set ready", nIdx);
		return;
	}
	if (pPlayer->haveState(eRoomPeer_WaitNextGame) == false)
	{
		//LOGFMTE("player state error uid = %u , state = %u", pPlayer->getUserUID(), pPlayer->getState());
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
	doProduceNewBanker();

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
	jsMsg["baseScore"] = getBaseScore();
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

	if (MSG_CLUB_ROOM_T_PLAYER_CHECK == nMsgType) {
		Json::Value jsRet;
		uint32_t nIdx = prealMsg["idx"].asUInt();
		jsRet["idx"] = nIdx;
		auto tPlayer = getPlayerByIdx(nIdx);
		uint32_t nUserID = getPlayerBySessionID(nSessionID) == nullptr ? 0 : getPlayerBySessionID(nSessionID)->getUserUID();
		if (nUserID == 0) {
			nUserID = getStandPlayerBySessionID(nSessionID) == nullptr ? 0 : getStandPlayerBySessionID(nSessionID)->nUserUID;
		}
		if (nUserID && tPlayer && getDelegate()) {
			uint32_t nClubID = getDelegate()->getDragInClubID(tPlayer->getUserUID());
			if (nClubID) {
				Json::Value jsMsg;
				jsMsg["uid"] = nUserID;
				jsMsg["tuid"] = tPlayer->getUserUID();
				jsMsg["clubID"] = nClubID;
				jsMsg["leagueID"] = isLeagueRoom();
				getRoomMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nClubID, eAsync_Club_T_Player_Check, jsMsg, [this, nUserID, nSessionID, nMsgType, jsRet](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
				{
					Json::Value jsResult;
					jsResult = jsRet;
					if (isTimeOut)
					{
						LOGFMTE("inform player T other in club error,time out room id = %u , uid = %u", getRoomID(), nUserID);
						jsResult["ret"] = 7;
						sendMsgToPlayer(jsResult, nMsgType, nSessionID);
						return;
					}

					if (retContent["ret"].asUInt() != 0)
					{
						LOGFMTE("inform player T other in club error, request error, room id = %u , uid = %u", getRoomID(), nUserID);
						jsResult["ret"] = 3;
						sendMsgToPlayer(jsResult, nMsgType, nSessionID);
						return;
					}
					jsResult["ret"] = 0;
					sendMsgToPlayer(jsResult, nMsgType, nSessionID);
				});
			}
			else {
				jsRet["ret"] = 2;
				sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			}
		}
		else {
			jsRet["ret"] = 1;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
		}
		return true;
	}

	if (MSG_CLUB_ROOM_T_PLAYER == nMsgType) {
		Json::Value jsRet;
		uint32_t nIdx = prealMsg["idx"].asUInt();
		jsRet["idx"] = nIdx;
		auto tPlayer = getPlayerByIdx(nIdx);
		uint32_t nUserID = getPlayerBySessionID(nSessionID) == nullptr ? 0 : getPlayerBySessionID(nSessionID)->getUserUID();
		if (nUserID == 0) {
			nUserID = getStandPlayerBySessionID(nSessionID) == nullptr ? 0 : getStandPlayerBySessionID(nSessionID)->nUserUID;
		}
		if (nUserID && tPlayer && getDelegate()) {
			uint32_t nClubID = getDelegate()->getDragInClubID(tPlayer->getUserUID());
			if (nClubID) {
				Json::Value jsMsg;
				jsMsg["uid"] = nUserID;
				jsMsg["tuid"] = tPlayer->getUserUID();
				jsMsg["clubID"] = nClubID;
				jsMsg["leagueID"] = isLeagueRoom();
				getRoomMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nClubID, eAsync_Club_T_Player_Check, jsMsg, [this, nUserID, nSessionID, nMsgType, jsRet, tPlayer](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
				{
					Json::Value jsResult;
					jsResult = jsRet;
					if (isTimeOut)
					{
						LOGFMTE("inform player T other in club error,time out room id = %u , uid = %u", getRoomID(), nUserID);
						jsResult["ret"] = 7;
						sendMsgToPlayer(jsResult, nMsgType, nSessionID);
						return;
					}

					if (retContent["ret"].asUInt() != 0)
					{
						LOGFMTE("inform player T other in club error, request error, room id = %u , uid = %u", getRoomID(), nUserID);
						jsResult["ret"] = 3;
						sendMsgToPlayer(jsResult, nMsgType, nSessionID);
						return;
					}
					
					Json::Value jsLeave;
					sendMsgToPlayer(jsLeave, MSG_ROOM_PLAYER_LEAVE, tPlayer->getSessionID());
					if (getDelegate()) {
						getDelegate()->onPlayerTOut(tPlayer->getUserUID());
					}
					doPlayerLeaveRoom(tPlayer->getUserUID());
					jsResult["ret"] = 0;
					sendMsgToPlayer(jsResult, nMsgType, nSessionID);
				});
			}
			else {
				jsRet["ret"] = 2;
				sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			}
		}
		else {
			jsRet["ret"] = 1;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
		}
		return true;
	}

	if (MSG_CLUB_ROOM_T_STAND_PLAYER == nMsgType) {
		Json::Value jsRet;
		uint32_t nIdx = prealMsg["idx"].asUInt();
		jsRet["idx"] = nIdx;
		auto tPlayer = getPlayerByIdx(nIdx);
		uint32_t nUserID = getPlayerBySessionID(nSessionID) == nullptr ? 0 : getPlayerBySessionID(nSessionID)->getUserUID();
		if (nUserID == 0) {
			nUserID = getStandPlayerBySessionID(nSessionID) == nullptr ? 0 : getStandPlayerBySessionID(nSessionID)->nUserUID;
		}
		if (nUserID && tPlayer && getDelegate()) {
			uint32_t nClubID = getDelegate()->getDragInClubID(tPlayer->getUserUID());
			if (nClubID) {
				Json::Value jsMsg;
				jsMsg["uid"] = nUserID;
				jsMsg["tuid"] = tPlayer->getUserUID();
				jsMsg["clubID"] = nClubID;
				jsMsg["leagueID"] = isLeagueRoom();
				getRoomMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nClubID, eAsync_Club_T_Player_Check, jsMsg, [this, nUserID, nSessionID, nMsgType, jsRet, tPlayer](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
				{
					Json::Value jsResult;
					jsResult = jsRet;
					if (isTimeOut)
					{
						LOGFMTE("inform player T other in club error,time out room id = %u , uid = %u", getRoomID(), nUserID);
						jsResult["ret"] = 7;
						sendMsgToPlayer(jsResult, nMsgType, nSessionID);
						return;
					}

					if (retContent["ret"].asUInt() != 0)
					{
						LOGFMTE("inform player T other in club error, request error, room id = %u , uid = %u", getRoomID(), nUserID);
						jsResult["ret"] = 3;
						sendMsgToPlayer(jsResult, nMsgType, nSessionID);
						return;
					}

					doPlayerStandUp(tPlayer->getUserUID());
					jsResult["ret"] = 0;
					sendMsgToPlayer(jsResult, nMsgType, nSessionID);
				});
			}
			else {
				jsRet["ret"] = 2;
				sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			}
		}
		else {
			jsRet["ret"] = 1;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
		}
		return true;
	}

	if (MSG_ROOM_THIRTEEN_CANCEL_DRAGIN == nMsgType) {
		auto pPlayer = (ThirteenPlayer*)getPlayerBySessionID(nSessionID);
		if (pPlayer == nullptr)
		{
			return true;
		}

		if (pPlayer->haveState(eRoomPeer_WaitDragIn)) {
			doPlayerStandUp(pPlayer->getUserUID());
		}
		return true;
	}

	if (MSG_ROOM_THIRTEEN_PLAYER_AUTO_STANDUP == nMsgType) {
		auto pPlayer = (ThirteenPlayer*)getPlayerBySessionID(nSessionID);
		if (pPlayer == nullptr)
		{
			Json::Value jsRet;
			jsRet["ret"] = 1;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("you are not in this room how to apply auto stand up? session id = %u", nSessionID);
			return true;
		}

		if (prealMsg["state"].isNull() || prealMsg["state"].isUInt() == false) {
			Json::Value jsRet;
			jsRet["ret"] = 2;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("you are not in this room how to apply auto stand up? session id = %u", nSessionID);
			return true;
		}
		auto nState = prealMsg["state"].asUInt();
		if (nState) {
			if (isMTT()) {
				Json::Value jsMsg;
				sendMsgToPlayer(jsMsg, MSG_ROOM_PLAYER_LEAVE, nSessionID);
				if (getDelegate()) {
					getDelegate()->onPlayerDoLeaved(this, pPlayer->getUserUID());
				}
				return true;
			}
			pPlayer->signAutoStandUp();
		}
		else {
			pPlayer->clearAutoStandUp();
		}

		Json::Value jsRet;
		jsRet["ret"] = 0;
		jsRet["state"] = nState;
		sendMsgToPlayer(jsRet, nMsgType, nSessionID);

		if (getDelegate()) {
			getDelegate()->onPlayerAutoStandUp(pPlayer->getUserUID(), pPlayer->isAutoStandUp());
		}

		if (pPlayer->isAutoStandUp()) {
			if (pPlayer->haveState(eRoomPeer_StayThisRound) == false) {
				doPlayerStandUp(pPlayer->getUserUID());
			}
		}
		//LOGFMTE("you are not in this room how to apply auto stand up? session id = %u", nSessionID);
		return true;
	}
	if (MSG_ROOM_THIRTEEN_PLAYER_AUTO_LEAVE == nMsgType) {
		auto pPlayer = (ThirteenPlayer*)getPlayerBySessionID(nSessionID);
		if (pPlayer == nullptr)
		{
			for (auto ref : m_vStandPlayers) {
				if (ref.second->nSessionID == nSessionID) {
					doPlayerLeaveRoom(ref.second->nUserUID);
					Json::Value jsMsg;
					sendMsgToPlayer(jsMsg, MSG_ROOM_PLAYER_LEAVE, nSessionID);
					return true;
				}
			}
			Json::Value jsRet;
			jsRet["ret"] = 1;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("you are not in this room how to apply auto stand up? session id = %u", nSessionID);
			return true;
		}

		if (prealMsg["state"].isNull() || prealMsg["state"].isUInt() == false) {
			Json::Value jsRet;
			jsRet["ret"] = 2;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("you are not in this room how to apply auto stand up? session id = %u", nSessionID);
			return true;
		}
		auto nState = prealMsg["state"].asUInt();
		if (nState) {
			if (isMTT()) {
				Json::Value jsMsg;
				sendMsgToPlayer(jsMsg, MSG_ROOM_PLAYER_LEAVE, nSessionID);
				if (getDelegate()) {
					getDelegate()->onPlayerDoLeaved(this, pPlayer->getUserUID());
				}
				return true;
			}
			pPlayer->signAutoLeave();
		}
		else {
			pPlayer->clearAutoLeave();
		}

		Json::Value jsRet;
		jsRet["ret"] = 0;
		jsRet["state"] = nState;
		sendMsgToPlayer(jsRet, nMsgType, nSessionID);

		if (getDelegate()) {
			getDelegate()->onPlayerAutoLeave(pPlayer->getUserUID(), pPlayer->isAutoLeave());
		}

		if (pPlayer->isAutoLeave()) {
			if (pPlayer->haveState(eRoomPeer_StayThisRound) == false) {
				doPlayerLeaveRoom(pPlayer->getUserUID());
				Json::Value jsMsg;
				sendMsgToPlayer(jsMsg, MSG_ROOM_PLAYER_LEAVE, nSessionID);
			}
		}
		
		//LOGFMTE("you are not in this room how to apply auto stand up? session id = %u", nSessionID);
		return true;
	}
	if (MSG_ROOM_THIRTEEN_STAND_PLAYERS == nMsgType) {
		uint32_t tIdx = 0;
		uint32_t pIdx = 0;
		std::vector<stStandPlayer*> vsPlayers;
		vsPlayers.clear();
		for (auto ref : m_vStandPlayers) {
			if (ref.second) {
				vsPlayers.push_back(ref.second);
			}
		}
		while (tIdx < vsPlayers.size()) {
			Json::Value jsMsg, jsPlayers;
			jsMsg["idx"] = pIdx;
			for (; tIdx < tIdx + 20; tIdx++) {
				if (tIdx >= vsPlayers.size()) {
					break;
				}
				auto st = vsPlayers.at(tIdx);
				jsPlayers[jsPlayers.size()] = st->nUserUID;
			}
			jsMsg["players"] = jsPlayers;
			pIdx++;
			sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
		}
		return true;
	}

	if (MSG_ROOM_THIRTEEN_NEED_DRAGIN == nMsgType) {
		if (isClubRoom() == 0) {
			Json::Value jsRet;
			jsRet["ret"] = 1;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("current room do not need to drag in? session id = %u", nSessionID);
			return true;
		}

		auto pPlayer = getPlayerBySessionID(nSessionID);
		if (pPlayer == nullptr)
		{
			Json::Value jsRet;
			jsRet["ret"] = 2;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("you are not in this room how to apply drag in? session id = %u", nSessionID);
			return true;
		}

		Json::Value jsMsg;
		jsMsg["ret"] = 0;
		jsMsg["idx"] = pPlayer->getIdx();
		jsMsg["min"] = getMinDragIn();
		jsMsg["max"] = getMaxDragIn();
		jsMsg["enterClubID"] = getDelegate()->getEnterClubID(pPlayer->getUserUID());
		if (isMTT()) {
			jsMsg["dragIn"] = 1;
		}
		if (isLeagueRoom()) {
			if (getDelegate() && getDelegate()->getDragInClubID(pPlayer->getUserUID())) {
				Json::Value jsClubs;
				jsClubs[jsClubs.size()] = getDelegate()->getDragInClubID(pPlayer->getUserUID());
				jsMsg["clubIDs"] = jsClubs;
				sendMsgToPlayer(jsMsg, MSG_ROOM_THIRTEEN_NEED_DRAGIN, pPlayer->getSessionID());
				return true;
			}
			Json::Value jsReq;
			jsReq["playerUID"] = pPlayer->getUserUID();
			jsReq["leagueID"] = isLeagueRoom();
			getRoomMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_player_apply_DragIn_Clubs, jsReq, [this, pPlayer](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
			{
				if (isTimeOut)
				{
					LOGFMTE("inform player drag in clubs error,time out  room id = %u , uid = %u", getRoomID(), pPlayer->getUserUID());
					Json::Value jsClubs;
					jsClubs[jsClubs.size()] = isClubRoom();
					jsUserData["clubIDs"] = jsClubs;
					sendMsgToPlayer(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN, pPlayer->getSessionID());
					return;
				}

				if (retContent["ret"].asUInt() != 0)
				{
					LOGFMTE("inform player drag in clubs error, request error, room id = %u , uid = %u", getRoomID(), pPlayer->getUserUID());
					Json::Value jsClubs;
					jsClubs[jsClubs.size()] = isClubRoom();
					jsUserData["clubIDs"] = jsClubs;
					sendMsgToPlayer(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN, pPlayer->getSessionID());
					return;
				}
				//Json::Value jsClubs;
				//jsClubs[jsClubs.size()] = isClubRoom();
				jsUserData["clubIDs"] = retContent["clubIDs"];
				sendMsgToPlayer(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN, pPlayer->getSessionID());
			}, jsMsg);
		}
		else {
			Json::Value jsClubs;
			jsClubs[jsClubs.size()] = isClubRoom();
			jsMsg["clubIDs"] = jsClubs;
			sendMsgToPlayer(jsMsg, MSG_ROOM_THIRTEEN_NEED_DRAGIN, pPlayer->getSessionID());
		}
		return true;
	}

	if (MSG_ROOM_THIRTEEN_APPLAY_DRAG_IN == nMsgType) {
		Json::Value jsRet;
		if (isClubRoom() == 0) {
			jsRet["ret"] = 10;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("Drag in error this room can not drag in? player sessionID = %u", nSessionID);
			return true;
		}
		auto nAmount = prealMsg["amount"].asUInt();
		if (isMTT()) {
			nAmount = getEnterFee();
		}
		auto pPlayer = getPlayerBySessionID(nSessionID);
		if (pPlayer == nullptr)
		{
			jsRet["ret"] = 1;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("you are not in this room how to drag in? session id = %u", nSessionID);
			return true;
		}
		if (nAmount == 0) {
			jsRet["ret"] = 2;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("Drag in amount is missing? player uid = %u", pPlayer->getUserUID());
			return true;
		}
		if (checkDragInAmount(nAmount) == false) {
			jsRet["ret"] = 3;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("Drag in amount is wrong? player uid = %u, amount = %u", pPlayer->getUserUID(), nAmount);
			return true;
		}
		auto nClubID = prealMsg["clubID"].asUInt();
		if (nClubID == 0) {
			jsRet["ret"] = 5;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("Drag in clubID is wrong? player uid = %u, amount = %u", pPlayer->getUserUID(), nAmount);
			return true;
		}
		else if (nClubID != isClubRoom()) {
			if (isLeagueRoom() == 0) {
				jsRet["ret"] = 6;
				sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("Drag in clubID is wrong? player uid = %u, amount = %u, clubID = %u", pPlayer->getUserUID(), nAmount, nClubID);
				return true;
			}
		}

		if (getDelegate()) {
			auto tClubID = getDelegate()->getDragInClubID(pPlayer->getUserUID());
			if (tClubID && tClubID != nClubID) {
				jsRet["ret"] = 6;
				sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("Drag in clubID is wrong? player uid = %u, amount = %u, clubID = %u", pPlayer->getUserUID(), nAmount, nClubID);
				return true;
			}
		}

		Json::Value jsReq;
		jsReq["playerUID"] = pPlayer->getUserUID();
		jsReq["roomID"] = getRoomID();
		jsReq["clubID"] = nClubID;
		jsReq["amount"] = nAmount;
		jsReq["leagueID"] = isLeagueRoom();
		jsReq["roomName"] = getOpts()["name"];
		jsReq["roomLevel"] = getOpts()["level"];
		jsReq["mtt"] = isMTT() ? 1 : 0;
		jsReq["initialCoin"] = getInitialCoin();
		auto pApp = getRoomMgr()->getSvrApp();
		pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_player_apply_DragIn, jsReq,[pApp, this, pPlayer, nSessionID, nAmount, nClubID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			Json::Value jsRet;
			if (isTimeOut)
			{
				LOGFMTE(" request of baseData apply drag in time out uid = %u , can not drag in ", pPlayer->getUserUID());
				jsRet["ret"] = 7;
				sendMsgToPlayer(jsRet, MSG_ROOM_THIRTEEN_APPLAY_DRAG_IN, nSessionID);
				return;
			}

			uint8_t nReqRet = retContent["ret"].asUInt();
			uint8_t nRet = 0;
			do {
				if (0 != nReqRet)
				{
					nRet = (nReqRet == 8) ? 8 : 4;
					break;
				}

				getDelegate()->onPlayerApplyDragIn(pPlayer->getUserUID(), nClubID);
			} while (0);

			jsRet["ret"] = nRet;
			sendMsgToPlayer(jsRet, MSG_ROOM_THIRTEEN_APPLAY_DRAG_IN, nSessionID);
		});
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
				Json::Value jsMsg;
				jsMsg["idx"] = ref->getIdx();
				jsMsg["state"] = 1;
				jsMsg["sys"] = 1;
				sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_GAME_PUT_CARDS_UPDATE);
				((ThirteenPlayer*)ref)->signDetermined();
			}
			else {
				LOGFMTE("auto put cards error, roomID = %u, playerID = %u", getRoomID(), ref->getUserUID());
				return false;
			}
		}
	}
	return true;
}

bool ThirteenRoom::onPlayerSetDao(uint8_t nIdx, IPeerCard::VEC_CARD vCards) {
	if (vCards.size() != MAX_HOLD_CARD_COUNT) {
		return false;
	}
	auto pPlayer = (ThirteenPlayer*)getPlayerByIdx(nIdx);
	auto pCard = pPlayer->getPlayerCard();
	if (pPlayer && isPlayerCanAct(nIdx) && pPlayer->hasDetermined() == false && pCard->checkSetDaoCards(vCards)) {
		ThirteenPeerCard::VEC_CARD vTemp;
		uint8_t nCurIdx = 0;
		for (uint8_t nDao = DAO_HEAD; nDao < DAO_MAX; nDao++) {
			vTemp.clear();
			vTemp.assign(vCards.begin() + (nDao > 0 ? 1 : 0) * HEAD_DAO_CARD_COUNT + (nDao > 0 ? nDao - 1 : 0) * OTHER_DAO_CARD_COUNT, vCards.begin() + HEAD_DAO_CARD_COUNT + OTHER_DAO_CARD_COUNT * nDao);
			if (pCard->setDao(nDao, vTemp) == false) {
				LOGFMTE("put cards error, roomID = %u, playerID = %u, nDao = %u", getRoomID(), nIdx, nDao);
				return false;
			}
		}
		Json::Value jsMsg;
		jsMsg["idx"] = nIdx;
		jsMsg["state"] = 1;
		sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_GAME_PUT_CARDS_UPDATE);
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

bool ThirteenRoom::onPlayerRotBanker(uint8_t nIdx, uint8_t nState) {
	if (m_bRotBanker) {
		return false;
	}
	auto pPlayer = (ThirteenPlayer*)getPlayerByIdx(nIdx);
	if (pPlayer && isPlayerCanAct(nIdx) && pPlayer->hasRotBanker() == false) {
		pPlayer->signRotBanker();
		if (nState) {
			uint32_t nCoin = getBaseScore() / 2;
			if (nCoin < 1) {
				nCoin = 1;
			}
			if (isMTT()) {
				nCoin = 0;
			}
			pPlayer->addSingleOffset(-1 * (int32_t)nCoin);
			Json::Value jsMsg;
			jsMsg["idx"] = nIdx;
			jsMsg["chips"] = pPlayer->getChips();
			sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_GOLDEN_UPDATE);
			m_bRotBanker = true;
			m_nBankerIdx = nIdx;
			if (getDelegate()) {
				getDelegate()->onPlayerRotBanker(pPlayer, nCoin);
			}
		}
		return true;
	}
	return false;
}

bool ThirteenRoom::onPlayerShowCards(uint8_t nIdx) {
	if (isCanMingPai() == false || m_bShowCards) {
		return false;
	}
	auto pPlayer = (ThirteenPlayer*)getPlayerByIdx(nIdx);
	if (pPlayer && isPlayerCanAct(nIdx) && pPlayer->hasShowCards() == false && pPlayer->hasDetermined()) {
		pPlayer->signShowCards();
		m_bShowCards = true;

		for (auto& ref : m_vPlayers) {
			if (ref && ref->getIdx() != nIdx) {
				auto tPlayer = (ThirteenPlayer*)ref;
				if (tPlayer->hasDetermined()) {
					tPlayer->clearDeterMined();
					auto tPCard = tPlayer->getPlayerCard();
					tPCard->reSetDao();
					Json::Value jsMsg;
					jsMsg["idx"] = tPlayer->getIdx();
					jsMsg["state"] = 0;
					sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_GAME_PUT_CARDS_UPDATE);
				}
			}
		}
		return true;
	}
	return false;
}

uint32_t ThirteenRoom::getBaseScore() {
	if (isMTT() && getDelegate()) {
		//return getDelegate()->getBlindBaseScore();
		return m_nMTTBaseScore;
	}

	if (m_jsOpts["baseScore"].isNull() || m_jsOpts["baseScore"].isUInt() == false) {
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

uint8_t ThirteenRoom::getPutCardsTime() {
	if (m_jsOpts["pcTime"].isNull() || m_jsOpts["pcTime"].isUInt() == false) {
		return 30;
	}
	uint8_t nTime = m_jsOpts["pcTime"].asUInt();
	if (nTime < 30) {
		return 30;
	}
	if (nTime > 120) {
		return 120;
	}
	return nTime + 3;
}

bool ThirteenRoom::isCanMingPai() {
	if (m_jsOpts["mingPai"].isNull() || m_jsOpts["mingPai"].isUInt() == false) {
		return false;
	}
	return m_jsOpts["mingPai"].asUInt() ? true : false;
}

bool ThirteenRoom::isPlayerCanMingPai(uint8_t nIdx) {
	auto pPlayer = getPlayerByIdx(nIdx);
	if (pPlayer) {
		int32_t nCoin = 0;
		if (isClubRoom()) {
			return pPlayer->getChips() + nCoin >= getMaxLose();
		}
		else {
			return pPlayer->getChips() >= (int32_t)nCoin;
		}
	}
	return false;
}

bool ThirteenRoom::isCanRotBanker() {
	if (m_jsOpts["rotBanker"].isNull() || m_jsOpts["rotBanker"].isUInt() == false) {
		return false;
	}
	return m_jsOpts["rotBanker"].asUInt() ? true : false;
}

bool ThirteenRoom::isPlayerCanRotBanker(uint8_t nIdx) {
	auto pPlayer = getPlayerByIdx(nIdx);
	if (pPlayer) {
		int32_t nCoin = getBaseScore() / 2;
		if (nCoin < 1) {
			nCoin = 1;
		}
		if (isClubRoom()) {
			return pPlayer->getChips() + nCoin >= getMaxLose();
		}
		else {
			return pPlayer->getChips() >= nCoin;
		}
	}
	return false;
}

bool ThirteenRoom::isMTT() {
	if (m_jsOpts["mtt"].isNull() || m_jsOpts["mtt"].isUInt() == false) {
		return false;
	}
	return m_jsOpts["mtt"].asUInt() ? true : false;
}

uint32_t ThirteenRoom::getEnterFee() {
	if (m_jsOpts["enterFee"].isNull() || m_jsOpts["enterFee"].isUInt() == false) {
		return 0;
	}
	return m_jsOpts["enterFee"].asUInt();
}

uint32_t ThirteenRoom::getInitialCoin() {
	if (m_jsOpts["initialCoin"].isNull() || m_jsOpts["initialCoin"].isUInt() == false) {
		return 0;
	}
	return m_jsOpts["initialCoin"].asUInt();
}

uint8_t ThirteenRoom::getOpenCnt() {
	if (m_jsOpts["starGame"].isNull() || m_jsOpts["starGame"].isUInt() == false) {
		return 2;
	}
	uint8_t nOpenCnt = m_jsOpts["starGame"].asUInt();
	if (nOpenCnt < 2) {
		nOpenCnt = 2;
	}
	else if (nOpenCnt > getSeatCnt()) {
		nOpenCnt = getSeatCnt();
	}
	return nOpenCnt;
}

uint32_t ThirteenRoom::isClubRoom() {
	if (m_jsOpts["clubID"].isNull() || m_jsOpts["clubID"].isUInt() == false) {
		return 0;
	}
	return m_jsOpts["clubID"].asUInt();
}

uint32_t ThirteenRoom::isLeagueRoom() {
	if (m_jsOpts["leagueID"].isNull() || m_jsOpts["leagueID"].isUInt() == false) {
		return 0;
	}
	return m_jsOpts["leagueID"].asUInt();
}

bool ThirteenRoom::hasRotBanker() {
	/*if (m_bRotBanker) {
		return true;
	}
	else {
		for (auto& ref : m_vPlayers) {
			if (ref && ref->haveState(eRoomPeer_StayThisRound) && ((ThirteenPlayer*)ref)->hasRotBanker() == false) {
				return false;
			}
		}
		return true;
	}*/
	return m_bRotBanker;
}

bool ThirteenRoom::isFinishRotBanker() {
	if (m_bRotBanker) {
		return true;
	}
	else {
		for (auto& ref : m_vPlayers) {
			if (ref && ref->haveState(eRoomPeer_StayThisRound) && ((ThirteenPlayer*)ref)->hasRotBanker() == false) {
				return false;
			}
		}
		return true;
	}
}

bool ThirteenRoom::onPlayerDragIn(uint32_t nUserID, uint32_t nAmount) {
	auto pPlayer = (ThirteenPlayer*)getPlayerByUID(nUserID);
	if (pPlayer) {
		pPlayer->addChips(nAmount);
		pPlayer->clearWaitDrgInTime();
		if (pPlayer->haveState(eRoomPeer_WaitDragIn)) {
			if (isMTT() && pPlayer->getChips() < getDragInNeed()) {
				pPlayer->setChips(getDragInNeed());
			}
			pPlayer->setState(eRoomPeer_WaitNextGame);
		}
		Json::Value jsMsg;
		jsMsg["chips"] = pPlayer->getChips();
		jsMsg["idx"] = pPlayer->getIdx();
		sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_DRAG_IN);
		return true;
	}
	return false;
}

bool ThirteenRoom::onPlayerDeclineDragIn(uint32_t nUserID) {
	auto pPlayer = (ThirteenPlayer*)getPlayerByUID(nUserID);
	if (pPlayer) {
		Json::Value jsMsg;
		sendMsgToPlayer(jsMsg, MSG_ROOM_THIRTEEN_DECLINE_DRAG_IN, pPlayer->getSessionID());
		if (pPlayer->haveState(eRoomPeer_WaitDragIn)) {
			doPlayerStandUp(nUserID);
		}
		return true;
	}
	return false;
}

int32_t ThirteenRoom::getMaxLose() {
	uint32_t nMaxLose = 3 + 10 + 5 + GAME_WIN_SWAT_EXTRA;
	if (isCanMingPai()) {
		nMaxLose += 10;
	}
	if (isCanRotBanker()) {
		nMaxLose *= 2;
	}

	for (uint8_t i = 1; i < getSeatCnt(); i++) {
		uint32_t nTemp = 3 + 10 + 5 + GAME_WIN_SHOOT_EXTRA;
		/*if (isCanMingPai()) {
			nTemp += 10;
		}
		if (isCanRotBanker()) {
			nTemp *= 2;
		}*/
		nMaxLose += nTemp;
	}
	//nMaxLose += (3 + 10 + 5 + 6) * 2;
	return nMaxLose/* * getBaseScore()*/;
}

int32_t ThirteenRoom::getMaxDragIn() {
	if (isMTT()) {
		return getEnterFee();
	}
	return getBaseScore() * BASE_DRAGIN_RATIO * getMultiple();
}

int32_t ThirteenRoom::getMinDragIn() {
	if (isMTT()) {
		return getEnterFee();
	}
	return getBaseScore() * BASE_DRAGIN_RATIO;
}

int32_t ThirteenRoom::getDragInNeed() {
	if (isMTT()) {
		return 1;
	}
	return getBaseScore() * BASE_DRAGIN_RATIO / 2;
}

uint8_t ThirteenRoom::getWinShui(uint8_t nIdx, uint8_t nWinType, uint8_t nDaoIdx) {
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
		nShui += GAME_WIN_SHOOT_EXTRA;
		for (uint8_t nDao = DAO_HEAD; nDao < DAO_MAX; nDao++) {
			uint8_t nTShui = getDaoWinShui(pCard->getType(nDao), nDao);
			nShui += nTShui > 1 ? nTShui : 0;
		}
		break;
	}
	case WIN_SHUI_TYPE_SHOWCARDS: {
		nShui += GAME_SHWO_CARDS_EXTRA_SHUI;
		for (uint8_t nDao = DAO_HEAD; nDao < DAO_MAX; nDao++) {
			uint8_t nTShui = getDaoWinShui(pCard->getType(nDao), nDao);
			nShui += nTShui > 1 ? nTShui : 0;
		}
		break;
	}
	case WIN_SHUI_TYPE_SWAT: {
		nShui += GAME_WIN_SWAT_EXTRA;
		for (uint8_t nDao = DAO_HEAD; nDao < DAO_MAX; nDao++) {
			uint8_t nTShui = getDaoWinShui(pCard->getType(nDao), nDao);
			nShui += nTShui > 1 ? nTShui : 0;
		}
		break;
	}
	}

	return nShui;
}

uint32_t ThirteenRoom::getWinCoin(uint8_t nIdx, uint8_t nWinType, uint8_t nDaoIdx) {
	return ((uint32_t)getWinShui(nIdx, nWinType, nDaoIdx)) * getBaseScore();
}

IGameRoomDelegate* ThirteenRoom::getDelegate() {
	auto pDelegate = GameRoom::getDelegate();
	if (pDelegate) {
		pDelegate->setCurrentPointer(this);
		return pDelegate;
	}
	else {
		return nullptr;
	}
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

uint8_t ThirteenRoom::doProduceNewBanker()
{
	if (m_nBankerIdx == (uint8_t)-1) {
		m_nBankerIdx = rand() % getSeatCnt();
		for (uint8_t i = 0; i < getSeatCnt(); i++) {
			auto pBanker = getPlayerByIdx(m_nBankerIdx);
			if (pBanker && pBanker->haveState(eRoomPeer_StayThisRound)) {
				break;
			}
			else {
				m_nBankerIdx = (m_nBankerIdx + 1) % getSeatCnt();
			}
		}
		m_bRotBanker = false;
	}
	else {
		if (m_bRotBanker == false) {
			m_nBankerIdx = (m_nBankerIdx + 1) % getSeatCnt();
			for (uint8_t i = 0; i < getSeatCnt(); i++) {
				auto pBanker = getPlayerByIdx(m_nBankerIdx);
				if (pBanker && pBanker->haveState(eRoomPeer_StayThisRound)) {
					break;
				}
				else {
					m_nBankerIdx = (m_nBankerIdx + 1) % getSeatCnt();
				}
			}
		}
	}

	// send msg tell new banker ;
	Json::Value jsMsg;
	jsMsg["bankerIdx"] = m_nBankerIdx;
	jsMsg["rotBanker"] = m_bRotBanker ? 1 : 0;
	sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_PRODUCED_BANKER);

	return m_nBankerIdx;
}

bool ThirteenRoom::checkDragInAmount(uint32_t nAmount) {
	if (nAmount > 0) {
		uint32_t nMin = getMinDragIn();
		if (nAmount < nMin) {
			return false;
		}
		uint32_t nMax = nMin * getMultiple();
		if (nMax && nAmount > nMax) {
			return false;
		}
		return true;
	}
	return false;
}

bool ThirteenRoom::doPlayerAutoLeave() {
	for (auto& ref : m_vPlayers) {
		if (ref && ((ThirteenPlayer*)ref)->isAutoLeave()) {
			auto nSessionID = ref->getSessionID();
			doPlayerLeaveRoom(ref->getUserUID());
			Json::Value jsMsg;
			sendMsgToPlayer(jsMsg, MSG_ROOM_PLAYER_LEAVE, nSessionID);
		}
	}
	return true;
}

bool ThirteenRoom::doPlayerAutoStandUp() {
	for (auto& ref : m_vPlayers) {
		if (ref && ((ThirteenPlayer*)ref)->isAutoStandUp()) {
			doPlayerStandUp(ref->getUserUID());
		}
	}
	return true;
}

void ThirteenRoom::requestHttpRoomInfo(Json::Value& jsMsg) {
	if (getCurState()->getStateID() != eRoomState_WaitPlayerAct) {
		return;
	}
	for (auto& ref : m_vPlayers) {
		if (ref && isPlayerCanAct(ref->getIdx()) && ((ThirteenPlayer*)ref)->hasDetermined()) {
			Json::Value jsPlayer, jsCards;
			((ThirteenPlayer*)ref)->getPlayerCard()->groupCardToJson(jsCards);
			jsPlayer["uid"] = ref->getUserUID();
			jsPlayer["cards"] = jsCards;
			jsMsg[jsMsg.size()] = jsPlayer;
		}
	}
}