#include "CoinRedBlack\RedBlackRoom.h"
#include "CoinRedBlack\RedBlackPlayer.h"
#include "IGameRoomDelegate.h"
#include "IGameRoomManager.h"
#include "AsyncRequestQuene.h"
#include "ISeverApp.h"
#include "RedBlackRoomStateResult.h"
#include "RedBlackRoomStateStartBet.h"
#define MAX_RB_WIN_RECORDER_CNT 70 
#define MAX_RB_TYPE_RECORDER_CNT 20 
RedBlackRoom::~RedBlackRoom()
{
	for (auto& ref : m_vStandGamePlayers)
	{
		delete ref.second;
		ref.second = nullptr;
	}
	m_vStandGamePlayers.clear();
}

bool RedBlackRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	m_nBankerUID = -1;
	m_nRichestPlayer = -1;
	m_nBestBetPlayer = -1;
	for (auto& ref : m_vBetPool)
	{
		ref.clear();
	}
	
	auto ret = GameRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
	IGameRoomState* pState = new RedBlackRoomStateStartBet();
	addRoomState(pState);
	setInitState(pState);
	pState = new RedBlackRoomStateResult();
	addRoomState(pState);
	return ret;
}

bool RedBlackRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
{
	switch (nMsgType)
	{
	case MSG_PLAYER_SIT_DOWN:
	{
		uint16_t nIdx = prealMsg["idx"].asUInt();
		IGamePlayer* pStand = nullptr;
		uint8_t nRet = 0;
		do
		{
			auto pPlayer = GameRoom::getPlayerBySessionID(nSessionID);
			if (pPlayer)
			{
				nRet = 4;
				break;
			}

			pStand = getPlayerBySessionID(nSessionID);
			if (!pStand)
			{
				nRet = 3;
				break;
			}

			if (nIdx > getSeatCnt()) // find empty pos
			{
				auto nCheckIdx = rand() % getSeatCnt();
				for (; nCheckIdx < getSeatCnt() * 2; ++nCheckIdx)
				{
					nIdx = nCheckIdx % getSeatCnt();
					auto p = getPlayerByIdx(nIdx);
					if (!p)
					{
						break;
					}
				}
			}

			pPlayer = getPlayerByIdx(nIdx);
			if (pPlayer)
			{
				nRet = 1;
				break;
			}
		} while (0);

		if (nRet)
		{
			Json::Value jsRet;
			jsRet["ret"] = nRet;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("uid = %u sitdown error ret = %u", pStand->getUserUID(), nRet);
			break;
		}

		stEnterRoomData tInfo;
		tInfo.nUserUID = pStand->getUserUID();
		tInfo.nSessionID = nSessionID;
		tInfo.nDiamond = 0;
		tInfo.nChip = pStand->getChips();
		auto nDret = doPlayerSitDown(&tInfo, nIdx) ? 0 : 6;

		Json::Value jsRet;
		jsRet["ret"] = nDret;
		sendMsgToPlayer(jsRet, MSG_PLAYER_SIT_DOWN, nSessionID);
	}
	break;
	default:
		return GameRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
	}
	return true;
}

IGamePlayer* RedBlackRoom::createGamePlayer()
{
	return new RedBlackPlayer();
}

IPoker* RedBlackRoom::getPoker()
{
	return &m_tPoker;
}

uint8_t RedBlackRoom::getRoomType()
{
	return eGame_Golden;
}

void RedBlackRoom::onGameEnd()
{
	doDistribute();
	// caculate result ;
	const auto nSeatCnt = getSeatCnt();
	std::vector<RedBlackPlayer*> vPlayers;
	for (auto nidx = 0; nidx < nSeatCnt; ++nidx)
	{
		auto p = (RedBlackPlayer*)getPlayerByIdx( nidx );
		if ( !p )
		{
			continue;
		}
		vPlayers.push_back(p);
	}

	for ( auto& pp : m_vStandGamePlayers )
	{
		auto p = (RedBlackPlayer*)pp.second;
		if (!p)
		{
			continue;
		}
		vPlayers.push_back(p);

	}

	auto isRedWin = m_vBetPool[eBet_Red].tPeer > m_vBetPool[eBet_Black].tPeer;
	auto nWinRate = getWinRateForCardType((CGoldenPeerCard::GoldenType)m_vBetPool[isRedWin ? eBet_Red : eBet_Black].tPeer.getType());
	int32_t nBankerLose = 0;
	for (auto& p : vPlayers)
	{
		if ( p->getUserUID() == m_nBankerUID )
		{
			continue;
		}
		p->addSingleOffset(p->getBetCoin(eBet_Black) * (isRedWin ? -1 : 1));
		p->addSingleOffset(p->getBetCoin(eBet_Red) * (isRedWin ? 1 : -1));
		p->addSingleOffset(nWinRate * p->getBetCoin(eBet_Other));
		nBankerLose += p->getSingleOffset();
	}

	if ( m_nBankerUID > 0 )
	{
		auto pBanker = getPlayerByUID( m_nBankerUID );
		if ( pBanker == nullptr || pBanker->getChips() < nBankerLose )
		{
			LOGFMTE( "banker is null , or coin is not engough banker = %u, lose = %d",m_nBankerUID,nBankerLose );
		}
		else
		{
			pBanker->addSingleOffset( -1 * nBankerLose );
			if ( pBanker->getChips() < getCoinNeedToBeBanker() )
			{
				m_nBankerUID = -1;
				// send msg 
				Json::Value jsmsg;
				jsmsg["newBankerID"] = m_nBankerUID;
				jsmsg["coin"] = 99999999;
				sendRoomMsg(jsmsg, MSG_RB_UPDATE_BANKER);
			}
		}
	}
	else
	{
		LOGFMTE( "no banker , so system lose = %d",nBankerLose ); 
	}

	if ( m_vRecorders.size() >= MAX_RB_WIN_RECORDER_CNT )
	{
		m_vRecorders.pop_front();
	}

	stRecorder str;
	str.eWinPort = isRedWin ? eBet_Red : eBet_Black;
	str.eType = (CGoldenPeerCard::GoldenType)m_vBetPool[str.eWinPort].tPeer.getType();
	str.nKeyCardValue = m_vBetPool[str.eWinPort].tPeer.getPairKeyValue();
	m_vRecorders.push_back(str);

	LOGFMTI("recorder size = %u room Id = %u", m_vRecorders.size(), getRoomID());
	// send msg to client ;
	Json::Value vJsResults;
	for ( auto idx = 0; idx < nSeatCnt; ++idx )
	{
		auto p = getPlayerByIdx(idx);
		if ( !p )
		{
			continue;
		}
		Json::Value jsItem;
		jsItem["idx"] = idx;
		jsItem["offset"] = p->getSingleOffset();
		vJsResults[vJsResults.size()] = jsItem;
	}

	Json::Value jsMsg;
	jsMsg["result"] = vJsResults;
	jsMsg["isRedWin"] = isRedWin ? 1 : 0;
	jsMsg["bankerLose"] = nBankerLose;

	auto pBest = getPlayerByUID(getBestBetPlayer());
	jsMsg["bestBetOffset"] = 0;
	if (pBest)
	{
		jsMsg["bestBetOffset"] = pBest->getSingleOffset();
	}

	auto pRichest = getPlayerByUID(getRichestPlayerUID());
	jsMsg["richestOffset"] = 0;
	if (pRichest)
	{
		jsMsg["richestOffset"] = pRichest->getSingleOffset();
	}

	Json::Value jsred;
	jsred["T"] = m_vBetPool[eBet_Red].tPeer.getType();
	jsred["V"] = m_vBetPool[eBet_Red].tPeer.getPairKeyValue();
	Json::Value jsredCard;
	m_vBetPool[eBet_Red].tPeer.toJson(jsredCard);
	jsred["cards"] = jsredCard;
	jsMsg["red"] = jsred;

	Json::Value jsBlack;
	jsBlack["T"] = m_vBetPool[eBet_Black].tPeer.getType();
	jsBlack["V"] = m_vBetPool[eBet_Black].tPeer.getPairKeyValue();
	Json::Value jsBlackCard;
	m_vBetPool[eBet_Black].tPeer.toJson(jsBlackCard);
	jsBlack["cards"] = jsBlackCard;
	jsMsg["black"] = jsBlack;

	// send msg to client 
	for (auto& ref : m_vStandGamePlayers)
	{
		if (ref.second)
		{
			jsMsg["selfOffset"] = ref.second->getSingleOffset();
			sendMsgToPlayer(jsMsg, MSG_RB_ROOM_RESULT , ref.second->getSessionID() );
		}
	}

	for (auto idx = 0; idx < nSeatCnt; ++idx)
	{
		auto p = getPlayerByIdx(idx);
		if (!p)
		{
			continue;
		}
		jsMsg["selfOffset"] = p->getSingleOffset();
		sendMsgToPlayer(jsMsg, MSG_RB_ROOM_RESULT, p->getSessionID());
	}
	// send msg to client  ---end 


	GameRoom::onGameEnd();
	for (auto& ref : m_vStandGamePlayers)
	{
		if (ref.second)
		{
			ref.second->onGameEnd();
		}
	}
}

void RedBlackRoom::onWillStartGame()
{
	for (auto& ref : m_vBetPool)
	{
		ref.clear();
	}
	
	
	if ( m_nBankerUID == -1 && m_vApplyBanker.empty() == false )
	{
		// find banker ;
		for (auto& ref : m_vApplyBanker)
		{
			auto p = getPlayerByUID(ref);
			if (p == nullptr)
			{
				LOGFMTE( "why apply banker id player is null ? id = %u",ref );
				continue;
			}

			if ( p->getChips() >= getCoinNeedToBeBanker())
			{
				m_nBankerUID = ref;
				if (p->getIdx() < getSeatCnt())
				{
					doPlayerStandUp( m_nBankerUID );
				}

				// send msg 
				Json::Value jsmsg;
				jsmsg["newBankerID"] = m_nBankerUID;
				jsmsg["coin"] = p->getChips();
				sendRoomMsg(jsmsg, MSG_RB_UPDATE_BANKER );
				break;
			}
		}
	}


	for (auto& ref : m_vStandGamePlayers)
	{
		if (ref.second)
		{
			ref.second->onGameWillStart();
		}
	}

	GameRoom::onWillStartGame();
}

void RedBlackRoom::onStartGame()
{
	GameRoom::onStartGame();
	for (auto& ref : m_vStandGamePlayers)
	{
		if (ref.second)
		{
			ref.second->onGameStart();
		}
	}
	// send msg inform bet ;
	Json::Value jsMsg;
	sendRoomMsg(jsMsg, MSG_RB_START_GAME);
}

void RedBlackRoom::onGameDidEnd()
{
	// check best bet and richest update 
	auto pR = getPlayerByUID( m_nRichestPlayer );
	auto isUpdateRichAndBestBet = false;
	if (pR == nullptr || pR->getSingleOffset() < 0)
	{
		auto nol = m_nRichestPlayer;
		m_nRichestPlayer = getRichestPlayerUID();
		if ( nol != m_nRichestPlayer )
		{
			doPlayerStandUp(m_nRichestPlayer);
			isUpdateRichAndBestBet = true;
		}
		
	}

	auto pBest = getPlayerByUID( m_nBestBetPlayer );
	if (pBest == nullptr || pBest->getSingleOffset() < 0)
	{
		auto nol = m_nBestBetPlayer;
		m_nBestBetPlayer = getBestBetPlayer();
		if (nol != m_nBestBetPlayer)
		{
			doPlayerStandUp(m_nBestBetPlayer);
			isUpdateRichAndBestBet = true;
		}
	}

	if ( isUpdateRichAndBestBet )
	{
		informRichAndBestBetPlayerUpdate();
	}

	GameRoom::onGameDidEnd();
	// any body need to leave ?

	for (auto& ref : m_vStandGamePlayers)
	{
		if (ref.second)
		{
			ref.second->onGameDidEnd();
		}
	}

	do
	{
		auto iter = std::find_if(m_vApplyBanker.begin(), m_vApplyBanker.end(), [this](auto& ref) 
		{ 
			auto p = getPlayerByUID(ref); 
			if (p == nullptr || p->getChips() < getCoinNeedToBeBanker())
			{
				return true;
			}
			return false;
		});

		if ( m_vApplyBanker.end() == iter )
		{
			break;
		}
		m_vApplyBanker.erase(iter);
	} while (true);
}

IGamePlayer* RedBlackRoom::getPlayerByUID(uint32_t nUserUID)
{
	auto p = GameRoom::getPlayerByUID( nUserUID );
	if ( p )
	{
		return p;
	}

	auto iter = m_vStandGamePlayers.find(nUserUID);
	if (iter == m_vStandGamePlayers.end())
	{
		return nullptr;
	}
	return iter->second;
}

IGamePlayer* RedBlackRoom::getPlayerBySessionID(uint32_t nSessionID)
{
	auto p = GameRoom::getPlayerBySessionID(nSessionID);
	if (p)
	{
		return p;
	}

	for (auto& ref : m_vStandGamePlayers)
	{
		if ( ref.second->getSessionID() == nSessionID )
		{
			return ref.second;
		}
	}
	return nullptr;
}

bool RedBlackRoom::doPlayerSitDown(stEnterRoomData* pEnterRoomPlayer, uint16_t nIdx)
{
	auto p = getPlayerByIdx(nIdx);
	if (p != nullptr)
	{
		return false;
	}

	if ( m_nRichestPlayer == pEnterRoomPlayer->nUserUID || pEnterRoomPlayer->nUserUID == m_nBestBetPlayer )
	{
		LOGFMTE( "richest or best bet player can not do sit down self " );
		return false;
	}

	auto iter = m_vStandGamePlayers.find(pEnterRoomPlayer->nUserUID);
	if ( m_vStandGamePlayers.end() == iter || iter->second == nullptr )
	{
		return false;
	}
	m_vPlayers[nIdx] = iter->second;
	iter->second->setIdx(nIdx);
	m_vStandGamePlayers.erase(iter);

	// send msg tell players ;
	Json::Value jsRoomPlayerSitDown;
	visitPlayerInfo(m_vPlayers[nIdx], jsRoomPlayerSitDown, 0);
	sendRoomMsg(jsRoomPlayerSitDown, MSG_ROOM_SIT_DOWN);
	if (getDelegate())
	{
		getDelegate()->onPlayerSitDown(this, p);
	}
	return true;
}

bool RedBlackRoom::doPlayerStandUp(uint32_t nUserUID)
{
	auto p = GameRoom::getPlayerByUID( nUserUID );
	if ( nullptr == p )
	{
		return false;
	}

	auto iter = m_vStandGamePlayers.find( nUserUID );
	if ( iter != m_vStandGamePlayers.end() )
	{
		LOGFMTE( "why have two player obj uid = %u",nUserUID );
		return false;
	}

	// send standup msg 
	Json::Value jsMsg;
	jsMsg["idx"] = p->getIdx();
	jsMsg["uid"] = p->getUserUID();
	sendRoomMsg(jsMsg, MSG_ROOM_STAND_UP);

	m_vPlayers[p->getIdx()] = nullptr;
	p->setIdx(-1);
	m_vStandGamePlayers[nUserUID] = p;
	return true;
}

bool RedBlackRoom::doPlayerLeaveRoom(uint32_t nUserUID)
{
	auto pPlayer = GameRoom::getPlayerByUID(nUserUID);
	if (pPlayer)
	{
		doPlayerStandUp(nUserUID);
	}

	auto iterStand = m_vStandGamePlayers.find(nUserUID);
	if ( m_vStandGamePlayers.end() == iterStand )
	{
		LOGFMTE("uid = %u not stand in this room = %u how to leave ?", nUserUID, getRoomID());
		return false;
	}

	if ( nUserUID == m_nBankerUID )
	{
		m_nBankerUID = -1;
		// send msg 
		Json::Value jsmsg;
		jsmsg["newBankerID"] = m_nBankerUID;
		jsmsg["coin"] = 99999999;
		sendRoomMsg(jsmsg, MSG_RB_UPDATE_BANKER);
	}

	// check shen suanzi and da fu hao 
	if (m_nBestBetPlayer == nUserUID)
	{
		m_nBestBetPlayer = -1;
	}
	
	if (m_nRichestPlayer == nUserUID)
	{
		m_nRichestPlayer = -1;
	}
	informRichAndBestBetPlayerUpdate();

	if (getDelegate())
	{
		getDelegate()->onPlayerDoLeaved(this, nUserUID);
	}

	delete iterStand->second;
	m_vStandGamePlayers.erase(iterStand);

	// tell data svr player leave ;
	Json::Value jsReqLeave;
	jsReqLeave["targetUID"] = nUserUID;
	jsReqLeave["roomID"] = getRoomID();
	jsReqLeave["port"] = getRoomMgr()->getSvrApp()->getLocalSvrMsgPortType();
	auto pAsync = getRoomMgr()->getSvrApp()->getAsynReqQueue();
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserUID, eAsync_Inform_Player_LeavedRoom, jsReqLeave);
	return true;
}

void RedBlackRoom::sendRoomMsg(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID )
{
	GameRoom::sendRoomMsg(prealMsg,nMsgType,nOmitSessionID);
	for (auto& ref : m_vStandGamePlayers)
	{
		sendMsgToPlayer(prealMsg, nMsgType, ref.second->getSessionID() );
	}
}

void RedBlackRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	GameRoom::packRoomInfo(jsRoomInfo);
	jsRoomInfo["bankerID"] = m_nBankerUID;

	auto pBest = getPlayerByUID(getBestBetPlayer());
	if ( pBest )
	{
		jsRoomInfo["bestBetUID"] = getBestBetPlayer();
		jsRoomInfo["bestBetCoin"] = pBest->getChips();
	}
	else
	{
		jsRoomInfo["bestBetUID"] = -1;
		jsRoomInfo["bestBetCoin"] = 0;
	}
	
	auto pRich = getPlayerByUID( getRichestPlayerUID() );
	if (pRich)
	{
		jsRoomInfo["richestUID"] = getRichestPlayerUID();
		jsRoomInfo["richestCoin"] = pRich->getChips();
	}
	else
	{
		jsRoomInfo["bestBetUID"] = -1;
		jsRoomInfo["richestCoin"] = 0;
	}
	
	
	Json::Value jsbetPool;
	for ( auto& betPool : m_vBetPool)
	{
		jsbetPool[jsbetPool.size()] = betPool.nBetChips;
	}
	jsRoomInfo["vBetPool"] = jsbetPool;

	LOGFMTI("recorder size = %u room Id = %u", m_vRecorders.size(),getRoomID() );
	Json::Value jsWinRecorder;
	Json::Value jsWinType;
	for ( auto& ref : m_vRecorders)
	{
		jsWinRecorder[jsWinRecorder.size()] = ref.eWinPort;
		if (jsWinRecorder.size() + MAX_RB_TYPE_RECORDER_CNT > m_vRecorders.size())
		{
			Json::Value js;
			js["T"] = ref.eType;
			js["V"] = ref.nKeyCardValue;
			jsWinType[jsWinType.size()] = js;
		}
	}

	jsRoomInfo["vWinRecord"] = jsWinRecorder;
	jsRoomInfo["vTypeRecord"] = jsWinType;
}

int32_t RedBlackRoom::getCoinNeedToBeBanker()
{
	return 10000;
}

int32_t RedBlackRoom::getWinRateForCardType(CGoldenPeerCard::GoldenType eType)
{
	return -1;
}

void RedBlackRoom::doDistribute()
{
	for ( uint8_t nIdx = 0; nIdx < GOLDEN_PEER_CARD; ++nIdx )
	{
		m_vBetPool[eBet_Black].tPeer.addCompositCardNum(getPoker()->distributeOneCard());
		m_vBetPool[eBet_Red].tPeer.addCompositCardNum(getPoker()->distributeOneCard());
	}
}

int32_t RedBlackRoom::getPoolCapacityToBet(eBetPool ePool)
{
	if ( -1 == m_nBankerUID )
	{
		return 99999999999;
	}

	// decide by banker 
	return 99999999;
}

RedBlackRoom::stBetPool& RedBlackRoom::getPool(eBetPool ePool)
{
	if ( ePool >= eBet_Max)
	{
		LOGFMTE("pool type error = %u", ePool);
		ePool = eBet_Black;
	}
	return m_vBetPool[ePool];
}

bool RedBlackRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)
{
	// check is already sit down ?
	auto psitDown = getPlayerByUID(pEnterRoomPlayer->nUserUID);
	if (psitDown)
	{
		LOGFMTE("this player is already sitdown uid = %u , room id = %u , why enter room again ?", psitDown->getUserUID(), getRoomID());
		//sendRoomInfo(pEnterRoomPlayer->nSessionID);
		psitDown->setNewSessionID(pEnterRoomPlayer->nSessionID);
		return true;
	}

	auto iterStand = m_vStandGamePlayers.find(pEnterRoomPlayer->nUserUID);
	if (iterStand == m_vStandGamePlayers.end())
	{
		auto p = createGamePlayer();
		p->init(pEnterRoomPlayer, -1);
		m_vStandGamePlayers[pEnterRoomPlayer->nUserUID] = p;
		LOGFMTD("room id = %u , player uid = %u enter room chip = %u", getRoomID(), p->getUserUID() , pEnterRoomPlayer->nChip);

		// check if you are richest ;
		auto pCurRich = getPlayerByUID( getRichestPlayerUID() );
		if ( pCurRich == nullptr || pCurRich->getChips() < p->getChips())
		{
			m_nRichestPlayer = p->getUserUID();
			informRichAndBestBetPlayerUpdate();
		}
	}
	else
	{
		LOGFMTE("room id = %u uid = %u already in this room why enter again ? chipRoom = %d , chipOut = %d", getRoomID(), pEnterRoomPlayer->nUserUID, iterStand->second->getChips(), pEnterRoomPlayer->nChip);
		iterStand->second->setNewSessionID(pEnterRoomPlayer->nSessionID);;
	}
	//sendRoomInfo(pEnterRoomPlayer->nSessionID);
	LOGFMTI( "room id = %u , player do enter room = %u",getRoomID(),pEnterRoomPlayer->nUserUID  );
	return true;
}

int32_t RedBlackRoom::getRichestPlayerUID()
{
	if ( m_nRichestPlayer == -1 )
	{
		int32_t nBestCoin = 0;
		for (auto& ref : m_vStandGamePlayers)
		{
			if (ref.second &&  ( m_nRichestPlayer == -1 || nBestCoin < ref.second->getChips() ) )
			{
				m_nRichestPlayer = ref.second->getUserUID();
				nBestCoin = ref.second->getChips();
			}
		}

		auto nSt = getSeatCnt();
		for ( uint8_t nIdx = 0; nIdx < nSt; ++nIdx )
		{
			auto p = getPlayerByIdx(nIdx);
			if ( p && (m_nRichestPlayer == -1 || nBestCoin < p->getChips()))
			{
				m_nRichestPlayer = p->getUserUID();
				nBestCoin = p->getChips();
			}
		}
	}
	return m_nRichestPlayer;
}

int32_t RedBlackRoom::getBestBetPlayer()
{
	if ( m_nBestBetPlayer == -1 )
	{
		int32_t nWinTimes = 0;
		for (auto& ref : m_vStandGamePlayers)
		{
			auto pp = (RedBlackPlayer*)ref.second;
			if (pp && (m_nBestBetPlayer == -1 || nWinTimes < pp->getWinTimes() ))
			{
				m_nBestBetPlayer = ref.second->getUserUID();
				nWinTimes = pp->getWinTimes();
			}
		}

		auto nSt = getSeatCnt();
		for (uint8_t nIdx = 0; nIdx < nSt; ++nIdx)
		{
			auto pp = (RedBlackPlayer*)getPlayerByIdx(nIdx);
			if (pp && (m_nBestBetPlayer == -1 || nWinTimes < pp->getWinTimes()))
			{
				m_nBestBetPlayer = pp->getUserUID();
				nWinTimes = pp->getWinTimes();
			}
		}
	}
	return m_nBestBetPlayer;
}

void RedBlackRoom::informRichAndBestBetPlayerUpdate()
{
	Json::Value jsmsg;
	auto pBest = getPlayerByUID(getBestBetPlayer());
	if (pBest)
	{
		jsmsg["bestBetUID"] = getBestBetPlayer();
		jsmsg["bestBetCoin"] = pBest->getChips();
	}
	else
	{
		jsmsg["bestBetUID"] = -1;
		jsmsg["bestBetCoin"] = 0;
	}

	auto pRich = getPlayerByUID(getRichestPlayerUID());
	if (pRich)
	{
		jsmsg["richestUID"] = getRichestPlayerUID();
		jsmsg["richestCoin"] = pRich->getChips();
	}
	else
	{
		jsmsg["bestBetUID"] = -1;
		jsmsg["richestCoin"] = 0;
	}
	sendRoomMsg(jsmsg, MSG_RB_UPDATE_RICH_AND_BEST );
}