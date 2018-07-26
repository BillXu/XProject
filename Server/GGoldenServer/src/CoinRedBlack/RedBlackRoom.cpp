#include "CoinRedBlack\RedBlackRoom.h"
#include "CoinRedBlack\RedBlackPlayer.h"
#include "IGameRoomDelegate.h"
#include "IGameRoomManager.h"
#include "AsyncRequestQuene.h"
#include "ISeverApp.h"
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
	for (auto& ref : m_vBetPool)
	{
		ref.clear();
	}

	return GameRoom::init(pRoomMgr,nSeialNum,nRoomID,nSeatCnt,vJsOpts);
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
	m_vRecorders.push_back(str);

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

	Json::Value jsred;
	jsred["type"] = m_vBetPool[eBet_Red].tPeer.getType();
	Json::Value jsredCard;
	m_vBetPool[eBet_Red].tPeer.toJson(jsredCard);
	jsred["cards"] = jsredCard;
	jsMsg["red"] = jsred;

	Json::Value jsBlack;
	jsBlack["type"] = m_vBetPool[eBet_Black].tPeer.getType();
	Json::Value jsBlackCard;
	m_vBetPool[eBet_Black].tPeer.toJson(jsBlackCard);
	jsBlack["cards"] = jsBlackCard;
	jsMsg["black"] = jsBlack;

	sendRoomMsg(jsMsg, MSG_RB_ROOM_RESULT );

	GameRoom::onGameEnd();
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
	GameRoom::onWillStartGame();
}

void RedBlackRoom::onStartGame()
{
	GameRoom::onStartGame();
	// send msg inform bet ;
	Json::Value jsMsg;
	sendRoomMsg(jsMsg, MSG_RB_START_GAME);
}

void RedBlackRoom::onGameDidEnd()
{
	GameRoom::onGameDidEnd();
	// any body need to leave ?
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
	
	Json::Value jsbetPool;
	for ( auto& betPool : m_vBetPool)
	{
		jsbetPool[jsbetPool.size()] = betPool.nBetChips;
	}
	jsRoomInfo["vBetPool"] = jsbetPool;

	Json::Value jsWinRecorder;
	Json::Value jsWinType;
	for ( auto& ref : m_vRecorders)
	{
		jsWinRecorder[jsWinRecorder.size()] = ref.eWinPort;
		if (jsWinRecorder.size() + MAX_RB_TYPE_RECORDER_CNT > m_vRecorders.size())
		{
			jsWinType[jsWinType.size()] = ref.eType;
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